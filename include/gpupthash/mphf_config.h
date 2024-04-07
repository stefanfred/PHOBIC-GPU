#pragma once

#include <cmath>
#include "shader_constants.h"
#include <numeric>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

#include "bucketers.h"

namespace gpupthash {

    struct MPHFconfig {
    private:
        std::vector<uint32_t> fulcrums;

    public:
        uint32_t partitionSize;
        uint32_t bucketCountPerPartition;
        uint32_t sortingBins;
        double m_averageBucketSize;

        MPHFconfig(double averageBucketSize = 8.0, uint32_t partitionSize = 2048) :
                partitionSize(partitionSize),
                sortingBins(256),
                bucketCountPerPartition(uint32_t(std::ceil(partitionSize / averageBucketSize))),
                m_averageBucketSize(averageBucketSize) {
            Bucketer* defaultBucketer = new OptBucketer();
            setBucketer(defaultBucketer);
            delete defaultBucketer;
            std::cout<<bucketCountPerPartition<<std::endl;
        }

        MPHFconfig(Bucketer* bucketer, double averageBucketSize = 8.0, uint32_t partitionSize = 2048) :
                partitionSize(partitionSize),
                sortingBins(256),
                bucketCountPerPartition(uint32_t(round(partitionSize / averageBucketSize))),
                m_averageBucketSize(averageBucketSize) {
            setBucketer(bucketer);
        }

        void setBucketer(Bucketer *bucketer) {
            // initialize fulcrums for bucket assignment
            bucketer->init(m_averageBucketSize, partitionSize);
            fulcrums.push_back(0);
            for (size_t xi = 1; xi < FULCS_INTER - 1; xi++) {
                double x = double(xi) / double(FULCS_INTER - 1);
                double y = bucketer->getBucketRel(x);
                uint32_t fulcV = uint32_t(y * double(bucketCountPerPartition<<16));
                fulcrums.push_back(fulcV);
            }
            fulcrums.push_back(bucketCountPerPartition<<16);
        }

        const std::vector<uint32_t> &getFulcs() const {
            return fulcrums;
        }

        uint32_t partitionMaxSize() const {
            // ToDo use Poission quantil
            return partitionSize + partitionSize / 2;
        }
    };

}