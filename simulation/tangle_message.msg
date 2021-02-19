message TangleMessage
{
    string id;
    int issuerNode;
    int sequence;
    int64_t issuingTime;
    
    string parent1;
    string parent2;
    
    bool requestMessageResponse = false;
    
    // metadata: needs to be reset on arrival of node
    int64_t processingTime;
    bool solid = false;
}
