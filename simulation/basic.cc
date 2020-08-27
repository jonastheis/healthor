#include "basic.h"

extern omnetpp::cConfigOption *CFGID_OUTPUT_VECTOR_FILE;

void Basic::initialize() {
    // initialize variables
    inbox = new cQueue();
    WATCH_PTR(inbox);

    // set parameters
    generationRate = par("generationRate");
    maxInboxSize = par("maxInboxSize");

    int networkProcessingRate = par("networkProcessingRate");
    currentProcessingRate = networkProcessingRate * (double)par("processingRateRandom");
    processingRateGenerator = new ConstantProcessingRateGenerator(par("targetProcessingRate"), int(par("processingChangeInterval")));

    // start generation event, called every simulation second
    char generationEventName[2];
    sprintf(generationEventName, "g%d", getIndex());
    generationEvent = new GenerationEvent(generationEventName);
    scheduleAt(simTime(), generationEvent);

    // start scheduler, called every simulation second
    char eventName[2];
    sprintf(eventName, "s%d", getIndex());
    schedulingEvent = new SchedulingEvent(eventName);
    scheduleAt(simTime(), schedulingEvent);

    // prepare collection of statistics
    generatedMessages = 0;
    WATCH(generatedMessages);

    processedMessages = 0;
    WATCH(processedMessages);

    droppedMessages = 0;
    WATCH(droppedMessages);

    inboxLengthVector.setName("InboxLength");
    inboxLengthVector.record(inbox->getLength());

    availableProcessingRateVector.setName("AvailableProcessingRate");
}

void Basic::refreshDisplay() const {
    int generationRate = par("generationRate");
    char buf[40];
    sprintf(buf, "G: %d, P: %d, Q: %d/%d", generationRate, currentProcessingRate, inbox->getLength(), maxInboxSize);
    getDisplayString().setTagArg("t", 0, buf);
}


void Basic::onGenerationEvent() {
    // generation event is triggered every simulation second
    scheduleAt(simTime()+1, generationEvent);

    // stop generation end at predefined time
    if (simTime() > par("generationDuration")) {
        return;
    }

    generateMessages();
}

void Basic::generateMessages() {
    for(int i=0; i<generationRate; i++) {
        double t = i / (double)generationRate;

        // generate message and enqueue it according to its time
        createMessage(t);
    }
}

void Basic::createMessage(double scheduleTime) {
    char msgname[20];
    sprintf(msgname, "n%d-m%d", getIndex(), generatedMessages++);
    cMessage *msg = new cMessage(msgname);

    simtime_t t = simTime() + scheduleTime;

    // record generation time of each message
    generatedMessagesMap[msg->getName()] = t.inUnit(SIMTIME_MS);

    scheduleAt(t, msg);
}

void Basic::onSchedulingEvent() {
    // scheduling event is triggered every simulation second
    scheduleAt(simTime()+1, schedulingEvent);

    int networkProcessingRate = par("networkProcessingRate");
    double processingRateRandom = par("processingRateRandom");
    currentProcessingRate = networkProcessingRate * processingRateGenerator->draw(processingRateRandom);
    EV << "Processing rate " << currentProcessingRate << "\n";

    availableProcessingRateVector.record(currentProcessingRate);

    scheduleMessages();
}

void Basic::scheduleMessages() {
    int i;
    for(i=0; i<currentProcessingRate; i++) {
        double t = i / (double)currentProcessingRate;

        scheduleAt(simTime() + t, new ProcessingEvent());
    }
}

void Basic::onProcessingEvent() {
    inboxLengthVector.recordWithTimestamp(simTime(), inbox->getLength());

    if(inbox->isEmpty()) {
        return;
    }

    cMessage *msg = (cMessage *)inbox->pop();

    broadcastMessage(msg);

    // process message: in the simulation we just count it up
    processedMessages++;

    // record processing time of each message
    processedMessagesMap[msg->getName()] = simTime().inUnit(SIMTIME_MS);


    delete msg;
}

void Basic::broadcastMessage(cMessage *msg) {
    EV << "Broadcast message: " << msg->getName() << "\n";
    cGate *arrivalGate = msg->getArrivalGate();

    for (int i=0; i<gateSize("gate"); i++) {
        if(arrivalGate == NULL || arrivalGate->getIndex() != i) {
            EV << "Forwarding message " << msg << " on gate[" << i << "]\n";
            sendCopyOf(msg, i);
        }
    }
}

void Basic::sendCopyOf(cMessage *msg, int gate) {
    // Duplicate message and send the copy.
    cMessage *copy = (cMessage *)msg->dup();
    send(copy, "gate$o", gate);
}


void Basic::sendOnGate(cMessage *msg, int gate) {
    send(msg, "gate$o", gate);
}



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


    // every other message is a data message
    onDataMessage(msg);
}

void Basic::onDataMessage(cMessage *msg) {
    // filter with map
    if (seenMessages.count(msg->getName()) > 0) {
        EV << "Already processed " << msg->getName() << ". Not queuing to inbox." << "\n";
        delete msg;
        return;
    }

    // check whether inbox is full
    // an inbox size <=0 means unlimited inbox size
    if(maxInboxSize > 0 && inbox->getLength() >= maxInboxSize) {
        droppedMessages++;
        EV << "Inbox is full. Dropping " << msg->getName() << "\n";
        delete msg;
        return;
    }

    // mark message as already processed
    seenMessages[msg->getName()] = true;

    EV << "Pushing " << msg->getName() << ". to inbox." << "\n";

    // put into inbox queue
    inbox->insert(msg);
}

void Basic::finish() {
    std::string configName = par("configName");

    char fileName[100];
    sprintf(fileName, "./results/%s-generated_%d.csv", configName.c_str(), getIndex());
    CSVWriter *writer = new CSVWriter(fileName, "msgID,time");
    writer->writeMap(generatedMessagesMap);
    writer->close();

    sprintf(fileName, "./results/%s-processed_%d.csv", configName.c_str(), getIndex());
    CSVWriter *writer2 = new CSVWriter(fileName, "msgID,time");
    writer2->writeMap(processedMessagesMap);
    writer2->close();

}


