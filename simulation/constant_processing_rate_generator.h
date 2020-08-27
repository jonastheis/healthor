#include <stdio.h>
#include <string.h>
#include <omnetpp.h>
#include <cmath>

using namespace omnetpp;

class ConstantProcessingRateGenerator{
    private:
        bool isMaxMode = false;
        int generated = 0;

        double low = 0;
        int interval = 0;
    public:
         ConstantProcessingRateGenerator() {}
         ConstantProcessingRateGenerator(double low, int interval) : low(low), interval(interval) {}
         virtual double draw(double max);

};
