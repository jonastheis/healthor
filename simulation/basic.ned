simple Basic {
    parameters:
        int generationRate = default(0);
        
        double targetProcessingRate = default(0);
        int processingChangeInterval = default(0);
        volatile double processingRateRandom = default(uniform(1,1));
        int networkProcessingRate = default(10);
        
        // an inbox size <=0 means unlimited inbox size
        int maxInboxSize = default(100);
        
        // let node generate messages until this simulation time in seconds
        int generationDuration = default(60);
        
        string configName;
    gates:
        inout gate[];
}

simple V1 extends Basic {
    @class(V1);
    
    int maxOutboxSize = default(100);

	// alpha is a scaling factor we use for scaling health. Once we introduce queue lengths again it should be removed.   
	int alpha = default(10);
}

channel VolatileChannel {
    parameters:
        volatile double delay = uniform(0.05,0.15);
        bool disabled = default(false);
}



network BasicNetwork
{
    submodules:
        node[3]: Basic;
    connections:
        node[0].gate++ <--> VolatileChannel <--> node[1].gate++;
        node[1].gate++ <--> VolatileChannel <--> node[2].gate++;
        node[0].gate++ <--> VolatileChannel <--> node[2].gate++;
}

network V1Network
{
    submodules:
        node[3]: V1;
    connections:
        node[0].gate++ <--> VolatileChannel <--> node[1].gate++;
        node[1].gate++ <--> VolatileChannel <--> node[2].gate++;
        node[0].gate++ <--> VolatileChannel <--> node[2].gate++;
}

network Scenario1
{
 	submodules:
 	    node[10]: Basic;
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