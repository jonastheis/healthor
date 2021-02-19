#include "v1.h"

void V1::initialize() {
    Basic::initialize();

    // initialize neighbors
    neighbors = new Neighbor[neighborsCount()];
    for(int i=0; i<neighborsCount(); i++) {
        neighbors[i].gate = i;

        // set to healthy by default
        neighbors[i].health = 1;
        neighbors[i].sendingRate = this->getParentModule()->getSubmodule("config")->par("networkProcessingRate");
        neighbors[i].maxOutboxSize = par("maxOutboxSize");
        neighbors[i].droppedMessages = 0;

        neighbors[i].outbox = new std::deque<std::string>();

        // create pacing event for every neighbor
        char eventName[5];
        sprintf(eventName, "p%d", i);
        PacingEvent *pacingEvent = new PacingEvent(eventName);
        pacingEvent->setNeighbor(i);
        scheduleAt(simTime()+0.1, pacingEvent);

        // add outvector for every outbox
        neighbors[i].outboxLengthVector = new cOutVector();
        cGate *g = gate("gate$i", i);
        char buf[100];
        sprintf(buf, "OutboxLength-%s", g->getPathStartGate()->getOwnerModule()->getFullPath().c_str());
        neighbors[i].outboxLengthVector->setName(buf);
        neighbors[i].outboxLengthVector->record(neighbors[i].outbox->size());

        // add outvector for sending rate per node
        neighbors[i].sendingRateVector = new cOutVector();
        sprintf(buf, "SendingRate-%s", g->getPathStartGate()->getOwnerModule()->getFullPath().c_str());
        neighbors[i].sendingRateVector->setName(buf);
        neighbors[i].sendingRateVector->record(neighbors[i].sendingRate);

        // add receiving rate stuff
        for(int k=0; k<DEFENSE_RW; k++) {
            neighbors[i].receivingRate[k] = k == 0 ? 0 : -1;
        }
        neighbors[i].receivingRateVector = new cOutVector();
        sprintf(buf, "ReceivingRate-%s", g->getPathStartGate()->getOwnerModule()->getFullPath().c_str());
        neighbors[i].receivingRateVector->setName(buf);
        neighbors[i].receivingRateVector->record(0);
    }

    // start health measurement engine, called every simulation second
    char eventName[10];
    sprintf(eventName, "h%d", getIndex());
    healthMeasurementEvent = new HealthMeasurementEvent(eventName);
    scheduleAt(simTime()+0.9, healthMeasurementEvent);


    // start defense engine, called every simulation second
    sprintf(eventName, "d%d", getIndex());
    defenseEvent = new DefenseEvent(eventName);
    scheduleAt(simTime()+1, defenseEvent);
    for(int k=0; k<DEFENSE_RW; k++) {
        allowedReceivingRate[k] = k == 0 ? computeRate(currentHealth) : -1;
    }

    healthVector.setName("Health");
    healthVector.record(currentHealth);
    allowedReceivingRateVector.setName("AllowedReceivingRate");

    // is this node an attacker?
    maliciousStartTime = par("maliciousStartTime");
    maliciousPacingRate = par("maliciousPacingRate");
    maliciousGateIndex = par("maliciousGateIndex");
    maliciousHealth = par("maliciousHealth");
}

void V1::finish() {
    Basic::finish();

    recordScalar("sentHealthMessages", sentHealthMessages);
    recordScalar("receivedHealthMessages", receivedHealthMessages);

    for(int i=0; i<neighborsCount(); i++) {
        cGate *g = gate("gate$i", i);

        char name[200];
        sprintf(name, "droppedMessages-%s", g->getPathStartGate()->getOwnerModule()->getFullPath().c_str());
        recordScalar(name, neighbors[i].droppedMessages);
    }
}



int V1::neighborsCount() {
    return gateSize("gate");
}


/**********************************************************************************
 *
 * message sending
 *
**********************************************************************************/

void V1::sendOnGate(cMessage *msg, int gate) {
    if(!this->gate("gate$o", gate)->isConnected()) {
        delete msg;
        return;
    }

    if(dynamic_cast<RequestMessage*>(msg)) {
        sentMessageRequests++;
    } else if(TangleMessage *tangleMsg = dynamic_cast<TangleMessage*>(msg)) {
        tangleMsg->getRequestMessageResponse() ? sentMessageRequestResponses++ : sentMessages++;
    } else if(dynamic_cast<HealthMessage*>(msg)) {
        sentHealthMessages++;
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

void V1::handleMessage(cMessage *msg) {
    // check for message type/event and start action accordingly

    if (msg == schedulingEvent) {
        onSchedulingEvent();
        return;
    }

    if(msg == generationEvent) {
        onGenerationEvent();
        return;
    }

    if(msg == healthMeasurementEvent) {
        onHealthMeasurementEvent();
        return;
    }

    if(msg == defenseEvent) {
        onDefenseEvent();
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

    if(PacingEvent *pacingEvent = dynamic_cast<PacingEvent*>(msg)) {
        onPacingEvent(pacingEvent);
        return;
    }

    if(SendingEvent *sendingEvent = dynamic_cast<SendingEvent*>(msg)) {
        onSendingEvent(sendingEvent);
        delete msg;
        return;
    }



    if(HealthMessage *healthMsg = dynamic_cast<HealthMessage*>(msg)) {
        receivedHealthMessages++;
        onHealthMessage(healthMsg);
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

        // count up received messages per neighbor, ignore self-messages and requested messages
        if (msg->getArrivalGate() != nullptr && !tangleMsg->getRequestMessageResponse()) {
            int n = msg->getArrivalGate()->getIndex();
            neighbors[n].receivingRate[0]++;
        }

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
 * defense engine
 *
**********************************************************************************/

double movingAverage(int *arr, int length) {
    int total = 0;
    int count = 0;

    for(int i=0; i<length; i++) {
        if(arr[i] >= 0) {
            total += arr[i];
            count++;
        }
    }

    return total / (double) count;
}

void V1::disableNeighbor(int neighbor) {
    // disable defense engine via configuration (useful for simualations)
    bool disableDefenseEngine = this->par("disableDefenseEngine");
    if(disableDefenseEngine) {
        return;
    }

    Basic* node = disconnectNeighbor(neighbor);
    node->par("outOfSync") = true;

    neighbors[neighbor].disabled = true;
}


void V1::onDefenseEvent() {
    // disable defense engine via configuration (useful for simualations)
    bool disableDefenseEngine = this->par("disableDefenseEngine");
    if(disableDefenseEngine) {
        return;
    }

    // do nothing if node is out of sync
    bool outOfSync = this->par("outOfSync");
    if(outOfSync) {
        return;
    }

    // defenseEvent is triggered every simulation second
    scheduleAt(simTime()+1, defenseEvent);

    // we want to get the average receiving rate of all neighbors.
    // if a node is below it too often we it might be unhealthy or malicious
    double avgReceivingRateNeighbors = 0;
    int totalOnlineNeighbors = 0;

    // calculate and check if neighbors exceed allowed receiving rate
    double allowedReceivingRateMovingAvg = movingAverage(allowedReceivingRate, DEFENSE_RW) * DEFENSE_RATE_GRACE;

    if(verboseLog) {
        // record allowed receiving rate
        allowedReceivingRateVector.record(allowedReceivingRateMovingAvg);
    }

    // verify for each neighbor if rate is exceeded
    for (int i=0; i<neighborsCount(); i++) {
        if(neighbors[i].disabled) {
            continue;
        }

        double neighborReceivingRateMovingAvg = movingAverage(neighbors[i].receivingRate, DEFENSE_RW);
        totalOnlineNeighbors++;

        // sum up receiving rates of all neighbors
        avgReceivingRateNeighbors += neighborReceivingRateMovingAvg;

        if(allowedReceivingRateMovingAvg < neighborReceivingRateMovingAvg) {
            neighbors[i].receivingRateExceeded++;
            EV_ERROR << "RATE exceeded: " << allowedReceivingRateMovingAvg << "-" << neighborReceivingRateMovingAvg << "\n";
        } else {
            neighbors[i].receivingRateExceeded = 0;
        }

        // if DEFENSE_RATE_EXCEEDED reached -> drop node
        if(neighbors[i].receivingRateExceeded >= DEFENSE_RATE_EXCEEDED) {
            // TODO: remove this throw, just to make sure that it works also in bigger experiments
//            throw cRuntimeError("Neighbor exceeded rate too often!!! need to drop!");
            EV_ERROR << "RATE exceeded too often! " << neighbors[i].receivingRateExceeded << "\n";

            // we need to "disable" this neighbor so we don't connect to it again
            disableNeighbor(i);
        }

        // record actual receiving rate of neighbors
        if(verboseLog) {
            neighbors[i].receivingRateVector->record(neighborReceivingRateMovingAvg);
        }
    }


    // calculate average receiving rate of all neighbors
    if(totalOnlineNeighbors != 0) {
        avgReceivingRateNeighbors = avgReceivingRateNeighbors / totalOnlineNeighbors * DEFENSE_RATE_SUBCEED_THRESHOLD;
    } else {
        avgReceivingRateNeighbors = 0;
    }

    // check that neighbors are not below the rate
    for (int i=0; i<neighborsCount(); i++) {
        if(neighbors[i].disabled) {
            continue;
        }

        double neighborReceivingRateMovingAvg = movingAverage(neighbors[i].receivingRate, DEFENSE_RW);

        if(neighborReceivingRateMovingAvg < avgReceivingRateNeighbors) {
            EV_ERROR << "RATE too low! " << neighborReceivingRateMovingAvg << "/" << avgReceivingRateNeighbors << "\n";
            neighbors[i].receivingRateSubceeded++;
        } else {
            neighbors[i].receivingRateSubceeded = 0;
        }

        // if DEFENSE_RATE_SUBCEEDED reached -> drop node
       if(neighbors[i].receivingRateSubceeded >= DEFENSE_RATE_SUBCEEDED) {
           // TODO: remove this throw, just to make sure that it works also in bigger experiments
//            throw cRuntimeError("Neighbor exceeded rate too often!!! need to drop!");
           EV_ERROR << "RATE subceeded too often! " << neighbors[i].receivingRateSubceeded << "\n";

           // we need to "disable" this neighbor so we don't connect to it again
           disableNeighbor(i);
       }
    }






    // rotate/reset receiving rate for all neighbors
    for (int i=0; i<neighborsCount(); i++) {
        neighbors[i].receivingRate[2] = neighbors[i].receivingRate[1];
        neighbors[i].receivingRate[1] = neighbors[i].receivingRate[0];
        neighbors[i].receivingRate[0] = 0;
    }

    // rotate/reset allowed receiving rate
    allowedReceivingRate[2] = allowedReceivingRate[1];
    allowedReceivingRate[1] = allowedReceivingRate[0];
    allowedReceivingRate[0] = computeRate(currentHealth);




    // check outbox occupation for each neighbor, if too high for too long -> drop
    for (int i=0; i<neighborsCount(); i++) {
        if(neighbors[i].disabled) {
            continue;
        }

        int outboxSize = neighbors[i].outbox->size();

        if(verboseLog) {
            // record length of outbox
            neighbors[i].outboxLengthVector->recordWithTimestamp(simTime(), outboxSize);
        }

        if(outboxSize > neighbors[i].maxOutboxSize * DEFENSE_HEALTH_GRACE) {
            neighbors[i].unhealthyCount++;
            EV_ERROR << "Outbox too full! " <<  neighbors[i].unhealthyCount << "\n";
        } else {
            neighbors[i].unhealthyCount = 0;
        }

        if(neighbors[i].unhealthyCount >= DEFENSE_HEALTH_COUNT) {
            // we need to "disable" this neighbor so we don't connect to it again
            EV_ERROR << "Outbox too full for too often! " <<  neighbors[i].unhealthyCount << "\n";
           disableNeighbor(i);
        }
    }





}

/**********************************************************************************
 *
 * end defense engine
 *
**********************************************************************************/


/**********************************************************************************
 *
 * health measurement engine
 *
**********************************************************************************/

void V1::onHealthMeasurementEvent() {
    // do nothing if node is out of sync
    bool outOfSync = this->par("outOfSync");
    if(outOfSync) {
        return;
    }

    // healthMeasurementEvent is triggered every simulation second
    scheduleAt(simTime()+1, healthMeasurementEvent);

    // calculate health within [0, 1]
    int currentInboxLength = inbox.size()+solidifier.bufferSize();
    if (currentInboxLength < networkProcessingRate) {
        // if healthy, scale by normalized currentProcessingRate
        currentHealth = 1 * (currentProcessingRate / (double)networkProcessingRate);
        // currentHealth = 1; // this has to be uncommented for Luigi's proposal
    } else {
        currentHealth = 1 - (currentInboxLength / (double)maxInboxSize);
    }



    if(verboseLog) {
        // track health of node over time
        healthVector.record(currentHealth);
    }

    // send to neighbors
    HealthMessage *msg = new HealthMessage();
    msg->setHealth(currentHealth);

    // malicious nodes might lie about their health
    if(maliciousHealth > -1 && maliciousStartTime <= simTime()) {
        msg->setHealth(maliciousHealth);
    }


    for (int i=0; i<neighborsCount(); i++) {
       sendCopyOf(msg, i);
    }

    delete msg;
}

/**********************************************************************************
 *
 * end health measurement engine
 *
**********************************************************************************/



/**********************************************************************************
 *
 * rate computation engine
 *
**********************************************************************************/

int V1::computeRate(double health) {
    if(cappedProcessingRate) {
        return int(networkProcessingRate * health);
    } else {
        // FFA for Healthor
        // Luigi's proposal: sends at current processing rate of the node
        return int(currentProcessingRate * health);
    }
}

void V1::onHealthMessage(HealthMessage *healthMsg) {
    // do nothing if node is out of sync
    bool outOfSync = this->par("outOfSync");
    if(outOfSync) {
        return;
    }

    int n = healthMsg->getArrivalGate()->getIndex();
    neighbors[n].health = healthMsg->getHealth();

    // convert to sending rate
    neighbors[n].sendingRate = computeRate(neighbors[n].health);

    EV_DETAIL << "Neighbor: " << n << " - " << neighbors[n].health << "|" << neighbors[n].sendingRate << "\n";

    delete healthMsg;

    if(verboseLog) {
        // record sending rate per neighbor
        neighbors[n].sendingRateVector->record(neighbors[n].sendingRate);
    }
}

/**********************************************************************************
 *
 * end rate computation engine
 *
**********************************************************************************/



/**********************************************************************************
 *
 * pacing engine
 *
**********************************************************************************/

void V1::onPacingEvent(PacingEvent *pacingEvent) {
    // do nothing if node is out of sync
    bool outOfSync = this->par("outOfSync");
    if(outOfSync) {
        return;
    }

    // pacing event is triggered every simulation second
    scheduleAt(simTime()+1, pacingEvent);

    Neighbor n = neighbors[pacingEvent->getNeighbor()];

    int sendingRate = n.sendingRate;
    // if > 0 then this nodes acts malicious from maliciousStartTime
    if(maliciousPacingRate > -1 && maliciousStartTime <= simTime()) {
        if(maliciousGateIndex == -1 || maliciousGateIndex == n.gate) {
            sendingRate = maliciousPacingRate;
            EV_ERROR << "Malicious Pacing: " << n.gate << " - rate: " << sendingRate << "\n";
        }
    }

    EV_DETAIL << "Pacing: " << n.gate << " - rate: " << sendingRate << "\n";

    if(sendingRate > 0) {
        for(int i=0; i<sendingRate; i++) {
            double t = i / (double)sendingRate;

            SendingEvent *sendingEvent = new SendingEvent();
            sendingEvent->setNeighbor(pacingEvent->getNeighbor());

            scheduleAt(simTime() + t, sendingEvent);
        }
    }
}

void V1::onSendingEvent(SendingEvent *sendingEvent) {
    Neighbor n = neighbors[sendingEvent->getNeighbor()];

//    if(verboseLog) {
//        // record length of outbox
//        n.outboxLengthVector->recordWithTimestamp(simTime(), n.outbox->size());
//    }

    if(n.outbox->empty()) {
        return;
    }

    // get message from outbox
    std::string msgId = n.outbox->front();
    n.outbox->pop_front();
    LocalTangleMessage *local = tangle.getMessage(msgId.c_str());

    TangleMessage *msg = TangleMessageFromLocal(local);
    sendOnGate(msg, n.gate);
    delete local;
}

/**********************************************************************************
 *
 * end pacing engine
 *
**********************************************************************************/



/**********************************************************************************
 *
 * message scheduling / processing
 *
**********************************************************************************/

void V1::calculateCurrentProcessingRate() {
    int64_t time = simTime().trunc(SIMTIME_S).raw() / 1000000000000;
    time = (time + processingPatternShift) % processingRate.size();
    currentProcessingRate = networkProcessingRate * processingScale * processingRate[time];
}

void V1::enqueueMessageToOutboxes(LocalTangleMessage *msg) {
    EV_DETAIL << "Enqueueing message: " << msg->id << "\n";

    // arrivalGate == -1 means it was generated on this node -> send to all
    for (int i=0; i<neighborsCount(); i++) {
        if(msg->arrivalGateIndex == -1 || msg->arrivalGateIndex != i) {
            // check whether outbox is full
            // an outbox size <=0 means unlimited outbox size
            if(neighbors[i].maxOutboxSize > 0 && (neighbors[i].outbox->size() >= neighbors[i].maxOutboxSize)) {
                neighbors[i].droppedMessages++;

                // tail drop
                EV << "neighbor["<<i<<"] is full. Dropping " << msg->id << "\n";
                continue;

                // head drop
//                TangleMessage* msgToDrop = (TangleMessage*) neighbors[i].outbox->get(0); // get message at head
//                neighbors[i].outbox->remove(msgToDrop);
//                EV << "neighbor["<<i<<"] is full. Dropping " << msgToDrop->getId() << "\n";

                // random drop
//                int index = rand() % neighbors[i].outbox->getLength();
//                TangleMessage* msgToDrop = (TangleMessage*) neighbors[i].outbox->get(index);
//                neighbors[i].outbox->remove(msgToDrop);
//                EV << "neighbor["<<i<<"] is full. Dropping " << msgToDrop->getId() << "\n";
            }

            // put into outbox
            neighbors[i].outbox->push_back(msg->id);
        }
    }
}


void V1::onProcessingEvent() {
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
        // send to all neighbors (via pacing engine) by enqueuing to outbox
        enqueueMessageToOutboxes(local);
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
