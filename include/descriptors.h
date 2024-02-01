#pragma once

#include <vector>

#include "vulkan_api.h"
#include "check.h"


namespace descr {
	static vk::DescriptorSetLayoutBinding uniformBufferBinding(const uint32_t binding, const uint32_t count = 1) {
		vk::DescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = binding;
		uboLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
		uboLayoutBinding.descriptorCount = count;
		uboLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eCompute | vk::ShaderStageFlagBits::eAllGraphics;
		uboLayoutBinding.pImmutableSamplers = nullptr; // Optional
		return uboLayoutBinding;
	}
	static vk::DescriptorSetLayoutBinding combinedImageSamplerBinding(const uint32_t binding, const uint32_t count = 1) {
		vk::DescriptorSetLayoutBinding samplerLayoutBinding{};
		samplerLayoutBinding.binding = binding;
		samplerLayoutBinding.descriptorCount = count;
		samplerLayoutBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		samplerLayoutBinding.pImmutableSamplers = nullptr;
		samplerLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eCompute;
		return samplerLayoutBinding;
	}
	static vk::DescriptorSetLayoutBinding storageBinding(const uint32_t binding, const uint32_t count = 1) {
		return vk::DescriptorSetLayoutBinding(
			binding,                                          // The binding number of this entry
			vk::DescriptorType::eStorageBuffer,               // Type of resource descriptors used for this binding
			count,                                               // Number of descriptors contained in the binding
			vk::ShaderStageFlagBits::eCompute |				  // All defined shader stages can access the resource
			vk::ShaderStageFlagBits::eAllGraphics
		);
	}
}

class DescriptorSetAllocation;

class DescriptorAllocator {
private:
	std::vector<vk::DescriptorPool> pools;
	size_t poolIndex = 0;
	size_t currentPoolOffset = 0;
public:
	vk::Device device;

	DescriptorAllocator() {};
	DescriptorAllocator(const vk::Device& device);
	DescriptorSetAllocation alloc(const vk::DescriptorSetLayout& layout);
	const std::vector<DescriptorSetAllocation> alloc(const vk::DescriptorSetLayout& layout, const size_t count);
	void reset();
	void destroy();

	static vk::DescriptorSetLayout createLayout(
		const vk::Device& device,
		const std::vector<vk::DescriptorSetLayoutBinding>& bindings);

	static std::vector<vk::DescriptorSetLayout> createLayouts(
		const vk::Device& device,
		const std::vector<std::vector<vk::DescriptorSetLayoutBinding>>& layouts);
};

class DescriptorSetAllocation {
private:
	DescriptorAllocator* allocator;
public:
	vk::DescriptorSet set;
	vk::DescriptorSetLayout layout;
public:
	DescriptorSetAllocation() : allocator(nullptr) {}
	DescriptorSetAllocation(DescriptorAllocator* allocator, vk::DescriptorSet set, vk::DescriptorSetLayout layout);
	void updateUniformBuffer(const uint32_t binding, const vk::Buffer& uniformBuffer, const uint32_t arrayIndex = 0) const;
	void updateStorageBuffer(const uint32_t binding, const vk::Buffer& storageBuffer, const uint32_t arrayIndex = 0) const;
};
