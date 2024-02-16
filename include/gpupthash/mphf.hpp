#pragma once

#include "shader_constants.h"
#include "mphf_config.h"
#include "encoders/base/golomb_sequence.hpp"
#include "encoders/base/ef_sequence.hpp"
#include "encoders/pilotEncoders/ortho_encoder.hpp"
#include "encoders/pilotEncoders/ortho_encoder_dual.hpp"
#include "encoders/base/encoders.hpp"
#include "fastmod/fastmod.h"
#include "hasher.hpp"

namespace gpupthash {


    template<typename PilotEncoder, typename PartitionOffsetEncoder, typename Hasher>
    class MPHF {

    private:
        MPHFconfig config;

        uint32_t partitions;
        PilotEncoder pilots;
        PartitionOffsetEncoder partitionOffsets;

        inline uint32_t hash(uint32_t pilot, uint32_t key) const {
            key ^= pilot;
            key = ((key >> 16) ^ key) * 0x45d9f3b;
            key = ((key >> 16) ^ key) * 0x45d9f3b;
            key = (key >> 16) ^ key;
            return key;
        }

        inline uint32_t hashPos(uint32_t pilot, uint32_t lower1, uint32_t lower2, uint32_t partitionSize) const {
            uint64_t M = fastmod::computeM_u32(partitionSize); //ToD: cache M for all possibel partition sizes
            uint32_t hashPilot = fastmod::fastdiv_u32(pilot, M);
            uint32_t hashValue = hash(lower1, hash(lower2, hashPilot)) >> 1;
            uint32_t pos = fastmod::fastmod_u32(hashValue + pilot, M, partitionSize);
            return pos;
        }

        inline uint64_t getBucket(uint64_t bucketBits) const {
            uint64_t z = bucketBits * uint64_t(FULCS_INTER - 1);
            uint64_t index = z >> 32;
            uint64_t part = z & 0xFFFFFFFF;
            uint64_t v1 = (config.getFulcs()[index + 0] * part) >> 32;
            uint64_t v2 = (config.getFulcs()[index + 1] * (0xFFFFFFFF - part)) >> 32;
            return (v1 + v2) >> 16;
        }

    public:

        template<typename keyType>
        static inline Key initialHash(keyType keyRaw) {
            return Hasher::hash(keyRaw);
        }

        template<typename keyType>
        inline uint32_t operator()(keyType keyRaw) const {
            Key key = initialHash(keyRaw);
            uint64_t partition = (uint64_t(key.partitioner) * uint64_t(partitions)) >> 32;
            uint64_t bucket = getBucket(key.bucketer);
            uint32_t pilot = pilots.access(partition, bucket);
            uint32_t partitionOffset = partitionOffsets.access(partition);
            uint32_t partitionSize = partitionOffsets.access(partition + 1) - partitionOffset;
            return partitionOffset + hashPos(pilot, key.lower1, key.lower2, partitionSize);
        }


        void setData(const std::vector<uint32_t> &pilots, std::vector<uint32_t> &partitionOffsets, uint32_t partitions,
                     MPHFconfig config) {
            this->config = config;
            this->partitions = partitions;

#pragma omp task
            this->partitionOffsets.encode(partitionOffsets.begin(), config.partitionSize, partitions + 1);
            this->pilots.encode(pilots.begin(), partitions, config.bucketCountPerPartition);
#pragma omp taskwait
        }
    };

}