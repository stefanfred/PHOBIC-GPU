#pragma once

#include <vector>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <filesystem>

#include "memory.h"
#include "vulkan_api.h"
#include "check.h"
#include "host_timer.h"

#define CAST(a) static_cast<uint32_t>(a.size())
typedef uint32_t uint;

template<typename T, typename V>
T ceilDiv(T x, V y) {
    return x / y + (x % y != 0);
}

std::string formatSize(uint64_t size);
uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties, vk::PhysicalDevice &pdevice);
void createBuffer(vk::PhysicalDevice &pDevice, vk::Device &device,
                  const vk::DeviceSize &size, vk::BufferUsageFlags usage,
                  vk::MemoryPropertyFlags properties, vk::Buffer &buffer, vk::DeviceMemory &bufferMemory);
void createBuffer(vk::PhysicalDevice &pDevice, vk::Device &device,
                  const vk::DeviceSize &size, vk::BufferUsageFlags usage,
                  vk::MemoryPropertyFlags properties, BufferAllocation &buffer);
void destroyBuffer(vk::Device &device, BufferAllocation& buffer);
void copyBuffer(vk::Device &device, vk::Queue &q, vk::CommandPool &commandPool,
                const vk::Buffer &srcBuffer, vk::Buffer &dstBuffer, vk::DeviceSize byteSize);

vk::CommandBuffer beginSingleTimeCommands(vk::Device &device, vk::CommandPool &commandPool);
void endSingleTimeCommands(vk::Device &device, vk::Queue &q,
                           vk::CommandPool &commandPool, vk::CommandBuffer &commandBuffer);


template <typename T>
void fillDeviceBuffer(vk::Device& device, vk::DeviceMemory& mem, const T* input, size_t size)
{
    void* data = CHECK(device.mapMemory(mem, 0, size * sizeof(T), vk::MemoryMapFlags()), "fillDeviceBuffer failed");
    memcpy(data, input, static_cast<size_t>(size * sizeof(T)));
    device.unmapMemory(mem);
}

template <typename T>
void fillDeviceBuffer(vk::Device &device, vk::DeviceMemory &mem, const std::vector<T> &input)
{
    fillDeviceBuffer(device, mem, input, input.size());
}

template <typename T>
void fillHostBuffer(vk::Device& device, vk::DeviceMemory& mem, T* pilots, size_t size = 0)
{
    // copy memory from mem to output
    void* data = CHECK(device.mapMemory(mem, 0, size * sizeof(T), vk::MemoryMapFlags()), "fillHostBuffer failed");
    memcpy(pilots, data, static_cast<size_t>(size * sizeof(T)));
    device.unmapMemory(mem);
}

template <typename T>
void fillHostBuffer(vk::Device &device, vk::DeviceMemory &mem, std::vector<T> &pilots)
{
    fillHostBuffer(device, mem, pilots.data(), pilots.size());
}

template <typename T>
void fillDeviceWithStagingBuffer(vk::PhysicalDevice& pDevice, vk::Device& device,
    vk::CommandPool& commandPool, vk::Queue& q,
    BufferAllocation& b, T* data, size_t size)
{
    // Buffer b requires the eTransferSrc bit
    // data (host) -> staging (device) -> Buffer b (device)
    vk::Buffer staging;
    vk::DeviceMemory mem;
    vk::DeviceSize byteSize = size * sizeof(T);

    createBuffer(pDevice, device, byteSize, vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible,
        staging, mem);
    // V host -> staging V
    fillDeviceBuffer<T>(device, mem, data, size);
    // V staging -> buffer V
    copyBuffer(device, q, commandPool, staging, b.buffer, byteSize);
    device.destroyBuffer(staging);
    device.freeMemory(mem);
}

template <typename T>
void fillDeviceWithStagingBufferVec(vk::PhysicalDevice &pDevice, vk::Device &device,
                           vk::CommandPool &commandPool, vk::Queue &q,
                           BufferAllocation &b, const std::vector<T> &data)
{
    fillDeviceWithStagingBuffer(pDevice, device, commandPool, q, b, data.data(), data.size());
}

template <typename T>
void fillHostWithStagingBuffer(vk::PhysicalDevice& pDevice, vk::Device& device,
    vk::CommandPool& commandPool, vk::Queue& q,
    const BufferAllocation& b, T *data, size_t size)
{
    HostTimer s;
    // Buffer b requires the eTransferDst bit
    // Buffer b (device) -> staging (device) -> data (host)
    vk::Buffer staging;
    vk::DeviceMemory mem;
    vk::DeviceSize byteSize = size * sizeof(T);


    createBuffer(pDevice, device, byteSize, vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCached,
        staging, mem);

    // V buffer -> staging V
    copyBuffer(device, q, commandPool, b.buffer, staging, byteSize);
    // V staging -> host V
    fillHostBuffer<T>(device, mem, data, size);
    device.destroyBuffer(staging);
    device.freeMemory(mem);
}


template <typename T>
void fillHostWithStagingBuffer(vk::PhysicalDevice &pDevice, vk::Device &device,
                           vk::CommandPool &commandPool, vk::Queue &q,
                           const BufferAllocation& b, std::vector<T> &data)
{
    fillHostWithStagingBuffer(pDevice, device, commandPool, q, b, data.data(), data.size());
}



template <typename T>
T getBufferValue(vk::PhysicalDevice& pDevice, vk::Device& device,
    vk::CommandPool& commandPool, vk::Queue& q,
    const BufferAllocation& b)
{
    std::vector<T> res;
    res.resize(1);
    fillHostWithStagingBuffer(pDevice, device, commandPool, q, b, res);
    return res[0];
}