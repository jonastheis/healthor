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

struct Neighbor {
    int gate;
    double health;
    int sendingRate;
    int maxOutboxSize;
    cOutVector *outboxLengthVector;
    cOutVector *sendingRateVector;
    cQueue *outbox;
};

class V1 : public Basic {
    protected:
        virtual void initialize() override;
        virtual void handleMessage(cMessage *msg) override;

//        virtual void onSchedulingEvent() override;
//        virtual void scheduleMessages() override;
        virtual void onProcessingEvent() override;

        virtual void onHealthMeasurementEvent();
        virtual void onHealthMessage(HealthMessage *healthMsg);

        virtual void enqueueMessageToOutboxes(cMessage *msg);
        virtual void onPacingEvent(PacingEvent *pacingEvent);
        virtual void onSendingEvent(SendingEvent *sendingEvent);

        int neighborsCount();

        HealthMeasurementEvent *healthMeasurementEvent;
        double currentHealth = 1;
        Neighbor *neighbors;

        // stats
        cOutVector healthVector;
};

Define_Module(V1);
