#ifndef V1_H_
#define V1_H_

#include <stdio.h>
#include <string.h>
#include <omnetpp.h>
#include <map>

using namespace omnetpp;

#include "basic.h"
#include "health_measurement_event_m.h"
#include "health_message_m.h"
#include "pacing_event_m.h"
#include "sending_event_m.h"
#include "defense_event_m.h"

#define DEFENSE_RW 3
#define DEFENSE_RATE_GRACE 1.05
#define DEFENSE_RATE_EXCEEDED 5

#define DEFENSE_RATE_SUBCEEDED 5
#define DEFENSE_RATE_SUBCEED_THRESHOLD 0.25

#define DEFENSE_HEALTH_GRACE 0.95
#define DEFENSE_HEALTH_COUNT 10

struct Neighbor {
    int gate;
    double health;
    int sendingRate;
    int maxOutboxSize;
    int droppedMessages;
    cOutVector *outboxLengthVector;
    cOutVector *sendingRateVector;
    std::deque<std::string> *outbox;

    bool disabled = false;
    int receivingRate[DEFENSE_RW];
    int receivingRateExceeded = 0;
    int receivingRateSubceeded = 0;
    cOutVector *receivingRateVector;
    int unhealthyCount = 0;
};

class V1 : public Basic {
    protected:
        virtual void initialize() override;
        virtual void handleMessage(cMessage *msg) override;
        virtual void finish() override;

        int neighborsCount();
        virtual void sendOnGate(cMessage *msg, int gate) override;

        // health measurement engine
        virtual void onHealthMeasurementEvent();
        HealthMeasurementEvent *healthMeasurementEvent;
        double currentHealth = 1;

        // rate computation engine
        virtual int computeRate(double health);
        virtual void onHealthMessage(HealthMessage *healthMsg);

        // pacing engine
        virtual void onPacingEvent(PacingEvent *pacingEvent);
        virtual void onSendingEvent(SendingEvent *sendingEvent);

        // defense engine
        virtual void onDefenseEvent();
        DefenseEvent *defenseEvent;
        int allowedReceivingRate[DEFENSE_RW];
        void disableNeighbor(int neighbor);

        // attacker (optional)
        int maliciousPacingRate;
        simtime_t maliciousStartTime;
        int maliciousGateIndex;
        double maliciousHealth;

        // message scheduling / processing
        virtual void calculateCurrentProcessingRate() override;
        virtual void enqueueMessageToOutboxes(LocalTangleMessage *msg);
        virtual void onProcessingEvent() override;

        Neighbor *neighbors;

        // stats
        cOutVector healthVector;
        cOutVector allowedReceivingRateVector;

        int sentHealthMessages = 0;
        int receivedHealthMessages = 0;

};

Define_Module(V1);

#endif /* V1_H_ */
