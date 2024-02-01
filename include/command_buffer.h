#pragma once

#include "vulkan_api.h"
#include "descriptors.h"
#include "pipeline.h"

struct TimestampInfo {
	std::string name;
	vk::PipelineStageFlagBits flags = vk::PipelineStageFlagBits::eAllCommands;
};

struct TimestampHandle {
	TimestampInfo info;
	uint32_t id;
};

struct TimestampResult {
	TimestampHandle handle;
	float time;
};

class TimestampCreateInfo {
public:
	TimestampHandle addTimestamp(TimestampInfo timestamp);
	TimestampInfo get(uint32_t index);
	uint32_t size();
private:
	std::vector<TimestampInfo> timestamps;
};

class CommandBuffer {
private:
	vk::CommandBuffer primaryBuffer;

	// timestamps
	vk::QueryPool timestampPool;
	TimestampCreateInfo timestampsInfo;
	std::vector<TimestampResult> lastRead;

public:
	DescriptorAllocator descrAlloc;

	CommandBuffer(const vk::Device& device, const vk::CommandPool& commandPool);

	void bindComputeDescriptorSet(const vk::PipelineLayout& layout, const vk::DescriptorSet& set, const uint32_t setNumber = 0);
	void bindComputeDescriptorSet(const Pipeline& pipeline, const DescriptorSetAllocation& set, const uint32_t setNumber = 0);

	// push constant support
	void pushComputePushConstants(const vk::PipelineLayout& layout, const void* data, const uint32_t len);
	template<typename PUSH_CONST>
	inline void pushComputePushConstants(const Pipeline& pipeline, const PUSH_CONST& c) {
		pushComputePushConstants(pipeline.layout(), &c, sizeof(PUSH_CONST));
	}

	// memory barrier support
	void pipelineBarrier(vk::PipelineStageFlags srcStageMask,
		vk::PipelineStageFlags dstStageMask,
		vk::DependencyFlags dependencyFlags,
		vk::ArrayProxy<const vk::MemoryBarrier> const& memoryBarriers,
		vk::ArrayProxy<const vk::BufferMemoryBarrier> const& bufferMemoryBarriers,
		vk::ArrayProxy<const vk::ImageMemoryBarrier> const& imageMemoryBarriers);
	void readWritePipelineBarrier();

	void bindComputePipeline(const vk::Pipeline& computePipeline);
	void bindComputePipeline(const Pipeline& computePipeline);

	void dispatch(const uint32_t groupCountX, const uint32_t groupCountY = 1, const uint32_t groupCountZ = 1);
	void submit(const vk::Device& device, const vk::Queue& queue);
	void destroy(const vk::Device& device, const vk::CommandPool& commandPool);

	void begin();

	void attachTimestamps(const vk::Device& device, TimestampCreateInfo createInfo);
	void writeTimeStamp(TimestampHandle handle);
	void readTimestamps(const vk::Device& device, const vk::PhysicalDevice& pDevice);
	std::vector<TimestampResult> getTimestamps();

	void copyBuffer(vk::Buffer src, vk::Buffer dst, vk::DeviceSize byteSize);
};
