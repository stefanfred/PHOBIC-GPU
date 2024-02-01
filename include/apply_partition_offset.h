#pragma once

#include "app.h"
#include "command_buffer.h"



class PartitionOffsetStage {
private:
	App& app;
	const ShaderStage* partitionOffsetStage;

	uint32_t workGroupSize;

public:
	PartitionOffsetStage(App& app, uint32_t workGroupSize, uint32_t bucketCountPerPartition);

	void addCommands(CommandBuffer* cb, uint32_t partitions, vk::Buffer bucketOffsets, vk::Buffer partitionOffsets);
};