#pragma once

#include "vulkan_api.h"
#include "app/memory.h"
#include "encoders/base/check.h"

template<typename BUFFER>
class UniformBuffer {
private:
    BufferAllocation uboBufferAllocation;
    const bool isDynamic;
public:
    // static uniform buffer
    UniformBuffer(const MemoryAllocator &memoryAlloc, const BUFFER &ubo) : isDynamic(false) {
        uboBufferAllocation = memoryAlloc.createDeviceLocalBuffer(vk::BufferUsageFlagBits::eUniformBuffer, &ubo,
                                                                  sizeof(BUFFER));
    }

    // dynamic uniform buffer
    UniformBuffer(const MemoryAllocator &memoryAlloc) : isDynamic(true) {
        uboBufferAllocation = memoryAlloc.createBuffer(vk::BufferUsageFlagBits::eUniformBuffer,
                                                       vk::MemoryPropertyFlagBits::eHostVisible |
                                                       vk::MemoryPropertyFlagBits::eHostCoherent,
                                                       sizeof(BUFFER));
    }

    const void update(const vk::Device &device, const BUFFER &data) const {
        uboBufferAllocation.fillWithStagingData(device, &data, sizeof(BUFFER));
    }

    const vk::Buffer &handle() const { return uboBufferAllocation.buffer; }

    void destroy(const MemoryAllocator &alloc) const {
        uboBufferAllocation.free(alloc);
    }
};

