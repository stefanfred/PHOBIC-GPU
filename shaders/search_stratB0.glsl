uint pilot = 0;
uint offset = lID;

while(true) { // search for mapping
    
    // find no local coll pilot
    initial_pos(globalBucketStartPos, pilot, partitionSize, size);
    if(size>1) {
        if(localCollision) {
            pilot += 1;
            continue;
        }
    }

    // small bucket (use displacement)

    if(lID==0) {
        pilotFound = 0xFFFFFFFF;
        sharedOffset = wSize;
    } 
    barrier();

    uint index = 0;
    while(offset < partitionSize && pilotFound == 0xFFFFFFFF) {
        #ifdef PILOT_LIMIT
        if(pilot*partitionSize+offset>endPilot.pilot) {
            break;
        }
        #endif
        uint pos = pos(index, offset, partitionSize);
        index+=1;
        if((free[pos / 32] & (1 << (pos % 32))) != 0) {
            index=0;
            offset=atomicAdd(sharedOffset, 1);
        } else if(index==size) {
            atomicMin(pilotFound, offset);
        }
        //barrier();
    }
    barrier();
    offset = lID;

    if(pilotFound != 0xFFFFFFFF) {
        // write result to free array
        for(uint i = lID;i < size; i+=wSize) {
            uint pos = pos(i, pilotFound, partitionSize);
            uint mask = 1 << (pos % 32);
            atomicOr(free[pos/32], mask);
        }
        barrier();

        // write pilot to output
        if(lID == 0) {
            pilotsFound[bucketCnt] = pilotFound + partitionSize * pilot;
        }
        // continue with next bucket
        return true;
    } else {
        pilot+=1;
    }
}
return true;