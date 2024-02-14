#pragma once

#include "vulkan_api.h"
#include <vector>


struct ShaderModule {
    vk::ShaderModule handle;
    vk::ShaderStageFlagBits stage;

    ShaderModule(vk::ShaderModule shaderModule, vk::ShaderStageFlagBits flag) : handle(shaderModule), stage(flag) {}
};

class Shader {
private:
    const char *shaderName;
    std::vector<ShaderModule> modules;
public:
    Shader(
            const vk::Device &device,
            const char *shaderPath);

    std::vector<vk::PipelineShaderStageCreateInfo> shaderStages() const;

    void destroy(const vk::Device &device);

    static void recompileShader(const char *shaderPath);
};
