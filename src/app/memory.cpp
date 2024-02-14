#include <functional>

#include "app/memory.h"
#include "encoders/base/check.h"

static vk::CommandBuffer allocateBuffer(
        const vk::Device &device,
        const vk::CommandPool &commandPool) {

    vk::CommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = vk::StructureType::eCommandBufferAllocateInfo;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    return CHECK(device.allocateCommandBuffers(allocInfo), "failed to allocate one time command buffer")[0];
}

MemoryAllocator::MemoryAllocator(
        const vk::PhysicalDevice pDevice,
        const vk::Device device,
        const QueueFamilyIndices indices,
        const vk::Queue transferQueue,
        const vk::CommandPool transferCommandPool) :
        pDevice(pDevice),
        queueIndices(indices),
        deviceProperties(pDevice.getMemoryProperties()),
        device(device),
        transferQueue(transferQueue),
        transferCommandPool(transferCommandPool) {}


uint32_t MemoryAllocator::findMemoryType(
        const uint32_t memoryTypeBits,
        const vk::MemoryPropertyFlags properties) const {

    for (uint32_t i = 0; i < deviceProperties.memoryTypeCount; i++) {
        if ((memoryTypeBits & (1 << i)) &&
            (deviceProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}


vk::DeviceMemory MemoryAllocator::allocateMemory(
        const vk::MemoryRequirements &memRequirements,
        const vk::MemoryPropertyFlags flags) const {

    vk::MemoryAllocateInfo allocInfo{};
    allocInfo.sType = vk::StructureType::eMemoryAllocateInfo;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, flags);

    return CHECK(device.allocateMemory(allocInfo),
                 "failed to allocate memory buffer!");
}

vk::DeviceMemory MemoryAllocator::allocateBufferMemory(const vk::Buffer &buffer,
                                                       const vk::MemoryPropertyFlags flags) const {

    vk::MemoryRequirements memRequirements;
    device.getBufferMemoryRequirements(buffer, &memRequirements);
    return allocateMemory(memRequirements, flags);
}

BufferAllocation MemoryAllocator::createBuffer(
        const vk::BufferUsageFlags usage,
        const vk::MemoryPropertyFlags flags,
        const size_t capacity) const {

    vk::Buffer buffer;
    vk::DeviceMemory memory;

    vk::BufferCreateInfo bufferInfo{};
    bufferInfo.sType = vk::StructureType::eBufferCreateInfo;
    bufferInfo.size = capacity;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;

    buffer = CHECK(device.createBuffer(bufferInfo), "create buffer failed");
    memory = allocateBufferMemory(buffer, flags);

    CHECK(device.bindBufferMemory(buffer, memory, 0), "failed to bind buffer memory!");

    return BufferAllocation(memory, 0, capacity, buffer);
}

static void runCommandsSync(
        const vk::Device &device, const vk::CommandPool &pool, const vk::Queue queue,
        std::function<void(const vk::CommandBuffer &)> recorder) {

    vk::CommandBufferBeginInfo beginInfo{};
    beginInfo.sType = vk::StructureType::eCommandBufferBeginInfo;
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

    const vk::CommandBuffer oneTimeBuffer = allocateBuffer(device, pool);

    // record command buffer
    CHECK(oneTimeBuffer.begin(beginInfo), "failed to begin one time command buffer!");
    {
        recorder(oneTimeBuffer);
    }
    CHECK(oneTimeBuffer.end(), "failed to end one time command buffer!");

    // submit command buffer
    vk::SubmitInfo submitInfo{};
    submitInfo.sType = vk::StructureType::eSubmitInfo;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &oneTimeBuffer;
    CHECK(queue.submit(submitInfo), "failed to submit to queue!");

    // wait until queue is processed, this should only be used
    // for initial loading purposes
    CHECK(queue.waitIdle(), "failed to wait for idle queue!");

    // release buffer
    device.freeCommandBuffers(pool, {oneTimeBuffer});
}

AsyncCopyOp::AsyncCopyOp(vk::Device device, vk::Fence fence, vk::CommandPool pool, vk::CommandBuffer buffer)
        : device(device), transferFence(fence), pool(pool), oneTimeBuffer(buffer), pending(true) {}

bool AsyncCopyOp::isReady() const {
    CHECK(pending, "copy operation already completed");

    const vk::Result result = device.getFenceStatus(transferFence);
    if (result == vk::Result::eSuccess) {
        return true;
    } else {
        // if this assertion fails then probably the device is lost (VK_ERROR_DEVICE_LOST)
        CHECK(result == vk::Result::eNotReady, "fence in error state");
        return false;
    }
}

void AsyncCopyOp::complete() {
    CHECK(pending, "copy operation already completed");
    release();
}

void AsyncCopyOp::release() {
    if (pending) {
        device.destroyFence(transferFence);
        device.freeCommandBuffers(pool, {oneTimeBuffer});
        pending = false;
    }
}

static vk::Fence createFence(const vk::Device &device) {
    vk::FenceCreateInfo fenceInfo{};
    fenceInfo.sType = vk::StructureType::eFenceCreateInfo;
    return CHECK(device.createFence(fenceInfo), "failed to create fence");
}

static AsyncCopyOp runCommandsAsync(
        const vk::Device &device, const vk::CommandPool &pool, const vk::Queue queue,
        std::function<void(const vk::CommandBuffer &)> recorder) {

    vk::CommandBufferBeginInfo beginInfo{};
    beginInfo.sType = vk::StructureType::eCommandBufferBeginInfo;
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

    const vk::Fence fence = createFence(device);
    const vk::CommandBuffer oneTimeBuffer = allocateBuffer(device, pool);

    // record command buffer
    CHECK(oneTimeBuffer.begin(beginInfo), "failed to begin one time command buffer!");
    {
        recorder(oneTimeBuffer);
    }
    CHECK(oneTimeBuffer.end(), "failed to end one time command buffer!");

    // submit command buffer
    vk::SubmitInfo submitInfo{};
    submitInfo.sType = vk::StructureType::eSubmitInfo;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &oneTimeBuffer;
    CHECK(queue.submit(submitInfo, fence), "failed to submit to queue!");

    return AsyncCopyOp(device, fence, pool, oneTimeBuffer);
}

AsyncCopyOp MemoryAllocator::runTransferCommandsAsync(std::function<void(const vk::CommandBuffer &)> recorder) const {
    return runCommandsAsync(device, transferCommandPool, transferQueue, recorder);
}

void MemoryAllocator::runTransferCommandsSync(std::function<void(const vk::CommandBuffer &)> recorder) const {
    runCommandsSync(device, transferCommandPool, transferQueue, recorder);
}

BufferAllocation MemoryAllocator::createStagingBuffer(
        const void *srcData,
        const size_t bufferCapacity) const {

    // create staging buffer
    BufferAllocation stagingBuffer = createStagingBuffer(bufferCapacity);

    // copy data to staging buffer
    stagingBuffer.fillWithStagingData(device, srcData, bufferCapacity);
    return stagingBuffer;
}

BufferAllocation MemoryAllocator::createStagingBuffer(
        const size_t bufferCapacity) const {

    // create staging buffer
    BufferAllocation stagingBuffer = createBuffer(vk::BufferUsageFlagBits::eTransferSrc,
                                                  vk::MemoryPropertyFlagBits::eHostVisible |
                                                  vk::MemoryPropertyFlagBits::eHostCoherent,
                                                  bufferCapacity);

    return stagingBuffer;
}

BufferAllocation MemoryAllocator::createDeviceLocalBuffer(
        const size_t bufferCapacity,
        const void *srcData,
        const vk::BufferUsageFlags usage) const {

    // create staging buffer and fill it with data to transfer to GPU
    BufferAllocation stagingBuffer = createStagingBuffer(srcData, bufferCapacity);

    // create device local buffer
    BufferAllocation deviceLocalBuffer = createDeviceLocalBuffer(bufferCapacity, usage);

    // cannot be moved inside the lambda to avoid early destruction
    copyBufferSync(stagingBuffer, deviceLocalBuffer);

    // destroy staging buffer
    stagingBuffer.free(*this);

    return deviceLocalBuffer;
}

void MemoryAllocator::copyBufferSync(
        const BufferAllocation src,
        const BufferAllocation dst) const {

    // cannot be moved inside the lambda to avoid early destruction
    vk::BufferCopy copyRegion{};
    copyRegion.srcOffset = 0; // Optional
    copyRegion.dstOffset = 0; // Optional
    copyRegion.size = src.capacity;

    runTransferCommandsSync([&](const vk::CommandBuffer &commands) {
        commands.copyBuffer(src.buffer, dst.buffer, 1, &copyRegion);
    });
}

AsyncCopyOp MemoryAllocator::copyBufferAync(
        const BufferAllocation src,
        const BufferAllocation dst) const {

    // cannot be moved inside the lambda to avoid early destruction
    vk::BufferCopy copyRegion{};
    copyRegion.srcOffset = 0; // Optional
    copyRegion.dstOffset = 0; // Optional
    copyRegion.size = src.capacity;

    return runTransferCommandsAsync([&](const vk::CommandBuffer &commands) {
        commands.copyBuffer(src.buffer, dst.buffer, 1, &copyRegion);
    });
}

BufferAllocation MemoryAllocator::createDeviceLocalBuffer(const size_t bufferCapacity,
                                                          const vk::BufferUsageFlags usage
) const {

    // create device local buffer
    BufferAllocation deviceLocalBuffer = createBuffer(vk::BufferUsageFlagBits::eStorageBuffer | usage,
                                                      vk::MemoryPropertyFlagBits::eDeviceLocal, bufferCapacity);

    return deviceLocalBuffer;
}

void BufferAllocation::fillWithStagingData(const vk::Device &device, const void *src, size_t len) const {
    CHECK(len <= capacity, "buffer overflow detected!");

    void *dst = CHECK(device.mapMemory(memory, offset, len),
                      "unable to map memory!");
    {
        std::memcpy(dst, src, len);
    }
    device.unmapMemory(memory);

}

void BufferAllocation::free(const MemoryAllocator &allocator) const {
    allocator.device.freeMemory(memory);
    allocator.device.destroyBuffer(buffer);
}
