simple Basic {
    parameters:               
//        int networkProcessingRate = default(10);
        double processingScale = default(1);
        string processingRateFile = default("./inputs/constant.csv");
        
        int generationRate = default(0);
        
        // false = free for all scenario. effects currentProcessingRate calculation in basic.cc and V1::computeRate
        bool cappedProcessingRate = default(true); 
        
		bool outOfSync = default(false);        
        
        // an inbox size <=0 means unlimited inbox size
        int maxInboxSize = default(100);
        
        string configName;
        bool verboseLog = default(true);
    gates:
        inout gate[] @loose;
}

simple V1 extends Basic {
    @class(V1);
    
    // an outbox size <=0 means unlimited outbox size
    int maxOutboxSize = default(100);
    
    bool disableDefenseEngine = default(false);
    
    // -1 == non-malicious behavior
    int maliciousStartTime = default(-1);
    
    // -1 == non-malicious behavior
    int maliciousPacingRate = default(-1);
    
    // -1 == all gates, otherwise gateIndex
    int maliciousGateIndex = default(-1);
    
    // -1 == non-malicious behavior
    double maliciousHealth = default(-1);
}

channel VolatileChannel {
    parameters:
        volatile double delay = uniform(0.05,0.15);
        bool disabled = default(false);
}

module ConfigWrapper 
{
    
        int n = default(100);
    	int conns = default(4);
    	
    	// node types / processing
    	double constant = default(0.5);
    	double aws = default(0.25);
    	double azure = default(0.15);
    	double didata = default(0.1);
    	
    	double minProcessingScale = default(0.9);
    	double maxProcessingScale = default(2.5);
    	double minGenerationThreshold = default(1.3);
    	
    	int networkProcessingRate = default(100);
    	int networkGenerationRate = default(networkProcessingRate);
    	double passiveGeneration = default(0.3);
    	double topGeneration = default(0.1); // x% of nodes generate top 20% of traffic
}

module DynamicNetworkBasic
{
    @class(DynamicNetwork);
    @isNetwork(true);
    
    submodules:
        config: ConfigWrapper;
        node[config.n]: Basic  {
            gates:
                gate[config.conns];
        }
}

module DynamicNetworkV1
{
    @class(DynamicNetwork);
    @isNetwork(true);
    
    submodules:
        config: ConfigWrapper;
        node[config.n]: V1  {
            gates:
                gate[config.conns];
        }
}

network BasicNetwork
{
    submodules:
        config: ConfigWrapper;
        node[3]: Basic;
    connections:
        node[0].gate++ <--> VolatileChannel <--> node[1].gate++;
        node[1].gate++ <--> VolatileChannel <--> node[2].gate++;
        node[0].gate++ <--> VolatileChannel <--> node[2].gate++;
}

network V1Network
{
    submodules:
        config: ConfigWrapper;
        node[3]: V1;
    connections:
        node[0].gate++ <--> VolatileChannel <--> node[1].gate++;
        node[1].gate++ <--> VolatileChannel <--> node[2].gate++;
        node[0].gate++ <--> VolatileChannel <--> node[2].gate++;
}

network MaliciousNetworkSimple
{
    submodules:
        config: ConfigWrapper;
        node[3]: V1;
    connections:
        node[0].gate++ <--> VolatileChannel <--> node[1].gate++;
        node[1].gate++ <--> VolatileChannel <--> node[2].gate++;
        node[0].gate++ <--> VolatileChannel <--> node[2].gate++;
}


network Scenario1
{
 	submodules:
 	    config: ConfigWrapper;
 	    node[10]: Basic {
            gates:
                gate[config.conns];
        }
 	connections:
 	    node[0].gate++ <--> VolatileChannel <--> node[1].gate++;
 	    node[0].gate++ <--> VolatileChannel <--> node[2].gate++;
 	    node[0].gate++ <--> VolatileChannel <--> node[6].gate++;
 	    node[0].gate++ <--> VolatileChannel <--> node[9].gate++;
 	    
 	    node[1].gate++ <--> VolatileChannel <--> node[2].gate++;
 	    node[1].gate++ <--> VolatileChannel <--> node[3].gate++;
 	    node[1].gate++ <--> VolatileChannel <--> node[8].gate++;
 	     
 	    node[2].gate++ <--> VolatileChannel <--> node[3].gate++;
 	    node[2].gate++ <--> VolatileChannel <--> node[7].gate++;
 	    
 	    node[3].gate++ <--> VolatileChannel <--> node[4].gate++;
 	    node[3].gate++ <--> VolatileChannel <--> node[5].gate++;

		node[4].gate++ <--> VolatileChannel <--> node[5].gate++;
 	    node[4].gate++ <--> VolatileChannel <--> node[6].gate++;
 	    node[4].gate++ <--> VolatileChannel <--> node[7].gate++; 
 	    
 	    node[5].gate++ <--> VolatileChannel <--> node[6].gate++;
 	    node[5].gate++ <--> VolatileChannel <--> node[9].gate++;
 	    
 	    node[6].gate++ <--> VolatileChannel <--> node[8].gate++;
 	    
 	    node[7].gate++ <--> VolatileChannel <--> node[8].gate++;
 	    node[7].gate++ <--> VolatileChannel <--> node[9].gate++;
 	    
 	    node[8].gate++ <--> VolatileChannel <--> node[9].gate++;    
}

network Scenario1V1
{
 	submodules:
 	    config: ConfigWrapper;
 	    node[10]: V1;
 	connections:
 	    node[0].gate++ <--> VolatileChannel <--> node[1].gate++;
 	    node[0].gate++ <--> VolatileChannel <--> node[2].gate++;
 	    node[0].gate++ <--> VolatileChannel <--> node[6].gate++;
 	    node[0].gate++ <--> VolatileChannel <--> node[9].gate++;
 	    
 	    node[1].gate++ <--> VolatileChannel <--> node[2].gate++;
 	    node[1].gate++ <--> VolatileChannel <--> node[3].gate++;
 	    node[1].gate++ <--> VolatileChannel <--> node[8].gate++;
 	     
 	    node[2].gate++ <--> VolatileChannel <--> node[3].gate++;
 	    node[2].gate++ <--> VolatileChannel <--> node[7].gate++;
 	    
 	    node[3].gate++ <--> VolatileChannel <--> node[4].gate++;
 	    node[3].gate++ <--> VolatileChannel <--> node[5].gate++;

		node[4].gate++ <--> VolatileChannel <--> node[5].gate++;
 	    node[4].gate++ <--> VolatileChannel <--> node[6].gate++;
 	    node[4].gate++ <--> VolatileChannel <--> node[7].gate++; 
 	    
 	    node[5].gate++ <--> VolatileChannel <--> node[6].gate++;
 	    node[5].gate++ <--> VolatileChannel <--> node[9].gate++;
 	    
 	    node[6].gate++ <--> VolatileChannel <--> node[8].gate++;
 	    
 	    node[7].gate++ <--> VolatileChannel <--> node[8].gate++;
 	    node[7].gate++ <--> VolatileChannel <--> node[9].gate++;
 	    
 	    node[8].gate++ <--> VolatileChannel <--> node[9].gate++;    
}