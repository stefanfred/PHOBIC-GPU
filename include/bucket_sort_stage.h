#pragma once

#include "app.h"
#include "command_buffer.h"


struct PushStructBucketSort
{
	uint32_t partitionMaxSize;
};

class BucketSortStage {
private:
	App& app;
	const ShaderStage* bucketSortStage;

public:
	BucketSortStage(App& app, uint32_t bucketCountPerPartition, uint32_t sortingBins);

	void addCommands(CommandBuffer* cb, PushStructBucketSort constants, vk::Buffer bucketSizes, size_t partitions, vk::Buffer debug, vk::Buffer histo, vk::Buffer partitionsSizes, vk::Buffer bucketSortPermutation);
};