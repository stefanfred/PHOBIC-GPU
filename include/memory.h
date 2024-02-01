#pragma once

#include <functional>
#include "vulkan_api.h"
#include "queue_family.h"

class MemoryAllocator;

class MemoryAllocation {
public:
	vk::DeviceSize offset;
	vk::DeviceSize capacity;
	vk::DeviceMemory memory;

	MemoryAllocation() : offset(0), capacity(0) {};
	MemoryAllocation(
		vk::DeviceMemory mem, vk::DeviceSize offset, vk::DeviceSize capacity) :
			memory(mem), offset(offset), capacity(capacity) {};

	virtual void free(const MemoryAllocator& allocator) const = 0;
};

class BufferAllocation : public MemoryAllocation {
public:
	vk::Buffer buffer;
	BufferAllocation* uploadStaging;
	BufferAllocation* dwonloaddStaging;

	BufferAllocation() : MemoryAllocation() {};
	BufferAllocation(
		vk::DeviceMemory mem,  vk::DeviceSize offset, vk::DeviceSize capacity, vk::Buffer buf) :
		MemoryAllocation(mem, offset, capacity), buffer(buf) {};

	virtual void free(const MemoryAllocator& allocator) const override;
	void fillWithStagingData(const vk::Device& device, const void* src, size_t len) const;
};

class AsyncCopyOp {
private:
	vk::Device device;
	vk::Fence transferFence;
	vk::CommandPool pool;
	vk::CommandBuffer oneTimeBuffer;
	bool pending;
public:
	AsyncCopyOp() : pending(false) {} // default constructor
	AsyncCopyOp(vk::Device device, vk::Fence fence, vk::CommandPool pool, vk::CommandBuffer buffer);
	bool isReady() const;
	void complete();
	void release();
};

class MemoryAllocator {
private:
	vk::Queue transferQueue;
	vk::CommandPool transferCommandPool;
	vk::PhysicalDeviceMemoryProperties deviceProperties;
	QueueFamilyIndices queueIndices;
public:
	vk::Device device;
	vk::PhysicalDevice pDevice;

	MemoryAllocator(
		const vk::PhysicalDevice pDevice, 
		const vk::Device device,
		const QueueFamilyIndices indices,
		const vk::Queue transferQueue,
		const vk::CommandPool transferCommandPool);

	// default constructor
	MemoryAllocator() :
		pDevice(vk::PhysicalDevice()),
		device(vk::Device()),
		queueIndices(QueueFamilyIndices()),
		transferQueue(vk::Queue()),
		transferCommandPool(vk::CommandPool()),
		deviceProperties(vk::PhysicalDeviceMemoryProperties()) {}

private:
	uint32_t findMemoryType(
		const uint32_t memoryTypeBits,
		const vk::MemoryPropertyFlags properties) const;

	vk::DeviceMemory allocateMemory(
		const vk::MemoryRequirements& memRequirements,
		const vk::MemoryPropertyFlags flags) const;

    vk::DeviceMemory allocateBufferMemory(
		const vk::Buffer& buffer,
		const vk::MemoryPropertyFlags flags) const;
	
public:
	// 
	// Creates a new exclusive buffer and allocates memory  for it
	// usually createStagingBuffer() or createDeviceLocalBuffer() should be
	// used instead.
	// @see BufferAllocation#free()
	//
	BufferAllocation createBuffer(
		const vk::BufferUsageFlags usage,
		const vk::MemoryPropertyFlags flags,
		const size_t capacity) const;

	//
	// Creates a staging buffer, maps it to device memory and writes 
	// the given data into it.
	// @see BufferAllocation#free()
	//
	BufferAllocation createStagingBuffer(
		const void* srcData,
		const size_t bufferCapacity) const;


	BufferAllocation createStagingBuffer(
		const size_t bufferCapacity) const;

	// 
	// Creates a device local buffer and fills it with the given data
	// using a temporary staging buffer.
	// @see BufferAllocation#free()
	//
	BufferAllocation createDeviceLocalBuffer(
		const size_t bufferCapacity,
		const void* srcData,
		const vk::BufferUsageFlags usage = vk::BufferUsageFlagBits()) const;

	BufferAllocation createDeviceLocalBuffer(
		const size_t bufferCapacity,
		const vk::BufferUsageFlags usage = vk::BufferUsageFlagBits()) const;


	//
	// Copies the buffer to the destination buffer.
	//
	void copyBufferSync(
		const BufferAllocation src,
		const BufferAllocation dst) const;

	AsyncCopyOp copyBufferAync(
		const BufferAllocation src,
		const BufferAllocation dst) const;

	//
	// Simple helper function to submit eOneTimeSubmit commands to the transfer queue.
	//
	void runTransferCommandsSync(std::function<void(const vk::CommandBuffer&)> recorder) const;
	AsyncCopyOp runTransferCommandsAsync(std::function<void(const vk::CommandBuffer&)> recorder) const;
};
