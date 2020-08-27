#include <stdio.h>
#include <string.h>
#include <omnetpp.h>
#include <map>

using namespace omnetpp;

#include "generation_event_m.h"
#include "scheduling_event_m.h"
#include "processing_event_m.h"
#include "constant_processing_rate_generator.h"
#include "csv_writer.h"



class Basic : public cSimpleModule {
    protected:
        virtual void initialize() override;
        virtual void refreshDisplay() const override;
        virtual void handleMessage(cMessage *msg) override;
        virtual void finish() override;

        virtual void onDataMessage(cMessage *msg);
        virtual void broadcastMessage(cMessage *msg);
        virtual void sendCopyOf(cMessage *msg, int gate);
        virtual void sendOnGate(cMessage *msg, int gate);
        virtual void createMessage(double scheduleTime);

        virtual void onGenerationEvent();
        virtual void generateMessages();

        virtual void onSchedulingEvent();
        virtual void scheduleMessages();

        virtual void onProcessingEvent();

        // defined as messages/simulation second, i.e. higher is better
        int generationRate;

        // defined as messages/simulation second, i.e. higher is better
        ConstantProcessingRateGenerator *processingRateGenerator;
        int currentProcessingRate;

        // stats
        int generatedMessages;
        int processedMessages;
        int droppedMessages;
        cOutVector inboxLengthVector;
        cOutVector availableProcessingRateVector;
        std::map<std::string, int64_t> generatedMessagesMap;
        std::map<std::string, int64_t> processedMessagesMap;

        std::map<std::string, bool> seenMessages;
        cQueue *inbox;
        int maxInboxSize;

        SchedulingEvent *schedulingEvent;
        GenerationEvent *generationEvent;

};
Define_Module(Basic);

