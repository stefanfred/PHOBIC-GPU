if(size>1 && lID==0) {
    localCollision = false;
}

//for(uint i=0;i<10;i++) {
    initial_pos(globalBucketStartPos, pilot, partitionSize, size);
//}

if(size>1) {
    if(localCollision) {
        pilot += 1;
        #ifdef PILOT_LIMIT
        if(pilot > endPilot.p1) {
            return false;
        }
        #endif
        continue;
    }
    #ifdef PILOT_LIMIT
    else if (lID == 0) {
        bucketPossible = true;
    }
    #endif
}