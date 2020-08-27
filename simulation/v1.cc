#include "v1.h"

void V1::initialize() {
    Basic::initialize();

    // initialize neighbors
    neighbors = new Neighbor[gateSize("gate")];
    for(int i=0; i<neighborsCount(); i++) {
        neighbors[i].gate = i;

        // set to healthy by default
        neighbors[i].health = 1;
        neighbors[i].sendingRate = par("networkProcessingRate");
        neighbors[i].maxOutboxSize = par("maxOutboxSize");

        neighbors[i].outbox = new cQueue();
        WATCH_PTR(neighbors[i].outbox);

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
        neighbors[i].outboxLengthVector->record(neighbors[i].outbox->getLength());

        // add outvector for sending rate per node
        neighbors[i].sendingRateVector = new cOutVector();
        sprintf(buf, "SendingRate-%s", g->getPathStartGate()->getOwnerModule()->getFullPath().c_str());
        neighbors[i].sendingRateVector->setName(buf);
        neighbors[i].sendingRateVector->record(neighbors[i].sendingRate);
    }

    // start health measurement engine, called every simulation second
    char eventName[5];
    sprintf(eventName, "h%d", getIndex());
    healthMeasurementEvent = new HealthMeasurementEvent(eventName);
    scheduleAt(simTime()+0.9, healthMeasurementEvent);


    healthVector.setName("Health");
    healthVector.record(currentHealth);
}

void V1::onProcessingEvent() {
    inboxLengthVector.recordWithTimestamp(simTime(), inbox->getLength());

    if(inbox->isEmpty()) {
        return;
    }

    cMessage *msg = (cMessage *)inbox->pop();

    enqueueMessageToOutboxes(msg);

    // process message: in the simulation we just count it up
    processedMessages++;

    // record processing time of each message
    processedMessagesMap[msg->getName()] = simTime().inUnit(SIMTIME_MS);


    delete msg;
}

void V1::enqueueMessageToOutboxes(cMessage *msg) {
    EV << "Enqueueing message: " << msg->getName() << "\n";
    cGate *arrivalGate = msg->getArrivalGate();

    for (int i=0; i<neighborsCount(); i++) {
        if(arrivalGate == NULL || arrivalGate->getIndex() != i) {
            // TODO: check whether outbox is full
            // currently outbox is of unlimited size

            cMessage *copy = (cMessage *)msg->dup();
            // put into outbox
            neighbors[i].outbox->insert(copy);
        }
    }
}

void V1::onPacingEvent(PacingEvent *pacingEvent) {
    // pacing event is triggered every simulation second
    scheduleAt(simTime()+1, pacingEvent);

    Neighbor n = neighbors[pacingEvent->getNeighbor()];

    EV << "Pacing: " << n.gate << "\n";
    EV << "rate: " << n.sendingRate << "\n";

    for(int i=0; i<n.sendingRate; i++) {
        double t = i / (double)n.sendingRate;

        SendingEvent *sendingEvent = new SendingEvent();
        sendingEvent->setNeighbor(pacingEvent->getNeighbor());

        scheduleAt(simTime() + t, sendingEvent);
    }
}

void V1::onSendingEvent(SendingEvent *sendingEvent) {
    Neighbor n = neighbors[sendingEvent->getNeighbor()];

    // record length of outbox
    n.outboxLengthVector->recordWithTimestamp(simTime(), n.outbox->getLength());

    if(n.outbox->isEmpty()) {
        return;
    }

    cMessage *msg = (cMessage *)n.outbox->pop();
    sendOnGate(msg, n.gate);
}

void V1::onHealthMeasurementEvent() {
    // healthMeasurementEvent is triggered every simulation second
    scheduleAt(simTime()+1, healthMeasurementEvent);

    int networkProcessingRate = par("networkProcessingRate");
    int alpha = par("alpha");

    // calculate health within [0, 1]
    int inboxLength = inbox->getLength();
    if (inboxLength < networkProcessingRate) {
        currentHealth = 1;
    } else if(inboxLength > alpha*networkProcessingRate) {
        currentHealth = 0;
    } else {
        currentHealth = 1 - (inboxLength / (double)(alpha*networkProcessingRate));
    }

    //EV << "Health: " << currentHealth << "\n";

    // track health of node over time
    healthVector.record(currentHealth);

    // send to neighbors
    HealthMessage *msg = new HealthMessage();
    msg->setHealth(currentHealth);

    for (int i=0; i<neighborsCount(); i++) {
       sendCopyOf(msg, i);
    }

    delete msg;
}

/*
 * Health computation engine: converts health measurement of a neighbor to a sending rate.
 */
void V1::onHealthMessage(HealthMessage *healthMsg) {
    int n = healthMsg->getArrivalGate()->getIndex();

    if(healthMsg->getHealth() >= 0 && healthMsg->getHealth() <= 1) {
        neighbors[n].health = healthMsg->getHealth();
    } else {
        EV << "Invalid health message from: " << n << ". Health: " << healthMsg->getHealth() << "\n";
    }
    neighbors[n].health = healthMsg->getHealth();

    // convert to sending rate
    int networkProcessingRate = par("networkProcessingRate");
    neighbors[n].sendingRate = int(networkProcessingRate * neighbors[n].health);

    EV << "Neighbor: " << n << " - " << neighbors[n].health << "|" << neighbors[n].sendingRate << "\n";

    delete healthMsg;

    // record sending rate per neighbor
    neighbors[n].sendingRateVector->record(neighbors[n].sendingRate);
}

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

    if(ProcessingEvent *processingEvent = dynamic_cast<ProcessingEvent*>(msg)) {
        onProcessingEvent();
        delete msg;
        return;
    }

    if(HealthMessage *healthMsg = dynamic_cast<HealthMessage*>(msg)) {
        onHealthMessage(healthMsg);
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


    // every other message is a data message
    onDataMessage(msg);
}


int V1::neighborsCount() {
    return gateSize("gate");
}
