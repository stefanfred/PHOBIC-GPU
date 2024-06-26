#pragma once

#include "shader_constants.h"
#include "mphf_config.h"
#include "encoders/base/rice_sequence.hpp"
#include "encoders/base/ef_sequence.hpp"
#include "encoders/pilotEncoders/interleaved_encoder.hpp"
#include "encoders/pilotEncoders/interleaved_encoder_dual.hpp"
#include "encoders/base/encoders.hpp"
#include "fastmod/fastmod.h"
#include "hasher.hpp"

namespace phobicgpu {

template <typename PilotEncoder, typename PartitionOffsetEncoder, typename Hasher>
class MPHF {
private:
    std::vector<uint32_t> fulcs;

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

    inline uint32_t hashPos(uint32_t pilot, uint32_t lower1, uint32_t lower2,
                            uint32_t partitionSize) const {
        uint64_t M =
            fastmod::computeM_u32(partitionSize);  // ToDo: cache M for all possibel partition sizes
        uint32_t hashPilot = fastmod::fastdiv_u32(pilot, M);
        uint32_t hashValue = hash(lower1, hash(lower2, hashPilot)) >> 1;
        uint32_t pos = fastmod::fastmod_u32(hashValue + pilot, M, partitionSize);
        return pos;
    }

    inline uint64_t getBucket(uint64_t bucketBits) const {
        uint64_t z = bucketBits * uint64_t(FULCS_INTER - 1);
        uint64_t index = z >> 32;
        uint64_t part = z & 0xFFFFFFFF;
        uint64_t v1 = (fulcs[index + 0] * part) >> 32;
        uint64_t v2 = (fulcs[index + 1] * (0xFFFFFFFF - part)) >> 32;
        return (v1 + v2) >> 16;
    }

public:
    PilotEncoder& getPilotEncoder() {
        return pilots;
    }

    constexpr static bool noHash() {
        return std::is_same_v<Hasher, nohash>;
    }

    template <typename keyType>
    static inline Key initialHash(const keyType& keyRaw) {
        return Hasher::hash(keyRaw);
    }

    void setData(const std::vector<uint32_t>& pilots, std::vector<uint32_t>& partitionOffsets,
                 uint32_t partitions, MPHFconfig config) {
        fulcs = config.getFulcs();
        this->partitions = partitions;

#pragma omp task
        this->partitionOffsets.encode(partitionOffsets.begin(), config.partitionSize,
                                      partitions + 1);
        this->pilots.encode(pilots.begin(), partitions, config.bucketCountPerPartition);
#pragma omp taskwait
    }

    template <typename keyType>
    inline uint32_t operator()(const keyType& keyRaw) const {
        Key key = initialHash(keyRaw);
        uint64_t partition = (uint64_t(key.partitioner) * uint64_t(partitions)) >> 32;
        uint64_t bucket = getBucket(key.bucketer);
        uint32_t pilot = pilots.access(partition, bucket);
        uint32_t partitionOffset = partitionOffsets.access(partition);
        uint32_t partitionSize = partitionOffsets.access(partition + 1) - partitionOffset;
        return partitionOffset + hashPos(pilot, key.lower1, key.lower2, partitionSize);
    }

    float getBitsPerKey() const {
        return float(pilots.num_bits() + partitionOffsets.num_bits() + 32 * fulcs.size() + 32 * partitions) /
               float(partitionOffsets.access(partitions));
    }

    std::string getResultLine() {
        double n = float(partitionOffsets.access(partitions));
        return "total_bits=" + std::to_string(getBitsPerKey()) + " pilot_bits=" + std::to_string(pilots.num_bits() / n) +
               " offsets_bits=" + std::to_string(partitionOffsets.num_bits() / n);
    }
};

}  // namespace gpupthash