#ifndef BASIC_H_
#define BASIC_H_

#include <stdio.h>
#include <iostream>
#include <string.h>
#include <omnetpp.h>
#include <map>
#include <deque>
#include <functional>
#include <algorithm>


using namespace omnetpp;

#include "generation_event_m.h"
#include "create_message_event_m.h"
#include "scheduling_event_m.h"
#include "processing_event_m.h"
#include "request_message_m.h"
#include "request_message_event_m.h"
#include "tangle_message_m.h"

#include "constant_processing_rate_generator.h"
#include "csv.h"
#include "tangle.h"
#include "solidifier.h"
#include "dynamic_network.h"


class Basic : public cSimpleModule {
    public:
        ~Basic();

    protected:
        virtual void initialize() override;
        virtual void refreshDisplay() const override;
        virtual void handleMessage(cMessage *msg) override;
        virtual void finish() override;

        virtual void broadcastMessage(cMessage *msg, int arrivalGate);
        virtual void sendCopyOf(cMessage *msg, int gate);
        virtual void sendOnGate(cMessage *msg, int gate);

        virtual void onTangleMessage(TangleMessage *msg);
        virtual void onRequestMessage(RequestMessage *msg);
        virtual void handleUnsolidMessage(LocalTangleMessage *msg);
        virtual void onCreateMessageRequestEvent(RequestMessageEvent *event);
        virtual bool dropPolicy(LocalTangleMessage *msg);
        virtual void dropMessage(LocalTangleMessage *msg);

        // peering
        virtual void connectNeighbor();
        virtual Basic* disconnectNeighbor(int gateId);
        virtual void goOutofSync();
        virtual std::map<int, bool> getNeighbors();

        // generate messages
        virtual void calculateCurrentGenerationRate();
        virtual void onGenerationEvent();
        virtual void generateMessages();
        virtual void onCreateMessageEvent();
        GenerationEvent *generationEvent;

        // schedule messages
        virtual void calculateCurrentProcessingRate();
        virtual void onSchedulingEvent();
        virtual void scheduleMessages();
        virtual void onProcessingEvent();
        SchedulingEvent *schedulingEvent;

        int currentGenerationRate = 0;
        int generationRate = 0;

        // defined as messages/simulation second, i.e. higher is better
        std::vector<double> processingRate;
        int currentProcessingRate = 0;
        double processingScale = 0;
        int processingPatternShift = 0;
        int networkProcessingRate = 0; // used to create offset reading from processingRate: not all nodes behave the same at the same time.
        bool cappedProcessingRate = true;

        // stats
        int generatedMessages = 0;
        int processedMessages = 0;
        int droppedMessages = 0;

        int sentMessages = 0;
        int sentMessageRequests = 0;
        int sentMessageRequestResponses = 0;

        int receivedMessages = 0;
        int receivedMessageRequests = 0;
        int receivedMessageRequestResponses = 0;

        cOutVector inboxLengthVector;
        cOutVector totalInboxVector;
        cOutVector availableProcessingRateVector;
        cOutVector solidificationBuffer;
        cOutVector outstandingMessageRequests;

        // local ledger related stuff
        std::deque<LocalTangleMessage*> inbox;
        int maxInboxSize;
        Tangle tangle;
        Solidifier solidifier;

        bool verboseLog;
};
Define_Module(Basic);


static TangleMessage* TangleMessageFromLocal(LocalTangleMessage* local) {
    TangleMessage* msg = new TangleMessage(local->id);
    msg->setId(local->id);
    msg->setIssuerNode(local->issuerNode);
    msg->setSequence(local->sequence);
    msg->setIssuingTime(local->issuingTime);

    msg->setRequestMessageResponse(local->requestMessageResponse);

    msg->setParent1(local->parent1);
    msg->setParent2(local->parent2);

    return msg;
}

static LocalTangleMessage* LocalFromTangleMessage(TangleMessage* msg) {
    LocalTangleMessage *local = new LocalTangleMessage();
    local->setId(msg->getId());
    local->issuingTime = msg->getIssuingTime();
    local->issuerNode = msg->getIssuerNode();
    local->sequence = msg->getSequence();

    local->requestMessageResponse = msg->getRequestMessageResponse();

    local->setParent1(msg->getParent1());
    local->setParent2(msg->getParent2());

    local->arrivalGateIndex = msg->getArrivalGate() == NULL ? -1 : msg->getArrivalGate()->getIndex();

    return local;
}


#endif /* BASIC_H_ */
