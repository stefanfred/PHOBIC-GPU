#include "command_buffer.h"
#include "check.h"

static vk::CommandBuffer allocatePrimaryBuffer(const vk::Device& device, const vk::CommandPool& commandPool) {
	vk::CommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = vk::StructureType::eCommandBufferAllocateInfo;
	allocInfo.commandPool = commandPool;
	allocInfo.level = vk::CommandBufferLevel::ePrimary; // not submitted to any other command buffer
	allocInfo.commandBufferCount = 1;

	return CHECK(device.allocateCommandBuffers(allocInfo), "no buffer allocated")[0];
}

static vk::Fence createFence(const vk::Device& device) {
	vk::FenceCreateInfo fenceInfo{};
	fenceInfo.sType = vk::StructureType::eFenceCreateInfo;
	// create fence in signaled state to prevent deadlock when
	// rendering the first frame
	fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;
	return CHECK(device.createFence(fenceInfo), "failed to create fence");
}

static vk::Semaphore createSemaphore(const vk::Device& device) {
	vk::SemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = vk::StructureType::eSemaphoreCreateInfo;
	return CHECK(device.createSemaphore(semaphoreInfo), "failed to create semaphore");
}

CommandBuffer::CommandBuffer(const vk::Device& device, const vk::CommandPool& commandPool) :
	primaryBuffer(allocatePrimaryBuffer(device, commandPool)),
	descrAlloc(DescriptorAllocator(device))
{
}

void CommandBuffer::destroy(const vk::Device& device, const vk::CommandPool& commandPool) {
	// release allocator
	descrAlloc.destroy();

	device.destroyQueryPool(timestampPool);
}

void CommandBuffer::bindComputePipeline(const vk::Pipeline& pipeline) {
	primaryBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, pipeline);
}

// overloads for convenience
void CommandBuffer::bindComputePipeline(const Pipeline& pipeline) { bindComputePipeline(pipeline.handle()); }

void CommandBuffer::pushComputePushConstants(const vk::PipelineLayout& layout, const void* data, const uint32_t len) {
	primaryBuffer.pushConstants(layout, vk::ShaderStageFlagBits::eCompute, 0, len, data);
}

void CommandBuffer::dispatch(const uint32_t groupCountX, const uint32_t groupCountY, const uint32_t groupCountZ) {
	primaryBuffer.dispatch(groupCountX, groupCountY, groupCountZ);
}

void CommandBuffer::pipelineBarrier(vk::PipelineStageFlags srcStageMask,
	vk::PipelineStageFlags dstStageMask,
	vk::DependencyFlags dependencyFlags,
	vk::ArrayProxy<const vk::MemoryBarrier> const& memoryBarriers,
	vk::ArrayProxy<const vk::BufferMemoryBarrier> const& bufferMemoryBarriers,
	vk::ArrayProxy<const vk::ImageMemoryBarrier> const& imageMemoryBarriers) {

	primaryBuffer.pipelineBarrier(srcStageMask, dstStageMask, dependencyFlags, memoryBarriers, bufferMemoryBarriers, imageMemoryBarriers);
}

void CommandBuffer::readWritePipelineBarrier() {
	pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, vk::DependencyFlags(), { vk::MemoryBarrier(vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eShaderRead) }, {}, {});
}

void CommandBuffer::bindComputeDescriptorSet(const vk::PipelineLayout& layout, const vk::DescriptorSet& set, const uint32_t setNumber) {
	primaryBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, layout, setNumber,
		{ set }, {});
}

// overloads for convenience
void CommandBuffer::bindComputeDescriptorSet(const Pipeline& pipeline, const DescriptorSetAllocation& set, const uint32_t setNumber) { bindComputeDescriptorSet(pipeline.layout(), set.set, setNumber); }


void CommandBuffer::submit(const vk::Device& device, const vk::Queue& queue) {
	primaryBuffer.end();
	CHECK(queue.submit( { vk::SubmitInfo(0, nullptr, nullptr, 1, &primaryBuffer) }), "failed to submit to queue!");
}

void CommandBuffer::begin() {
	vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	primaryBuffer.begin(beginInfo);
	primaryBuffer.resetQueryPool(timestampPool, 0, timestampsInfo.size());
}


void CommandBuffer::attachTimestamps(const vk::Device& device, TimestampCreateInfo createInfo) {
	// reset
	//device.destroyQueryPool(timestampPool);

	timestampsInfo = createInfo;
	vk::QueryPoolCreateInfo poolCreateInfo({}, vk::QueryType::eTimestamp, createInfo.size());
	timestampPool = CHECK(device.createQueryPool(poolCreateInfo), "createQueryPool failed");
}


void CommandBuffer::writeTimeStamp(TimestampHandle handle) {
	primaryBuffer.writeTimestamp(handle.info.flags, timestampPool, handle.id);
}

void CommandBuffer::readTimestamps(const vk::Device& device, const vk::PhysicalDevice& pDevice) {
	uint64_t* timestamps = new uint64_t[timestampsInfo.size()];// | vk::QueryResultFlagBits::eWait
	CHECK(device.getQueryPoolResults(timestampPool, 0, timestampsInfo.size(), timestampsInfo.size() * sizeof(uint64_t), timestamps, sizeof(uint64_t), vk::QueryResultFlagBits::e64 | vk::QueryResultFlagBits::eWait), "getQueryPoolResults failed");

	vk::PhysicalDeviceProperties properties = pDevice.getProperties();
	std::vector<TimestampResult> res;
	for (uint32_t i = 0; i < timestampsInfo.size(); i++) {
		res.push_back(TimestampResult{ TimestampHandle{timestampsInfo.get(i), i}, properties.limits.timestampPeriod * (timestamps[i] - timestamps[0]) });
	}
	lastRead = res;
	delete[] timestamps;
}

void CommandBuffer::copyBuffer(vk::Buffer src, vk::Buffer dst, vk::DeviceSize byteSize) {
	vk::BufferCopy copyRegion(0ULL, 0ULL, byteSize);
	primaryBuffer.copyBuffer(src, dst, 1, &copyRegion);
}

std::vector<TimestampResult> CommandBuffer::getTimestamps() {
	return lastRead;
}

TimestampInfo TimestampCreateInfo::get(uint32_t index) {
	return timestamps[index];
}

TimestampHandle TimestampCreateInfo::addTimestamp(TimestampInfo timestamp) {
	timestamps.push_back(timestamp);
	return TimestampHandle{ timestamp, (uint32_t) timestamps.size() - 1 };
}


uint32_t TimestampCreateInfo::size() {
	return timestamps.size();
}