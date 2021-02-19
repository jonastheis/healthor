#ifndef SOLIDIFIER_H_
#define SOLIDIFIER_H_

#include <stdlib.h>
#include <string.h>
#include <map>
#include <omnetpp.h>
#include <deque>

using namespace omnetpp;

#include "tangle.h"

struct SolidificationResult {
    std::vector<std::string> solidMessages;
    std::vector<std::string> outstandingRequests;
};


class Solidifier {
public:
    Solidifier();
    Solidifier(Tangle* tangle);

    std::vector<std::string> handleUnsolidMessage(LocalTangleMessage* msg);
    SolidificationResult handleSolidMessage(LocalTangleMessage* msg);

    int bufferSize() const;
    LocalTangleMessage* dropFromTail();
//    TangleMessage* dropFromHead();
//    TangleMessage* dropRandom();

    int outstandingRequests() const;
    bool isOutstandingRequest(const char* msgId);

private:
    // reference to the tangle in order to retrieve messages and check message solidity
    Tangle *tangle;

    // an unsolid message has unknown parents. This map keeps track of all outstanding message requests.
    std::map<std::string, bool> requests;

    // the solidification buffer. all unsolid messages reside here until they get solid.
    std::map<std::string, bool> buffer;
    std::deque<std::string> bufferQueue;

    void addMessageToRequests(std::vector<std::string>* outstandingRequests, const char* msgId);

    void solidify(std::map<std::string, bool>* solidMessages, std::vector<std::string>* outstandingRequests, LocalTangleMessage* msg);
};

#endif /* SOLIDIFIER_H_ */
