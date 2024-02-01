#pragma once

#include "app.h"
#include "bucket_sizes_stage.h"
#include "bucket_sort_stage.h"
#include "prefix_sum_stage.h"
#include "redistribute_keys_stage.h"
#include "apply_partition_offset.h"
#include "mphf_config.h"
#include "search_stage.h"
#include "mphf.hpp"


class MPHFbuilder {
private:
	class BuildInvocation {
		MPHFInterface* f;
		const std::vector<Key>& keys;
		uint32_t size;
		uint32_t partitions;
		MPHFconfig config;
		App& app;
		MPHFbuilder& builder;

		CommandBuffer* cb;
		uint32_t totalBucketCount;


		BufferAllocation debugBuffer;

		BufferAllocation keysSrc;

		BufferAllocation keysLowerDst;

		BufferAllocation bucketSizes;
		BufferAllocation bucketSizeHistogram;
		BufferAllocation keyOffsets;
		BufferAllocation partitionsSizes;
		BufferAllocation partitionsOffsets;
		BufferAllocation bucketPermuatation;
		BufferAllocation pilotsDevice;
		BufferAllocation pilotsHost;
		BufferAllocation fulcrums;

		BufferAllocation dictKeys;
		BufferAllocation maxPilot;

		BufferAllocation pilotLimitPilotCompact;
		BufferAllocation pilotLimitIterations;
		BufferAllocation pilotLimitMaxIter;
		BufferAllocation pilotLimitIterCompact;

		PrefixSumData ppsData;

		std::vector<uint32_t> keysUpperHost;
		std::vector<uint32_t> keysLowerHost;
		std::vector<uint32_t> partitionsOffsetsHost;

	public:
		BuildInvocation(MPHFInterface* f, const std::vector<Key>& keys, uint32_t size, MPHFconfig config, App& app, MPHFbuilder& builder) : f(f), keys(keys), size(size), config(config), app(app), builder(builder) {
			size = keys.size();
			partitions = (size + config.partitionSize - 1) / config.partitionSize;
		}

		void destroy();
		void allocateBuffers();
		void addGpuCommands();
		bool run();
	};

	MPHFconfig config;
	App& app;

	BucketSizesStage bucketSizesStage;
	BucketSortStage bucketSortStage;
	RedistributeKeysStage redistributeKeysStage;
	SearchStage searchStage;
	PrefixSumStage partitionOffsetPPSStage;
	PartitionOffsetStage applyPartitionOffsetStage;
public:
	MPHFbuilder(App& app, MPHFconfig config);
	bool build(const std::vector<Key>& keys, MPHFInterface* f);
	bool buildRandomized(size_t size, MPHFInterface* f, uint64_t seed);

};