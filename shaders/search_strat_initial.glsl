void initial_pos(uint globalBucketStartPos, uint pilot, uint partitionSize, uint size) {
    if(size==1) {
        if(lID==0) {
            initialPos[0] = hash64(globalBucketStartPos, pilot)
            % partitionSize;
        }
        barrier();
        return;
    }

    if(lID==0) {
        localCollision = false;
    }

    for(uint index = lID; index < MAX_PARTITION_SIZE / 32; index+= wSize) {
        localCollisionArray[index] = 0;
    }
    barrier();
    for(uint i = lID;i < size && !localCollision; i+=wSize) {
        uint pos = hash64(globalBucketStartPos + i, pilot) % partitionSize;
        uint mask = 1 << (pos % 32);
        if((atomicOr(localCollisionArray[pos / 32], mask) & mask) != 0) {
            localCollision = true;
        }
        initialPos[i] = pos;
        barrier();
    }
}

uint pos(uint i, uint offset, uint partitionSize) {
    uint pos = initialPos[i] + offset;
    if(pos>=partitionSize) {
        pos -=partitionSize;
    }
    return pos;
}