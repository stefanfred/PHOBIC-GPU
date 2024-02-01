
#include <vector>
#include <iostream>
#include <fstream>
#include <cstring>
#include <sstream>
#include "check.h"

#include "utils.h"

std::string formatSize(uint64_t size)
{
    std::ostringstream oss;
    if (size < 1024)
    {
        oss << size << " B";
    }
    else if (size < 1024 * 1024)
    {
        oss << size / 1024.f << " KB";
    }
    else if (size < 1024 * 1024 * 1024)
    {
        oss << size / (1024.0f * 1024.0f) << " MB";
    }
    else
    {
        oss << size / (1024.0f * 1024.0f * 1024.0f) << " GB";
    }
    return oss.str();
}

uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties, vk::PhysicalDevice &pdevice)
{
    vk::PhysicalDeviceMemoryProperties memProperties = pdevice.getMemoryProperties();
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

void createBuffer(vk::PhysicalDevice &pDevice, vk::Device &device,
                  const vk::DeviceSize &size, vk::BufferUsageFlags usage,
                  vk::MemoryPropertyFlags properties, vk::Buffer &buffer, vk::DeviceMemory &bufferMemory)
{
    vk::BufferCreateInfo inBufferInfo({}, size, usage);
    buffer = CHECK(device.createBuffer(inBufferInfo), "createBuffer failed");

    vk::MemoryRequirements memReq = device.getBufferMemoryRequirements(buffer);
    vk::MemoryAllocateInfo allocInfo(memReq.size,
                                     findMemoryType(memReq.memoryTypeBits, properties, pDevice));

    bufferMemory = CHECK(device.allocateMemory(allocInfo), "createBuffer failed");
    CHECK(device.bindBufferMemory(buffer, bufferMemory, 0U), "createBuffer failed");
}

void createBuffer(vk::PhysicalDevice &pDevice, vk::Device &device,
                  const vk::DeviceSize &size, vk::BufferUsageFlags usage,
                  vk::MemoryPropertyFlags properties, BufferAllocation &buffer) {
    createBuffer(pDevice, device, size, usage, properties, buffer.buffer, buffer.memory);
}

void destroyBuffer(vk::Device &device, BufferAllocation &buffer) {
    device.destroyBuffer(buffer.buffer);
    device.freeMemory(buffer.memory);
}

void copyBuffer(vk::Device &device, vk::Queue &q, vk::CommandPool &commandPool,
                const vk::Buffer &srcBuffer, vk::Buffer &dstBuffer, vk::DeviceSize byteSize)
{
    vk::CommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

    vk::BufferCopy copyRegion(0ULL, 0ULL, byteSize);
    commandBuffer.copyBuffer(srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(device, q, commandPool, commandBuffer);
}

vk::CommandBuffer beginSingleTimeCommands(vk::Device &device, vk::CommandPool &commandPool)
{
    vk::CommandBufferAllocateInfo allocInfo(commandPool, vk::CommandBufferLevel::ePrimary, 1);

    vk::CommandBuffer commandBuffer = CHECK(device.allocateCommandBuffers(allocInfo), "beginSingleTimeCommands failed")[0];

    vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    CHECK(commandBuffer.begin(beginInfo), "beginSingleTimeCommands failed");

    return commandBuffer;
}

void endSingleTimeCommands(vk::Device &device, vk::Queue &q,
                           vk::CommandPool &commandPool, vk::CommandBuffer &commandBuffer)
{
    CHECK(commandBuffer.end(), "endSingleTimeCommands failed");
    vk::SubmitInfo submitInfo(0U, nullptr, nullptr, 1U, &commandBuffer);
    CHECK(q.submit({submitInfo}, nullptr), "endSingleTimeCommands failed");
    CHECK(q.waitIdle(), "endSingleTimeCommands failed");
    device.freeCommandBuffers(commandPool, 1, &commandBuffer);
}
