#pragma once

#include <memory>
#include <vector>

#include "vulkan_api.h"
#include "shader.h"

namespace PushConstants {
    template<typename PUSH_CONST>
    static std::vector<vk::PushConstantRange> ofStruct() {
        vk::PushConstantRange range;
        range.offset = 0;
        range.size = sizeof(PUSH_CONST);
        range.stageFlags = vk::ShaderStageFlagBits::eCompute;

        std::vector<vk::PushConstantRange> vec;
        vec.reserve(1);
        vec.push_back(range);
        return vec;
    }
}

class Pipeline;

class PipelineBuilder {
protected:
    const Shader *shader;
    std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;
    std::vector<vk::PushConstantRange> pushConstantRanges;
public:
    PipelineBuilder(const Shader *shader);

    void addDescriptorSetLayout(const vk::DescriptorSetLayout &layout);

    void addPushConstantRange(const vk::PushConstantRange &range);

    virtual Pipeline build(const vk::Device &device) const = 0;
};

class ComputePipelineBuilder : public PipelineBuilder {
public:
    ComputePipelineBuilder(const Shader *shader);

    Pipeline build(const vk::Device &device) const;

    Pipeline buildSpecialization(
            const vk::Device &device,
            const std::vector<vk::SpecializationMapEntry> &entries, const void *specData,
            const uint32_t specDataSize) const;
};

class Pipeline {
private:
    const PipelineBuilder *builder;
    vk::PipelineLayout pipelineLayout;
    vk::Pipeline pipelineHandle;
public:
    Pipeline(
            const PipelineBuilder *builder,
            const vk::PipelineLayout pipelineLayout,
            const vk::Pipeline pipelineHandle);

    const vk::Pipeline &handle() const { return pipelineHandle; };

    const vk::PipelineLayout &layout() const { return pipelineLayout; }

    void destroy(const vk::Device &device);

    void deleteBuilder() { delete builder; };
};
