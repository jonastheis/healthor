#ifndef LOCAL_DATATYPES_H_
#define LOCAL_DATATYPES_H_

#include <stdlib.h>
#include <string.h>
#include <set>
#include <algorithm>
#include <omnetpp.h>

#include "cereal/types/set.hpp"
#include "cereal/types/string.hpp"



using namespace omnetpp;

const int ID_SIZE = 30;


typedef struct ltm {
    char id[ID_SIZE];
    int64_t issuingTime;
    int issuerNode;
    int sequence;

    char parent1[ID_SIZE];
    char parent2[ID_SIZE];

    bool requestMessageResponse = false;

    // metadata
    int64_t processingTime = -1;
    bool solid = false;
    int arrivalGateIndex = -1;

    // This method lets cereal know which data members to serialize
    template<class Archive>
    void serialize(Archive & archive) {
        archive(id, issuingTime, issuerNode, sequence, parent1, parent2, requestMessageResponse, processingTime, solid, arrivalGateIndex); // serialize things by passing them to the archive
    }

    void setId(const char* val) {
        strncpy(id, val, ID_SIZE);
    }
    void setParent1(const char* val) {
        strncpy(parent1, val, ID_SIZE);
    }
    void setParent2(const char* val) {
        strncpy(parent2, val, ID_SIZE);
    }

    void print() {
        EV << "LocalTangleMessage(id=" << id
                << ", issuingTime=" << issuingTime
                << ", issuerNode=" << issuerNode
                << ", sequence=" << sequence
                << ", parent1=" << parent1
                << ", parent2=" << parent2
                << ", processingTime=" << processingTime
                << ", solid=" << solid

                << ")\n";
    }
} LocalTangleMessage;


typedef struct approvers {
    char id[ID_SIZE];
    std::set<std::string> a;

    // This method lets cereal know which data members to serialize
    template<class Archive>
    void serialize(Archive & ar) {
        ar(id, a); // serialize things by passing them to the archive
    }

    void setId(char* val) {
        strncpy(id, val, ID_SIZE);
    }

    void insert(std::string msgId) {
        a.insert(msgId);
    }

    void remove(std::string msgId) {
        a.erase(msgId);
    }

    void print() {
        EV << "Approvers(id=" << id
                << ", size=" << a.size();
        for(auto f : a) {
          EV << ", approver=" << f;
        }
        EV << ")\n";
    }

} Approvers;

#endif /* LOCAL_DATATYPES_H_ */
