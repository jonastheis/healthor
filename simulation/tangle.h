#ifndef TANGLE_H_
#define TANGLE_H_

#include <stdlib.h>
#include <string.h>
#include <map>
#include <algorithm>
#include <omnetpp.h>
#include "db.h"

using namespace omnetpp;


// make global so that all nodes use the same DB
static DBWrapper *dbKV = NULL;


struct Parents {
    std::string parent1;
    std::string parent2;
};


class Tangle {
public:
    Tangle();
    Tangle(int nodeId);

    void add(LocalTangleMessage* msg);
    LocalTangleMessage* issueMessage();
    LocalTangleMessage* getMessage(const char* msgId);

    bool checkMessageSolidity(LocalTangleMessage* msg);
    bool isParentSolid(const char* parent);

    void addKnownMessage(LocalTangleMessage* msg);
    void removeKnownMessage(LocalTangleMessage* msg);

    void addApprover(char* msgId, char* approverId);
    void removeApprover(char* msgId, char* approverId);
    Approvers* getApprovers(char* msgId);

    void print();

    void iterateTangle(std::function<void(LocalTangleMessage*)> callback);

    void seen(const char *msgId);
    bool isSeen(const char *msgId);
    void deleteSeen(const char *msgId);


private:
    void addTip(LocalTangleMessage* msg);
    Parents getTips();

    int nodeId;

    // tips are stored in both a map and list for random and fast access
    std::map<std::string, bool> tips;
    std::vector<std::string> tipsList;

    int sequence = 0;

    // tests for KV store implementation
    void testKVstore();
    void testTangleFunctions();
};

#endif /* TANGLE_H_ */
