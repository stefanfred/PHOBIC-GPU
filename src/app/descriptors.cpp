#include <vector>

#include "app/uniform_buffer.h"
#include "app/descriptors.h"
#include "encoders/base/check.h"

// TODO:
// for each frame we create a set of descriptor pool
// when an allocation fails, a new descriptor pool is created
// after the frame is submitted and we have waited for the fence
// all descriptor pools are reset

static const uint32_t MAX_DESCRIPTOR_SETS_PER_POOL = 100;

static vk::DescriptorPool
createDescriptorPool(const vk::Device &device, const std::vector<vk::DescriptorType> &descriptorTypes) {
    std::vector<vk::DescriptorPoolSize> poolSizes;

    for (const vk::DescriptorType &type: descriptorTypes) {
        vk::DescriptorPoolSize poolSize{};
        poolSize.type = type;
        poolSize.descriptorCount = 1;
        poolSizes.push_back(poolSize);
    }

    vk::DescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = vk::StructureType::eDescriptorPoolCreateInfo;
    poolInfo.poolSizeCount = poolSizes.size();
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = MAX_DESCRIPTOR_SETS_PER_POOL;

    return CHECK(device.createDescriptorPool(poolInfo), "create descriptor pool failed!");
}

static std::vector<vk::DescriptorSet> allocateDescriptorSets(const vk::Device &device, const size_t count,
                                                             const vk::DescriptorPool &descriptorPool,
                                                             const vk::DescriptorSetLayout &descriptorSetLayout) {

    const std::vector<vk::DescriptorSetLayout> layouts(count, descriptorSetLayout);
    vk::DescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = vk::StructureType::eDescriptorSetAllocateInfo;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = count;
    allocInfo.pSetLayouts = layouts.data();

    return CHECK(device.allocateDescriptorSets(allocInfo), "failed to allocate descriptor sets!");
}


std::vector<vk::DescriptorSetLayout> DescriptorAllocator::createLayouts(
        const vk::Device &device,
        const std::vector<std::vector<vk::DescriptorSetLayoutBinding>> &bindingsLayouts) {

    std::vector<vk::DescriptorSetLayout> layouts;
    layouts.reserve(bindingsLayouts.size());
    for (const std::vector<vk::DescriptorSetLayoutBinding> &binding: bindingsLayouts) {
        layouts.push_back(DescriptorAllocator::createLayout(device, binding));
    }
    return layouts;
}

vk::DescriptorSetLayout DescriptorAllocator::createLayout(
        const vk::Device &device,
        const std::vector<vk::DescriptorSetLayoutBinding> &bindings) {


    vk::DescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = vk::StructureType::eDescriptorSetLayoutCreateInfo; //  VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = bindings.size();
    layoutInfo.pBindings = bindings.data();

    return CHECK(device.createDescriptorSetLayout(layoutInfo),
                 "failed to create descriptor set layout!");
}

DescriptorSetAllocation::DescriptorSetAllocation(DescriptorAllocator *allocator, vk::DescriptorSet set,
                                                 vk::DescriptorSetLayout layout)
        : allocator(allocator), set(set), layout(layout) {}


void DescriptorSetAllocation::updateUniformBuffer(
        const uint32_t binding,
        const vk::Buffer &uniformBuffer,
        const uint32_t arrayIndex) const {

    // Buffer info and data offset info
    vk::DescriptorBufferInfo bufferInfo(
            uniformBuffer,            // Buffer to get data from
            0ULL,                     // Position of start of data
            VK_WHOLE_SIZE             // Size of data
    );

    // update single uniform buffer
    vk::WriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = vk::StructureType::eWriteDescriptorSet;
    descriptorWrite.dstSet = set;
    descriptorWrite.dstBinding = binding;
    descriptorWrite.dstArrayElement = arrayIndex;
    descriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &bufferInfo;
    descriptorWrite.pImageInfo = nullptr;
    descriptorWrite.pTexelBufferView = nullptr;

    allocator->device.updateDescriptorSets(1U, &descriptorWrite, 0, nullptr);
}

void DescriptorSetAllocation::updateStorageBuffer(
        const uint32_t binding,
        const vk::Buffer &storageBuffer,
        const uint32_t arrayIndex) const {

    // Buffer info and data offset info
    vk::DescriptorBufferInfo bufferInfo(
            storageBuffer,            // Buffer to get data from
            0ULL,                     // Position of start of data
            VK_WHOLE_SIZE             // Size of data
    );

    // Binding index in the shader    V
    vk::WriteDescriptorSet descriptorWrite(set, binding, arrayIndex, 1U,
                                           vk::DescriptorType::eStorageBuffer, nullptr, &bufferInfo);

    allocator->device.updateDescriptorSets(1U, &descriptorWrite, 0U, nullptr);
}


DescriptorAllocator::DescriptorAllocator(const vk::Device &device) : device(device) {
}


DescriptorSetAllocation DescriptorAllocator::alloc(const vk::DescriptorSetLayout &layout) {
    return alloc(layout, 1)[0];
}

const std::vector<DescriptorSetAllocation> DescriptorAllocator::alloc(
        const vk::DescriptorSetLayout &layout,
        const size_t count) {

    std::vector<DescriptorSetAllocation> allocated;
    allocated.reserve(count);

    while (allocated.size() < count) {
        if (poolIndex == pools.size()) {
            // allocate new pool
            // create descriptor pool for uniform buffers and combined image sampler descriptors
            const vk::DescriptorPool pool = createDescriptorPool(device, {
                    vk::DescriptorType::eUniformBuffer,
                    vk::DescriptorType::eCombinedImageSampler
            });

            pools.push_back(pool);
            currentPoolOffset = 0;
        }

        const vk::DescriptorPool &pool = pools[poolIndex];
        const size_t remaining = MAX_DESCRIPTOR_SETS_PER_POOL - currentPoolOffset;
        const uint32_t n = std::min(remaining, count - allocated.size());

        std::vector<vk::DescriptorSet> sets = allocateDescriptorSets(device, n, pool, layout);
        currentPoolOffset += n;

        for (const vk::DescriptorSet &set: sets) {
            allocated.push_back({
                                        this, set, layout
                                });
        }

        if (currentPoolOffset == MAX_DESCRIPTOR_SETS_PER_POOL) {
            poolIndex++;
            currentPoolOffset = 0;
        }
    }

    return allocated;
}

void DescriptorAllocator::reset() {
    for (const vk::DescriptorPool &pool: pools) {
        device.resetDescriptorPool(pool);
    }
}

void DescriptorAllocator::destroy() {
    for (const vk::DescriptorPool &pool: pools) {
        device.destroyDescriptorPool(pool);
    }
    pools.clear();
}
