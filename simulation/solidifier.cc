#include "solidifier.h"

Solidifier::Solidifier() {
}

Solidifier::Solidifier(Tangle* tangle) {
    this->tangle = tangle;
}

/**
 * Gets the size of the solidification buffer,
 * i.e., the count of messages that are currently not solid.
 */
int Solidifier::bufferSize() const {
    return buffer.size();
}

LocalTangleMessage* Solidifier::dropFromTail() {
    LocalTangleMessage* msgToDrop = NULL;

    for(int i=bufferQueue.size()-1; i<0; i--) {
        LocalTangleMessage* msg = tangle->getMessage(bufferQueue.at(i).c_str());
        if(!msg->requestMessageResponse) {
            msgToDrop = msg;
            break;
        }
        delete msg;
    }

    if (msgToDrop == NULL) {
        return NULL;
    }

    bufferQueue.erase(std::remove(bufferQueue.begin(), bufferQueue.end(), msgToDrop->id), bufferQueue.end());
    buffer.erase(msgToDrop->id);

    return msgToDrop;
}

//TangleMessage* Solidifier::dropFromHead() {
//    TangleMessage* msgToDrop = NULL;
//
//    for(int i=0; i<bufferQueue.getLength(); i++) {
//        TangleMessage* msg = (TangleMessage*) bufferQueue.get(i);
//        if(!msg->getRequestMessageResponse()) {
//            msgToDrop = msg;
//            break;
//        }
//    }
//
//    if (msgToDrop == NULL) {
//        return NULL;
//    }
//
//    bufferQueue.remove(msgToDrop);
//    buffer.erase(msgToDrop->getId());
//
//    return msgToDrop;
//}
//
//TangleMessage* Solidifier::dropRandom() {
//    TangleMessage* msgToDrop = NULL;
//
//    while(1) {
//        int index = rand() % bufferQueue.getLength();
//        TangleMessage* msg = (TangleMessage*) bufferQueue.get(index);
//        if(!msg->getRequestMessageResponse()) {
//            msgToDrop = msg;
//            break;
//        }
//    }
//
//    bufferQueue.remove(msgToDrop);
//    buffer.erase(msgToDrop->getId());
//
//    return msgToDrop;
//}


/**
 * Gets the count of the outstanding message requests.
 */
int Solidifier::outstandingRequests() const {
    return requests.size();
}


bool Solidifier::isOutstandingRequest(const char* msgId) {
    return requests.count(msgId) == 1;
}

void Solidifier::addMessageToRequests(std::vector<std::string>* outstandingRequests, const char* msgId) {
    // only request msg/parent if:
    // 1. not solid and not in solidification buffer
    // 2. not already requested
    if (!tangle->isParentSolid(msgId) && buffer.count(msgId) == 0 && requests.count(msgId) == 0) {
        requests[msgId] = true;
        outstandingRequests->push_back(msgId);
    }
}

/**
 * Handles an unsolid messages.
 * 1. Puts it into the solidification buffer
 * 2. Creates message requests for missing parents
 */
std::vector<std::string> Solidifier::handleUnsolidMessage(LocalTangleMessage *msg) {
    std::vector<std::string> outstandingRequests;

    // add to solidification buffer
    buffer[msg->id] = msg;
    bufferQueue.push_back(msg->id);

    // remove from outstanding message requests.
    // we have the message now in the solidification buffer
    requests.erase(msg->id);

    // request parents
    addMessageToRequests(&outstandingRequests, msg->parent1);
    addMessageToRequests(&outstandingRequests, msg->parent2);

    return outstandingRequests;
}

/**
 * Handles a solid message.
 * A message can mean one or multiple messages getting solid.
 */
SolidificationResult Solidifier::handleSolidMessage(LocalTangleMessage *msg) {
    SolidificationResult res;
    std::map<std::string, bool> solidMessages;

    // solidify recursively starting with the received message
    solidify(&solidMessages, &res.outstandingRequests, msg);

    // do not return message that just got solid
    solidMessages.erase(msg->id);

    // create list from map
    for ( std::map<std::string, bool>::iterator it = solidMessages.begin(); it != solidMessages.end(); it++ ) {
        std::string solidMsg = it->first;
        res.solidMessages.push_back(solidMsg);
    }

    return res;
}

/**
 * Recursively solidifies messages starting from msg.
 */
void Solidifier::solidify(std::map<std::string, bool>* solidMessages, std::vector<std::string>* outstandingRequests, LocalTangleMessage* msg) {
    // end recursion if message is not solid
    if(!tangle->checkMessageSolidity(msg)) {
        // check whether parents need to be requested again
        addMessageToRequests(outstandingRequests, msg->parent1);
        addMessageToRequests(outstandingRequests, msg->parent2);

        return;
    }

    // only continue if we didn't walk this part already
    if(solidMessages->count(msg->id) > 0) {
        return;
    }

    solidMessages->insert(std::make_pair(msg->id, true));

    // erase from buffer and requests (if in list)
    buffer.erase(msg->id);
    bufferQueue.erase(std::remove(bufferQueue.begin(), bufferQueue.end(), msg->id), bufferQueue.end());

    requests.erase(msg->id);

    // solidify recursively by walking into future cone of message and checking for solidification
    Approvers *approvers = tangle->getApprovers(msg->id);
    if(approvers != NULL) {
        for(auto a : approvers->a) {
            if(buffer.count(a) > 0) {
                LocalTangleMessage* approverMsg = tangle->getMessage(a.c_str());
                solidify(solidMessages, outstandingRequests, approverMsg);
                delete approverMsg;
            }
        }
        delete approvers;
    }
}
