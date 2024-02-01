#pragma once

#include "app.h"
#include "command_buffer.h"
#include <vector>

struct PrefixSumData {
public:
	void destroy(const MemoryAllocator& allocator);

private:
	uint32_t totalSize;

	std::vector<BufferAllocation> buffers;
	std::vector<DescriptorSetAllocation> descriptors;
	std::vector<DescriptorSetAllocation> offsetDescriptors;

	friend class PrefixSumStage;
};

class PrefixSumStage {
private:
	App& app;

	uint32_t workGroupSizeLocal;
	uint32_t workGroupSizeOffset;


	const ShaderStage* localStage;
	const ShaderStage* offsetStage;

	struct PushStructPPS
	{
		uint32_t size;
	};
public:
	PrefixSumStage(App& app, uint32_t workGroupSizeLocal, uint32_t workGroupSizeOffset);

	void addCommands(CommandBuffer* cb, uint32_t totalSize, vk::Buffer work, PrefixSumData &data);
};