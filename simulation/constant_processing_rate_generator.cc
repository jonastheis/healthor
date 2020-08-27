#include <stdio.h>
#include <string.h>
#include <omnetpp.h>
#include <cmath>

using namespace omnetpp;

#include "constant_processing_rate_generator.h"


double ConstantProcessingRateGenerator::draw(double max) {
    if(interval == 0) {
        return max;
    }

    // update state
    if(generated % interval == 0) {
        isMaxMode = !isMaxMode;
    }
    generated++;


    if(isMaxMode) {
        return max;
    }

    return low;
}
