#include "pipeline.h"
#include "check.h"


PipelineBuilder::PipelineBuilder(const Shader* shader)
    : shader(shader) { }

void PipelineBuilder::addPushConstantRange(const vk::PushConstantRange& range) {
    pushConstantRanges.push_back(range);
}

ComputePipelineBuilder::ComputePipelineBuilder(const Shader* shader)
    : PipelineBuilder(shader) { }

Pipeline ComputePipelineBuilder::build(const vk::Device& device) const {
    return buildSpecialization(device, {}, nullptr, 0);
}

Pipeline ComputePipelineBuilder::buildSpecialization(const vk::Device& device,
    const std::vector<vk::SpecializationMapEntry>& entries, const void* specData, const uint32_t specDataSize) const {

    // create pipeline layout to specify uniforms
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = vk::StructureType::ePipelineLayoutCreateInfo;
    pipelineLayoutInfo.setLayoutCount = descriptorSetLayouts.size();
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = pushConstantRanges.size();
    pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();

    const vk::PipelineLayout pipelineLayout = CHECK(device.createPipelineLayout(pipelineLayoutInfo),
        "failed to create pipeline layout");

    const std::vector<vk::PipelineShaderStageCreateInfo> stages = shader->shaderStages();

    CHECK(stages.size() == 1, "there must only be one shader stage for compute pipelines");

    vk::PipelineShaderStageCreateInfo computeShaderStageInfo = stages[0];
    vk::SpecializationInfo info;

    // optional specialization constants
    if (entries.size() > 0) {
        info.dataSize = specDataSize;
        info.pData = specData;
        info.mapEntryCount = entries.size();
        info.pMapEntries = entries.data();
        computeShaderStageInfo.pSpecializationInfo = &info;
    }

    // create actual pipeline
    vk::ComputePipelineCreateInfo computeInfo(vk::PipelineCreateFlags(), computeShaderStageInfo, pipelineLayout);

    const vk::Pipeline computePipeline = CHECK(device.createComputePipeline(VK_NULL_HANDLE, computeInfo),
        "unable to create compute pipeline");

    return Pipeline(nullptr, pipelineLayout, computePipeline);
}

void PipelineBuilder::addDescriptorSetLayout(const vk::DescriptorSetLayout& layout) {
    descriptorSetLayouts.push_back(layout);
}


Pipeline::Pipeline(
    const PipelineBuilder* builder,
    const vk::PipelineLayout pipelineLayout,
    const vk::Pipeline graphicsPipeline) : 
            builder(builder),
            pipelineLayout(pipelineLayout), 
            pipelineHandle(graphicsPipeline) {}


void Pipeline::destroy(const vk::Device& device) {
    device.destroyPipeline(pipelineHandle);
    device.destroyPipelineLayout(pipelineLayout);
}