#include "basic.h"

void Basic::initialize() {
    verboseLog = par("verboseLog");

    // set parameters
    maxInboxSize = par("maxInboxSize");
    networkProcessingRate = this->getParentModule()->getSubmodule("config")->par("networkProcessingRate");
    cappedProcessingRate = par("cappedProcessingRate");

    processingScale = par("processingScale");
    generationRate = par("generationRate");

    // set processing rate
    CSVReader reader = CSVReader(par("processingRateFile"));
    processingRate = reader.readRate();
    reader.close();
    processingPatternShift = rand();
    calculateCurrentProcessingRate();

    // set generation rate
    calculateCurrentGenerationRate();

    // start generation event, called every simulation second
    char generationEventName[10];
    sprintf(generationEventName, "g%d", getIndex());
    generationEvent = new GenerationEvent(generationEventName);
    scheduleAt(simTime(), generationEvent);

    // start scheduler, called every simulation second
    char eventName[10];
    sprintf(eventName, "s%d", getIndex());
    schedulingEvent = new SchedulingEvent(eventName);
    scheduleAt(simTime(), schedulingEvent);

    int nodeId = getIndex();
    tangle = Tangle(nodeId);
    solidifier = Solidifier(&tangle);

    // prepare collection of statistics
    WATCH(generatedMessages);
    WATCH(processedMessages);
    WATCH(droppedMessages);

    WATCH(sentMessages);
    WATCH(sentMessageRequests);
    WATCH(sentMessageRequestResponses);

    WATCH(receivedMessages);
    WATCH(receivedMessageRequests);
    WATCH(receivedMessageRequestResponses);

    inboxLengthVector.setName("InboxLength");
    inboxLengthVector.record(inbox.size());
    totalInboxVector.setName("TotalInbox");
    totalInboxVector.record(inbox.size());

    availableProcessingRateVector.setName("AvailableProcessingRate");
    solidificationBuffer.setName("SolidificationBuffer");
    outstandingMessageRequests.setName("OutstandingMessageRequests");
}


void Basic::refreshDisplay() const {
    char buf[40];
    sprintf(buf, "G: %d, P: %d, Q: %ld/%d, S: %d/%d", currentGenerationRate, currentProcessingRate, inbox.size(), maxInboxSize, solidifier.bufferSize(), solidifier.outstandingRequests());
    getDisplayString().setTagArg("t", 0, buf);
}

void Basic::finish() {
    std::string configName = par("configName");
    int nodeId = getIndex();

    // get generated messages
    char fileNameGenerated[100];
    sprintf(fileNameGenerated, "./results/%s-generated_%d.csv", configName.c_str(), getIndex());
    CSVWriter *writerGenerated = new CSVWriter(fileNameGenerated, "msgID,time");

    // get processed messages
    char fileNameProcessed[100];
    sprintf(fileNameProcessed, "./results/%s-processed_%d.csv", configName.c_str(), getIndex());
    CSVWriter *writerProcessed = new CSVWriter(fileNameProcessed, "msgID,time");

    int countInTangle = 0;
    int* countInTangleP = &countInTangle;
    int countSolid = 0;
    int* countSolidP = &countSolid;
    std::function<void(LocalTangleMessage*)> callback = [writerGenerated, nodeId, writerProcessed, countInTangleP, countSolidP](LocalTangleMessage* msg) mutable {
        // get generated
        if(msg->issuerNode == nodeId) {
            std::string row = msg->id + std::string(",") + std::to_string(msg->issuingTime);
            writerGenerated->addRow(row);
        }
        // get processed
        if(msg->solid) {
            std::string row = msg->id + std::string(",") + std::to_string(msg->processingTime);
            writerProcessed->addRow(row);
            (*countSolidP)++;
        }
        (*countInTangleP)++;
    };
    tangle.iterateTangle(callback);

    writerGenerated->close();
    writerProcessed->close();

    // verify numbers
    EV << "Tangle: " << countInTangle << " - Solid: " << countSolid << "\n";

    // record scalars
    recordScalar("generatedMessages", generatedMessages);
    recordScalar("droppedMessages", droppedMessages);
    recordScalar("processedMessages", processedMessages);

    recordScalar("sentMessages", sentMessages);
    recordScalar("sentMessageRequests", sentMessageRequests);
    recordScalar("sentMessageRequestResponses", sentMessageRequestResponses);

    recordScalar("receivedMessages", receivedMessages);
    recordScalar("receivedMessageRequests", receivedMessageRequests);
    recordScalar("receivedMessageRequestResponses", receivedMessageRequestResponses);


    recordScalar("generationRate", generationRate);
    recordScalar("processingScale", processingScale);

    std::string processingRateFile = par("processingRateFile");
    int processingRateFileNumber = 0;
    if(processingRateFile == "./inputs/aws.csv") {
        processingRateFileNumber = 1;
    } else if(processingRateFile == "./inputs/azure.csv") {
        processingRateFileNumber = 2;
    } else if(processingRateFile == "./inputs/dimension_data.csv") {
        processingRateFileNumber = 3;
    }

    recordScalar("processingRateFile", processingRateFileNumber);
}

Basic::~Basic() {
//    inbox->clear();

//    std::map<std::string, TangleMessage*> knownMessages = tangle.getKnownMessages();

//    for ( std::map<std::string, TangleMessage*>::iterator it = knownMessages.begin(); it != knownMessages.end(); it++ ) {
//        TangleMessage* msg = it->second;
//        this->cancelAndDelete(msg);
//    }

//    this->cancelAndDelete(generationEvent);
//    this->cancelAndDelete(schedulingEvent);

//    delete inbox;
}

/**********************************************************************************
 *
 * node peering
 *
**********************************************************************************/

std::map<int, bool> Basic::getNeighbors() {
    std::map<int, bool> neighbors = std::map<int,bool>();

    for(int i=0; i<gateSize("gate"); i++) {
        cGate *g = this->gate("gate$i", i);
        cGate *gStart = g->getPathStartGate();
        cGate *gStartOther = gStart->getOtherHalf()->getPathStartGate();
        cModule *gStartModule = gStart->getOwnerModule();
        cModule *gStartOtherModule = gStartOther->getOwnerModule();

        if(gStartModule->getIndex() != this->getIndex()) {
            neighbors[gStartModule->getIndex()] = true;
        }
        if(gStartOtherModule->getIndex() != this->getIndex()) {
            neighbors[gStartOtherModule->getIndex()] = true;
        }
    }

    return neighbors;
}


void Basic::goOutofSync() {
    this->par("outOfSync") = true;

    // disconnect from all neighbors
    for (int i=0; i<gateSize("gate"); i++) {
        Basic* node = disconnectNeighbor(i);

        // make neighbors connect to other neighbor
        node->connectNeighbor();
    }
}

Basic* Basic::disconnectNeighbor(int gateId) {
    cGate *g = getInputGate(this, gateId);
    cGate *gStart = g->getPathStartGate();
    cGate *gStartOther = gStart->getOtherHalf()->getPathStartGate();

    // disconnect both sides, needs to be called from source gate: https://doc.omnetpp.org/omnetpp/manual/#sec:simple-modules:removing-connections
    gStart->disconnect();
    gStartOther->disconnect();

//    for(int i=0; i<gateSize("gate"); i++) {
//        cGate *g = this->gate("gate$i", i);
//        EV << i << ": " << g->getOwner()->getFullName() << ", in: " << g->isConnected() << ", out: " << g->getOtherHalf()->isConnected() << "\n";
//    }

    cModule *gStartModule = gStart->getOwnerModule();
    cModule *gStartOtherModule = gStartOther->getOwnerModule();

    if(gStartModule->getIndex() != this->getIndex()) {
        return check_and_cast<Basic*>(gStartModule);
    }
    return check_and_cast<Basic*>(gStartOtherModule);
}

void Basic::connectNeighbor() {
    // needed for direct method call between modules: https://doc.omnetpp.org/omnetpp/manual/#sec:simple-modules:direct-method-calls
    Enter_Method("connectNeighbor()");

    // get all nodes that have a free gate
   std::vector<cModule*> unconnectedNodes;
   cModule *parentModule = this->getParentModule();
   int n = parentModule->getSubmodule("config")->par("n");
   int conns = parentModule->getSubmodule("config")->par("conns");
   int nodeIndex = getIndex();
   cChannelType *channelType = cChannelType::get("VolatileChannel");

   std::map<int, bool> neighbors = getNeighbors();

   for (int i=0; i<n; i++) {
       // skip the node itself
       if(i == nodeIndex) {
           continue;
       }

       cModule *node = parentModule->getSubmodule("node", i);


       // ignore nodes to which this node is already connected
       if(neighbors.count(i) > 0) {
           continue;
       }

       if(node == nullptr) {
           throw cRuntimeError("Node with ID=%d could not be found in network (parent module).", i);
       }

       // do not consider node if it is out of sync
       bool outOfSync = node->par("outOfSync");
       if(outOfSync) {
           continue;
       }

       for(int g=0; g<conns; g++) {
           cGate *nodeOut = getOutputGate(node, g);
           if(!nodeOut->isConnected()) {
               unconnectedNodes.push_back(node);
               break;
           }
       }
   }

   // abort if no unconnected node
   if(unconnectedNodes.size() < 1) {
       return;
   }

   // get random node
   int r = intuniform(0, unconnectedNodes.size()-1);

   // connect to node
   // get first free gate of current node
   cGate *nodeOut = getFirstUnconnectedOutputGate(this);

   // get first free gate of other node
   cGate *otherNodeIn = getFirstUnconnectedInputGate(unconnectedNodes[r]);

   EV_DEBUG << "NodeOut(" << this->getIndex() << "):" << nodeOut->getFullName()  << "\n";
   EV_DEBUG << "OtherNodeIn(" << unconnectedNodes[r]->getIndex() << "): " << otherNodeIn->getFullName() << "\n";

   // connect nodes' via in and out
   cChannel *channel = channelType->create("channel");
   nodeOut->connectTo(otherNodeIn, channel);
   // for some reason the channel is sometimes already initialized. Only do it if not initialized.
   if(!channel->initialized()) {
       channel->callInitialize();
   }

   channel = channelType->create("channel");
   otherNodeIn->getOtherHalf()->connectTo(nodeOut->getOtherHalf(), channel);
   // for some reason the channel is sometimes already initialized. Only do it if not initialized.
   if(!channel->initialized()) {
      channel->callInitialize();
  }
}

/**********************************************************************************
 *
 * end node peering
 *
**********************************************************************************/


/**********************************************************************************
 *
 * message sending
 *
**********************************************************************************/

void Basic::broadcastMessage(cMessage *msg, int arrivalGate) {
    // arrivalGate == -1 means it was generated on this node -> send to all

    EV << "Broadcast message: " << msg->getName() << "\n";

    for (int i=0; i<gateSize("gate"); i++) {
        if(arrivalGate == -1 || arrivalGate != i) {
            EV_DETAIL << "Forwarding message " << msg << " on gate[" << i << "]\n";
            sendCopyOf(msg, i);
        }
    }
}

void Basic::sendCopyOf(cMessage *msg, int gate) {
    // Duplicate message and send the copy.
    cMessage *copy = (cMessage *)msg->dup();
    sendOnGate(copy, gate);
}


void Basic::sendOnGate(cMessage *msg, int gate) {
    if(!this->gate("gate$o", gate)->isConnected()) {
        delete msg;
        return;
    }

    if(dynamic_cast<RequestMessage*>(msg)) {
        sentMessageRequests++;
    } else if(TangleMessage *tangleMsg = dynamic_cast<TangleMessage*>(msg)) {
        tangleMsg->getRequestMessageResponse() ? sentMessageRequestResponses++ : sentMessages++;
    }

    send(msg, "gate$o", gate);
}

/**********************************************************************************
 *
 * end message sending
 *
**********************************************************************************/



/**********************************************************************************
 *
 * message dispatching
 *
**********************************************************************************/

void Basic::handleMessage(cMessage *msg) {
    // check for message type/event and start action accordingly

    if (msg == schedulingEvent) {
        onSchedulingEvent();
        return;
    }


    if(msg == generationEvent) {
        onGenerationEvent();
        return;
    }


    if(ProcessingEvent *processingEvent = dynamic_cast<ProcessingEvent*>(msg)) {
        onProcessingEvent();
        delete msg;
        return;
    }

    if(CreateMessageEvent *createMessageEvent = dynamic_cast<CreateMessageEvent*>(msg)) {
        onCreateMessageEvent();
        delete msg;
        return;
    }

    if(RequestMessageEvent *reqMsgEvent = dynamic_cast<RequestMessageEvent*>(msg)) {
        onCreateMessageRequestEvent(reqMsgEvent);
        return;
    }



    if(RequestMessage *reqMsg = dynamic_cast<RequestMessage*>(msg)) {
        receivedMessageRequests++;
        onRequestMessage(reqMsg);
        delete msg;
        return;
    }

    if(TangleMessage *tangleMsg = dynamic_cast<TangleMessage*>(msg)) {
        tangleMsg->getRequestMessageResponse() ? receivedMessageRequestResponses++ : receivedMessages++;

        onTangleMessage(tangleMsg);
        return;
    }

    // Log messages that are not identified by any of the cases before.
    EV_WARN << "Received unknown " << msg->getName() << ". Doing nothing." << "\n";
    delete msg;
}

/**********************************************************************************
 *
 * end message dispatching
 *
**********************************************************************************/






/**********************************************************************************
 *
 * message creation
 *
**********************************************************************************/

void Basic::calculateCurrentGenerationRate() {
    if(generationRate == 0) {
        currentGenerationRate = 0;
        return;
    }
    currentGenerationRate =  poisson(generationRate);
}

void Basic::onGenerationEvent() {
    // do nothing if node is out of sync
    bool outOfSync = this->par("outOfSync");
    if(outOfSync) {
        return;
    }

    // generation event is triggered every simulation second
    scheduleAt(simTime()+1, generationEvent);

    calculateCurrentGenerationRate();
    EV_DETAIL << "Generation rate " << currentGenerationRate << "\n";

    generateMessages();
}

void Basic::generateMessages() {
    for(int i=0; i<currentGenerationRate; i++) {
        double t = i / (double)currentGenerationRate;

        // schedule message creation event at specific time
        scheduleAt(simTime() + t, new CreateMessageEvent());
    }
}

void Basic::onCreateMessageEvent() {
    LocalTangleMessage *local = tangle.issueMessage();

    generatedMessages++;

    TangleMessage *msg = TangleMessageFromLocal(local);
    delete local;

    scheduleAt(simTime(), msg);
}

/**********************************************************************************
 *
 * end message creation
 *
**********************************************************************************/


/**********************************************************************************
 *
 * message scheduling / processing
 *
**********************************************************************************/

void Basic::calculateCurrentProcessingRate() {
    int64_t time = simTime().trunc(SIMTIME_S).raw() / 1000000000000;
    time = (time + processingPatternShift) % processingRate.size();
    if(cappedProcessingRate) {
        currentProcessingRate = std::min(networkProcessingRate * processingScale * processingRate[time], (double)networkProcessingRate);
    } else {
        currentProcessingRate = networkProcessingRate * processingScale * processingRate[time];
    }
}

void Basic::onSchedulingEvent() {
    // do nothing if node is out of sync
    bool outOfSync = this->par("outOfSync");
    if(outOfSync) {
        return;
    }

    // scheduling event is triggered every simulation second
    scheduleAt(simTime()+1, schedulingEvent);

    // inbox==0 && solidification_buffer==full -> node is out of sync
    if(inbox.empty() && solidifier.bufferSize() >= maxInboxSize) {
        goOutofSync();
        return;
    }

    calculateCurrentProcessingRate();
    EV_DETAIL << "Processing rate " << currentProcessingRate << "\n";

    if(verboseLog) {
        availableProcessingRateVector.record(currentProcessingRate);
    }

    scheduleMessages();
}

void Basic::scheduleMessages() {

    for(int i=0; i<currentProcessingRate; i++) {
        double t = i / (double)currentProcessingRate;

        scheduleAt(simTime() + t, new ProcessingEvent());
    }
}

void Basic::onProcessingEvent() {
    if(verboseLog) {
        inboxLengthVector.recordWithTimestamp(simTime(), inbox.size());
        totalInboxVector.recordWithTimestamp(simTime(), inbox.size() + solidifier.bufferSize());
        // record length of solidification buffer and outstanding requests
        solidificationBuffer.recordWithTimestamp(simTime(), solidifier.bufferSize());
        outstandingMessageRequests.recordWithTimestamp(simTime(), solidifier.outstandingRequests());
    }
    // nothing to do if inbox is empty
    if(inbox.empty()) {
        return;
    }

    // get message from inbox
    LocalTangleMessage *local = inbox.front();
    inbox.pop_front();

    // do not gossip requested messages: they are known to neighbors already
    if(!local->requestMessageResponse) {
        // gossip to all neighbors
        TangleMessage *msg = TangleMessageFromLocal(local);
        broadcastMessage(msg, local->arrivalGateIndex);
        delete msg;
    }

    // add to local ledger
    tangle.add(local);

    // process message: in the simulation we just count it up
    processedMessages++;


    // once a message becomes solid the solidification buffer needs to be checked again.
    // messages that have been waiting for it might've become solid now.
    SolidificationResult res = solidifier.handleSolidMessage(local);

    // add newly solid messages to inbox
    for (auto solidMsgId : res.solidMessages) {
        EV << "Pushing " << solidMsgId << " to inbox." << "\n";
        LocalTangleMessage *solidMsg = tangle.getMessage(solidMsgId.c_str());
        inbox.push_back(solidMsg);
    }

    // request missing messages
    for (auto parentId : res.outstandingRequests) {
        char name[50];
        sprintf(name, "requestEvent-%s", parentId.c_str());
        RequestMessageEvent* reqMsgEvent = new RequestMessageEvent(name);
        reqMsgEvent->setRequestId(parentId.c_str());

        scheduleAt(simTime(), reqMsgEvent);
    }

    delete local;
}

/**********************************************************************************
 *
 * end message scheduling
 *
**********************************************************************************/


/**********************************************************************************
 *
 * receipt/processing/solidification of tangle messages and message requests
 *
**********************************************************************************/

void Basic::onRequestMessage(RequestMessage *msg) {
    cGate* arrivalGate = msg->getArrivalGate();
    EV_DETAIL << "Message request " << msg->getName() << " received on gate " << arrivalGate->getIndex() << "." << "\n";

    // send message back to requesting node with flag set requestMessageResponse=true
    LocalTangleMessage* local = tangle.getMessage(msg->getRequestId());
    if(local != NULL) {
        TangleMessage *copy = TangleMessageFromLocal(local);
        copy->setRequestMessageResponse(true);
        sendOnGate(copy, arrivalGate->getIndex());
        delete local;
    }
}

void Basic::onTangleMessage(TangleMessage *msg) {
    // filter already seen messages
    if (tangle.isSeen(msg->getId())) {
        EV_DETAIL << "Already processed " << msg->getId() << ". Not queuing to inbox." << "\n";
        delete msg;
        return;
    }

    LocalTangleMessage *local = LocalFromTangleMessage(msg);
    delete msg;

    // check whether inbox+solidification buffer is full
    // an inbox size <=0 means unlimited inbox size
    if(maxInboxSize > 0 && (inbox.size()+solidifier.bufferSize()) >= maxInboxSize) {
        if (dropPolicy(local)) {
            return;
        }
    }

    // mark message as already seen
    tangle.seen(local->id);

    // keep track of all messages that are currently on a node.
    // either in the tangle, inbox or solidification buffer
    tangle.addKnownMessage(local);

    // handle unsolid messages
    if(!tangle.checkMessageSolidity(local)) {
        handleUnsolidMessage(local);

        // message is not put into inbox since only solid messages are put to the inbox.
        return;
    }


    // a solid message is inserted directly into the inbox
    inbox.push_back(local);
}

bool Basic::dropPolicy(LocalTangleMessage *msg) {
    // tail drop

    // prioritize solid messages and message request responses
    if(solidifier.bufferSize()>0 && (tangle.checkMessageSolidity(msg) || msg->requestMessageResponse)) {
        // drop from solidification buffer so that solid message can be put into inbox
        LocalTangleMessage *msgToDrop = solidifier.dropFromTail();

        // drop message from solidification buffer (if there is any better candidate to drop)
        if(msgToDrop != NULL) {
            dropMessage(msgToDrop);
            return false;
        }
    }

    dropMessage(msg);
    return true;


//     head drop
//    TangleMessage *msgToDrop = solidifier.dropFromHead();
//    dropMessage(msgToDrop);
//    return false;


    // random drop
//    TangleMessage *msgToDrop = solidifier.dropRandom();
//    dropMessage(msgToDrop);
//    return false;

}

void Basic::dropMessage(LocalTangleMessage *msg) {
    droppedMessages++;
    EV << "Inbox is full. Dropping " << msg->id << "\n";
    tangle.deleteSeen(msg->id);
    tangle.removeKnownMessage(msg);
}

void Basic::handleUnsolidMessage(LocalTangleMessage* msg) {
    EV_DETAIL << msg->id << " is not solid solid. Put on solidification buffer." << "\n";
    std::vector<std::string> v = solidifier.handleUnsolidMessage(msg);
    delete msg;

    // send request for missing messages
    for(auto parentId : v) {
        EV_DETAIL << msg->id << " is not solid solid: request parent " << parentId << " from neighbors." << "\n";
        char name[50];
        sprintf(name, "requestEvent-%s", parentId.c_str());
        RequestMessageEvent* reqMsgEvent = new RequestMessageEvent(name);
        reqMsgEvent->setRequestId(parentId.c_str());

        scheduleAt(simTime()+1, reqMsgEvent);
    }

    if(verboseLog) {
        // record length of solidification buffer and outstanding requests
        solidificationBuffer.recordWithTimestamp(simTime(), solidifier.bufferSize());
        outstandingMessageRequests.recordWithTimestamp(simTime(), solidifier.outstandingRequests());
    }
}

void Basic::onCreateMessageRequestEvent(RequestMessageEvent *event) {
    const char *msgId = event->getRequestId();

    if(!solidifier.isOutstandingRequest(msgId) || tangle.isSeen(msgId)) {
        delete event;
        return;
    }

    char name[50];
    sprintf(name, "request-%s", msgId);
    RequestMessage* reqMsg = new RequestMessage(name);
    reqMsg->setRequestId(msgId);
    broadcastMessage(reqMsg, -1);
    delete reqMsg;

    delete event;
    // TODO: there is an issue with scheduling this multiple times. Maybe it gets added too often to the solidification buffer?
//    scheduleAt(simTime() + 1.0, event);
}

/**********************************************************************************
 *
 * end receipt/processing/solidification of tangle messages and message requests
 *
**********************************************************************************/
