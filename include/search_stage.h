#pragma once

#include "app.h"
#include "command_buffer.h"
#include "shader_constants.h"
#include "mphf_config.h"

struct PushStructSearch
{
	uint32_t partitions;
};

class SearchStage {
private:
	App& app;
	const ShaderStage* searchStage;

	uint32_t workGroupSize;


public:
	SearchStage(App& app, uint32_t workGroupSize, MPHFconfig config);
	void addCommands(CommandBuffer* cb, uint32_t partitions,
		vk::Buffer keys, vk::Buffer bucketSizeHisto, vk::Buffer partitionSizes,
		vk::Buffer bucketPermuatation, vk::Buffer pilots, vk::Buffer partitionsOffsets, vk::Buffer debug);
	void destroy();
};