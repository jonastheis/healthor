#include <stdio.h>
#include <string.h>
#include <omnetpp.h>
#include <cmath>

using namespace omnetpp;

class VolatileChannel : public cDelayChannel {
    public:
        virtual void processMessage(cMessage *msg, simtime_t t, result_t& result) override;

};
Define_Channel(VolatileChannel);


void VolatileChannel::processMessage(cMessage *msg, simtime_t t, result_t& result) {
    cDelayChannel::processMessage(msg, t, result);

    volatile double delay = par("delay");
    result.delay = delay;
}
