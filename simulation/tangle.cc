#include "tangle.h"

const char* GENESIS = "GENESIS";

Tangle::Tangle() {
    this->nodeId = 0;
}

Tangle::Tangle(int nodeId) {
    this->nodeId = nodeId;

    std::string dbPath = "/tmp/healthor";
    if(dbKV == NULL) {
        dbKV = new DBWrapper(dbPath.c_str());
    }

//    testKVstore();
//    testTangleFunctions();
}

void Tangle::testKVstore() {
    LocalTangleMessage msg;
    strncpy(msg.id, "ABCDEDF", 6);
    msg.issuingTime = 1112;
    EV << msg.id << " " << msg.issuingTime << "\n";

    dbKV->putKnownMessage(nodeId, &msg);

//    dbKV.deleteKnownMessage(msg.id);

    LocalTangleMessage *msg2 = dbKV->getKnownMessage(nodeId, msg.id);

    if(msg2 != NULL) {

        EV << "Load known message from DB: " << msg2->id << " " << msg2->issuingTime << "\n";
    }
    delete msg2;





    // test tangle store
    msg.issuingTime = 999999;
    dbKV->putTangleMessage(nodeId, &msg);

    LocalTangleMessage *msg3 = dbKV->getTangleMessage(nodeId, msg.id);

    if(msg3 != NULL) {
        EV << "Load tangle message from DB: " << msg3->id << " " << msg3->issuingTime << "\n";
    }
    delete msg3;




    // test approver store
    Approvers a1;
    strncpy(a1.id, "a1", 6);

    a1.insert(msg.id);
    a1.insert("zzzzz");
    a1.insert("zzzzzXXXXXX");

    EV << "Approver " << a1.id << "(" << a1.a.size() << "):";
    for(auto f : a1.a) {
      EV << " " << f;
    }
    EV << "\n";


    dbKV->putApprovers(nodeId, &a1);

    Approvers *a2 = dbKV->getApprovers(nodeId, a1.id);

    if(a2 != NULL) {
        EV << "Load Approver from DB " << a1.id << "(" << a1.a.size() << "):";
            for(auto f : a1.a) {
              EV << " " << f;
            }
            EV << "\n";
    }
    delete a2;


    std::string seenId = std::to_string(nodeId) + "_123456";
    // test seen messages
    dbKV->putSeen(nodeId, msg.id);
    dbKV->putSeen(nodeId, seenId.c_str());
    EV << msg.id << ": " << dbKV->isSeen(nodeId, msg.id) << "\n";
    EV << seenId.c_str() << ": " << dbKV->isSeen(nodeId, seenId.c_str()) << "\n";

    dbKV->deleteSeen(nodeId, msg.id);
    EV << msg.id << ": " << dbKV->isSeen(nodeId, msg.id) << "\n";
    EV << seenId.c_str() << ": " << dbKV->isSeen(nodeId, seenId.c_str()) << "\n";

    // this should trigger an error log
    dbKV->deleteSeen(nodeId, msg.id);
}

void Tangle::testTangleFunctions() {
    LocalTangleMessage *msg0 = issueMessage();
    addKnownMessage(msg0);
    checkMessageSolidity(msg0);
    add(msg0);

    // check for message
    LocalTangleMessage *msg0_copy = getMessage(msg0->id);
    msg0_copy->print();
    delete msg0_copy;


    // build local tangle
    LocalTangleMessage *msg1 = issueMessage();
    addKnownMessage(msg1);
    add(msg1);

    checkMessageSolidity(msg1);
    msg1->print();
}



void Tangle::iterateTangle(std::function<void(LocalTangleMessage*)> callback) {
    dbKV->iterateTangle(nodeId, callback);
}


/**
 * Adds a message to the knownMessage buffer.
 */
void Tangle::addKnownMessage(LocalTangleMessage* msg) {
    dbKV->putKnownMessage(nodeId, msg);

    addApprover(msg->parent1, msg->id);
    addApprover(msg->parent2, msg->id);
}

void Tangle::addApprover(char* msgId, char* approverId) {
    Approvers *approvee = dbKV->getApprovers(nodeId, msgId);

    if(approvee == NULL) {
        approvee = new Approvers();
        approvee->setId(msgId);
    }

    approvee->insert(approverId);
    dbKV->putApprovers(nodeId, approvee);

    delete approvee;
}

/**
 * Removes a message from the knownMessage buffer.
 */
void Tangle::removeKnownMessage(LocalTangleMessage* msg) {
    dbKV->deleteKnownMessage(nodeId, msg->id);

    removeApprover(msg->parent1, msg->id);
    removeApprover(msg->parent2, msg->id);

    delete msg;
}

void Tangle::removeApprover(char* msgId, char* approverId) {
    Approvers *approvee = dbKV->getApprovers(nodeId, msgId);
    if(approvee == NULL) {
        return;
    }

    approvee->remove(approverId);
    dbKV->putApprovers(nodeId, approvee);

    delete approvee;
}

Approvers* Tangle::getApprovers(char* msgId) {
    return dbKV->getApprovers(nodeId, msgId);
}

/**
 * Adds a message to the local tangle.
 */
void Tangle::add(LocalTangleMessage* msg) {
    EV << "Adding " << msg->id << " to tangle. \n";

    msg->processingTime = simTime().inUnit(SIMTIME_MS);

    // add as tip
    addTip(msg);

    // add to tangle
    dbKV->putTangleMessage(nodeId, msg);
    // update known messages
    dbKV->putKnownMessage(nodeId, msg);

    // print tangle, for debug purposes only
    //print();
}

/**
 * Creates a TangleMessage with all its fields and performs tip selection.
 */
LocalTangleMessage* Tangle::issueMessage() {
    char id[ID_SIZE];
    sprintf(id, "%d_%d", nodeId, sequence);

    LocalTangleMessage* msg = new LocalTangleMessage();
    msg->setId(id);
    msg->issuerNode = nodeId;
    msg->sequence = sequence;
    msg->issuingTime = simTime().inUnit(SIMTIME_MS);

    Parents parents = getTips();
    msg->setParent1(parents.parent1.c_str());
    msg->setParent2(parents.parent2.c_str());

    sequence++;

    return msg;
}

/**
 * Adds the message as tip and removes its parents from the tip list.
 */
void Tangle::addTip(LocalTangleMessage* msg) {
    // add tip to list and map, we need list and map to enable random and fast access
    tipsList.push_back(msg->id);
    tips[msg->id] = true;

    EV_DETAIL << msg->id << " added as tip. total: " << tipsList.size() << "\n";

    // remove its parents if they are tips
    if (tips.erase(msg->parent1)) {
        tipsList.erase(std::remove(tipsList.begin(), tipsList.end(), msg->parent1), tipsList.end());
        EV_DETAIL << msg->parent1 << " removed. total: " << tipsList.size() << "\n";
    }
    if (tips.erase(msg->parent2)) {
        tipsList.erase(std::remove(tipsList.begin(), tipsList.end(), msg->parent2), tipsList.end());
        EV_DETAIL << msg->parent2 << " removed. total: " << tipsList.size() << "\n";
    }
}
/**
 * Performs random tip selection.
 * Makes sure that distinct tips are returned if there is more than 1 tip currently.
 */
Parents Tangle::getTips() {
    Parents p;

    if (tipsList.size() == 0) {
        p.parent1 = GENESIS;
        p.parent2 = GENESIS;
        return p;
    }

    if (tipsList.size() == 1) {
        p.parent1 = tipsList[0];
        p.parent2 = tipsList[0];
        return p;
    }

    // get random parent1
    int index = rand() % tipsList.size();
    p.parent1 = tipsList[index];

    // get random parent2; make sure they are not the same if enough tips are available; must be >1
    int index2;
    do {
        index2 = rand() % tipsList.size();
        p.parent2 = tipsList[index2];
    } while(index == index2);

    return p;
}

/**
 * Gets a message that is known to the node.
 */
LocalTangleMessage* Tangle::getMessage(const char* msgId) {
    return dbKV->getKnownMessage(nodeId, msgId);
}

/**
 * Checks a message's solidity by checking both parents.
 * If both parents are solid the message is marked as solid and true is returned.
 * This recursive solidification makes sure that all history of a message is known.
 */
bool Tangle::checkMessageSolidity(LocalTangleMessage* msg) {
    msg->solid = false;

    if(!isParentSolid(msg->parent1)) {
        return false;
    }

    if(!isParentSolid(msg->parent2)) {
        return false;
    }

    msg->solid = true;

    dbKV->putKnownMessage(nodeId, msg);

    return true;
}

/**
 * Checks whether a parent is solid.
 * A parent is solid if:
 * 1. it is GENESIS or
 * 2. it is marked as solid, i.e., its parents are solid.
 */
bool Tangle::isParentSolid(const char* parent) {
    if(strcmp(parent, GENESIS) == 0) {
        return true;
    }

    LocalTangleMessage *parentMsg = getMessage(parent);
    if(parentMsg != NULL && parentMsg->solid && dbKV->isInTangle(nodeId, parent)) {
        delete parentMsg;
        return true;
    }

    delete parentMsg;
    return false;
}

///**
// * Comparator for TangleMessages.
// * Sorts by issuingTime.
// */
//bool compareTangleMessage(TangleMessage* m1, TangleMessage* m2) {
//    return (m1->getIssuingTime() < m2->getIssuingTime());
//}
//
///**
// * Prints the local tangle of a node in a user friendly format.
// * Only necessary for debug purposes.
// */
//void Tangle::print() {
//    EV << "Tangle on node " << nodeId << "\n"
//            << "------------------------------------------\n";
//
//    // sort messages by issuing time -> should yield the same tangle on all nodes
//    std::vector<TangleMessage*> v;
//    for(std::map<std::string, TangleMessage*>::iterator it = db.begin(); it != db.end(); it++) {
//        v.push_back( it->second );
//    }
//    std::sort(v.begin(), v.end(), compareTangleMessage);
//
//    // print tangle
//    for (std::vector<TangleMessage*>::iterator it = v.begin(); it != v.end(); it++) {
//        TangleMessage* msg = *it;
//        EV << msg->getId() << " - solid: " << msg->getSolid() << " - processingTime: " << msg->getProcessingTime() << "\n"
//                << "    p1: " << msg->getParent1() << "\n"
//                << "    p2: " << msg->getParent2() << "\n"
//                << "------------------------------------------\n";
//    }
//}


void Tangle::seen(const char *msgId) {
    dbKV->putSeen(nodeId, msgId);
}

bool Tangle::isSeen(const char *msgId) {
    return dbKV->isSeen(nodeId, msgId);
    return false;
}

void Tangle::deleteSeen(const char *msgId) {
    dbKV->deleteSeen(nodeId, msgId);
}
