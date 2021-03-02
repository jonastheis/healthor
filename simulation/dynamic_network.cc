#include "dynamic_network.h"


cGate *getInputGate(cModule *node, int index) {
    return node->gate("gate$i", index);
}

cGate *getOutputGate(cModule *node, int index) {
    return node->gate("gate$o", index);
}

cGate *getFirstUnconnectedInputGate(cModule *node) {
    for (int i=0; i<node->gateSize("gate"); i++) {
        cGate *in = getInputGate(node, i);
        if(!in->isConnected()) {
            return in;
        }
    }

    return nullptr;
}

cGate *getFirstUnconnectedOutputGate(cModule *node) {
    for (int i=0; i<node->gateSize("gate"); i++) {
        cGate *out = getOutputGate(node, i);
        if(!out->isConnected()) {
            return out;
        }
    }

    return nullptr;
}


void DynamicNetwork::initialize() {
    int n = this->getSubmodule("config")->par("n");
    int conns = this->getSubmodule("config")->par("conns");

    std::map<int, bool> seenNodes;
    // find factory object to create channels
    cChannelType *channelType = cChannelType::get("VolatileChannel");
    cChannel *channel;

    // iterate through all nodes
    for (int i=0; i<n; i++) {
        seenNodes.clear();
        cModule *node = this->getSubmodule("node", i);

        // for each gate of a node
        // if not connected: pick random node and connect
        for(int g=0; g<conns; g++) {
            cGate *nodeOut = getOutputGate(node, g);
            if(!nodeOut->isConnected()) {
                cGate *otherNodeIn = nullptr;

                // repeat until non duplicate node with unconnected gate is found
                do {
                    int randomNodeId = intuniform(i+1, n-1);
//                    EV << "i: " << i << " g: " << g << " random: " << randomNodeId << "\n";
                    if(seenNodes.count(randomNodeId) > 0) {
                        continue;
                    }

                    // mark id as already visited
                    seenNodes[randomNodeId] = true;

                    cModule *otherNode = this->getSubmodule("node", randomNodeId);

                    // get first free gate of node
                    otherNodeIn = getFirstUnconnectedInputGate(otherNode);
                    // skip if there's no free gate
                    if(otherNodeIn == nullptr) {
                        break;
                    }


                    // connect nodes' via in and out
                    channel = channelType->create("channel");
                    nodeOut->connectTo(otherNodeIn, channel);
                    channel->callInitialize();

                    channel = channelType->create("channel");
                    otherNodeIn->getOtherHalf()->connectTo(getInputGate(node, g), channel);
                    channel->callInitialize();
                } while(otherNodeIn == nullptr);

            }
        }
    }


    std::vector<cModule*> unconnectedNodes;
    // connect nodes with < 4
    for (int i=0; i<n; i++) {
        cModule *node = this->getSubmodule("node", i);
        int count = 0;
        for(int g=0; g<conns; g++) {
            cGate *nodeOut = getOutputGate(node, g);
            if(nodeOut->isConnected()) {
                count++;
            }
        }

        if(count < 4) {
            unconnectedNodes.push_back(node);
        }
    }

    std::vector<cModule*> newlyConnected;
    int count = 0;
    while(count < 4) {
//        EV << unconnectedNodes.size() << "\n";
        for (std::vector<cModule*>::iterator i = unconnectedNodes.begin(); i != unconnectedNodes.end(); ++i) {
            cModule *node = *i;
            cGate *nodeOut = getFirstUnconnectedOutputGate(node);
            if(nodeOut == nullptr) {
                continue;
            }

            int r;
            cModule *otherNode = nullptr;
            do {
                r = intuniform(0, unconnectedNodes.size()-1);
                otherNode = unconnectedNodes[r];
            } while(node == otherNode);

            cGate *otherNodeIn = getFirstUnconnectedInputGate(otherNode);
            if(otherNodeIn == nullptr) {
                continue;
            }

            // connect nodes' via in and out
            channel = channelType->create("channel");
            nodeOut->connectTo(otherNodeIn, channel);
            channel->callInitialize();

            channel = channelType->create("channel");
            otherNodeIn->getOtherHalf()->connectTo(nodeOut->getOtherHalf(), channel);
            channel->callInitialize();

            newlyConnected.push_back(node);
            newlyConnected.push_back(otherNode);
        }
        count++;

        // remove nodes from unconnectedNodes if enough neighbors
        for (auto node : newlyConnected) {
            int c = 0;
            for(int g=0; g<conns; g++) {
                cGate *nodeOut = getOutputGate(node, g);
                if(nodeOut->isConnected()) {
                    c++;
                }
            }

            if(c >= 4) {
                // unconnectedNodes.erase(std::remove(unconnectedNodes.begin(), unconnectedNodes.end(), node), unconnectedNodes.end());
            }
        }
        newlyConnected.clear();
    }

    // verify that at least 2 gates of every node are connected
    for (int i=0; i<n; i++) {
        cModule *node = this->getSubmodule("node", i);
        int count = 0;
        for(int g=0; g<conns; g++) {
            cGate *nodeOut = getOutputGate(node, g);
            if(nodeOut->isConnected()) {
                count++;
            }
        }

        if(count < 2) {
            throw cRuntimeError("Node %d has less than 2 connected gates (%d)!", i, count);
        }
    }


    // configure network
    double constant = this->getSubmodule("config")->par("constant");
    double aws =  this->getSubmodule("config")->par("aws");
    double azure = this->getSubmodule("config")->par("azure");
    double didata = this->getSubmodule("config")->par("didata");

    double minProcessingScale = this->getSubmodule("config")->par("minProcessingScale");
    double maxProcessingScale = this->getSubmodule("config")->par("maxProcessingScale");
    double minGenerationThreshold = this->getSubmodule("config")->par("minGenerationThreshold");

    int networkProcessingRate = this->getSubmodule("config")->par("networkProcessingRate");
    int networkGenerationRate = this->getSubmodule("config")->par("networkGenerationRate");
    double passiveGeneration = this->getSubmodule("config")->par("passiveGeneration");
    double topGeneration = this->getSubmodule("config")->par("topGeneration");
    int topGenerationRate = networkGenerationRate * 0.2 / (topGeneration * n);
    int normalGenerationRate = networkGenerationRate * 0.8 / ((1-passiveGeneration-topGeneration) * n);

    std::string processingRateFile;

    // configure every node
    for (int i=0; i<n; i++) {
        cModule *node = this->getSubmodule("node", i);

        // set processing rate file
        double r = uniform(0, 1);
        if(r < constant) {
            processingRateFile = "./inputs/constant.csv";
        } else if(r < constant+aws) {
            processingRateFile = "./inputs/aws.csv";
        } else if(r < constant+aws+azure) {
            processingRateFile = "./inputs/azure.csv";
        } else {
            processingRateFile = "./inputs/dimension_data.csv";
        }
        node->par("processingRateFile") = processingRateFile;

        // set processing scale
        double processingScale = uniform(minProcessingScale, maxProcessingScale);
        node->par("processingScale") = processingScale;
    }


    int generationRate;
    std::vector<cModule*> generationNodes;
    int totalGeneration = 0;
    for (int i=0; i<n; i++) {
        cModule *node = this->getSubmodule("node", i);
        double processingScale = node->par("processingScale");


        // avoid a weak node generating messages
        if(processingScale < minGenerationThreshold) {
            generationRate = 0;
        } else {
            double r = uniform(0, 1);
            if(r < passiveGeneration) {
               generationRate = 0;
           } else if(r < passiveGeneration+topGeneration) {
               generationRate = intuniform(topGenerationRate*0.9, topGenerationRate*1.1);
               generationNodes.push_back(node);
           } else {
               generationRate = normalGenerationRate;
               generationNodes.push_back(node);
           }
        }

        totalGeneration += generationRate;
        node->par("generationRate") = generationRate;
    }

    // pick random generation nodes to distribute the non-distributed generation
    while(totalGeneration <= networkGenerationRate) {
        int r = intuniform(0, generationNodes.size()-1);
        generationRate = generationNodes[r]->par("generationRate");
        generationNodes[r]->par("generationRate") = generationRate + 1;
        totalGeneration++;
    }

    // verify that totalGenerationRate = networkGenerationRate +- 0.1
    if(totalGeneration < networkGenerationRate*0.9) {
        throw cRuntimeError("totalGeneration (%d) is smaller than allowed networkGenerationRate(%d)*0.9=%d", totalGeneration, networkGenerationRate, (int)(networkGenerationRate*0.9));
    } else if(totalGeneration > networkGenerationRate*1.1) {
        throw cRuntimeError("totalGeneration (%d) is bigger than allowed networkGenerationRate*1.1=%d", totalGeneration, (int)(networkGenerationRate*1.1));
    }

    // log all information
    for (int i=0; i<n; i++) {
        cModule *node = this->getSubmodule("node", i);

        std::string f = node->par("processingRateFile");
        double ps = node->par("processingScale");
        int gr = node->par("generationRate");

        EV << "i: " << i << " file: " << f << " processing_scale: " << ps << " generation_rate: " << gr << "\n";
    }
}
