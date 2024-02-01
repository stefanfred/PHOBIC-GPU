#pragma once

#include "shader_constants.h"
#include "mphf_config.h"
#include "encoders/golomb_sequence.hpp"
#include "encoders/ef_sequence.hpp"
#include "encoders/ortho_encoder.hpp"
#include "encoders/ortho_encoder_dual.hpp"
#include "encoders/encoders.hpp"
#include "../external/fastmod/fastmod.h"

using namespace pthash;

struct PartitionData {
	uint32_t offset;
	uint32_t size;
};

struct Key {
	uint32_t partitioner;
	uint32_t bucketer;
	uint32_t lower1;
	uint32_t lower2;
};

class MPHFInterface {

	public:

	virtual void setData(const std::vector<uint32_t> &pilots, std::vector<uint32_t> &partitionOffsets, uint32_t partitions, MPHFconfig config) = 0;

	virtual uint32_t operator()(Key key) const = 0;
};

template <typename PilotEncoder, typename PartitionOffsetEncoder>
class MPHF : public MPHFInterface {
public:

	double bitsPerKey;
	double constrTimePerKeyNs;
	double queryTimePerKeyNs;

private:
	MPHFconfig config;

	std::vector<uint32_t> debug;

	uint32_t partitions;
	PilotEncoder pilots;
	PartitionOffsetEncoder partitionOffsets;

	uint32_t hash(uint32_t pilot, uint32_t key) const {
		key ^= pilot;
		key = ((key >> 16) ^ key) * 0x45d9f3b;
		key = ((key >> 16) ^ key) * 0x45d9f3b;
		key = (key >> 16) ^ key;
		return key;
	}

	uint32_t hashPos(uint32_t pilot, uint32_t lower1, uint32_t lower2, uint32_t partitionSize) const {
		uint64_t M = fastmod::computeM_u32(partitionSize);
		uint32_t hashPilot = fastmod::fastdiv_u32(pilot, M);
		uint32_t hashValue = hash(lower1, hash(lower2, hashPilot)) >> 1;
		uint32_t pos = fastmod::fastmod_u32(hashValue + pilot, M, partitionSize);
		return pos;
	}

	uint64_t getBucket(uint64_t bucketBits) const {
		uint64_t z = bucketBits * uint64_t(FULCS_INTER - 1);
		uint64_t index = z >> 32;
		uint64_t part = z & 0xFFFFFFFF;
		uint64_t v1 = (config.getFulcs()[index + 0] * part) >> 32;
		uint64_t v2 = (config.getFulcs()[index + 1] * (0xFFFFFFFF - part))>>32;
		return (v1 + v2)>>16;
	}

	PartitionData getPartitionData(uint32_t partition) const {
		uint32_t offset = 0;
		uint32_t offsetNext = partitionOffsets.access(partition);
		return PartitionData{ offset, offsetNext - offset };
	}
	
public:

	uint32_t operator()(Key key) const
	{
		uint64_t partition = (uint64_t(key.partitioner) * uint64_t(partitions)) >> 32;
		uint64_t bucket = getBucket(key.bucketer);
		uint32_t pilot = pilots.access(partition, bucket);
		PartitionData partitionData = getPartitionData(partition);
		return partitionData.offset + hashPos(pilot, key.lower1, key.lower2, partitionData.size);
	}

	std::vector<uint32_t> getDebug() const {
		return debug;
	}


	void setData(const std::vector<uint32_t> &pilots, std::vector<uint32_t> &partitionOffsets, uint32_t partitions, MPHFconfig config) {
		this->config=config;
		this->partitions=partitions;

        #pragma omp task
		this->partitionOffsets.encode(partitionOffsets.begin(), partitions);
		this->pilots.encode(pilots.begin(), partitions, config.bucketCountPerPartition);
	}
};
