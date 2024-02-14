#include "app/shader.h"
#include "app/ioutil.h"
#include "encoders/base/check.h"

#include <vector>
#include <cstring>

static ShaderModule createShaderModule(const vk::Device &device, const std::string fileName,
                                       vk::ShaderStageFlagBits flagBits) {
    const std::vector<char> &code = readFile(fileName);

    vk::ShaderModuleCreateInfo createInfo{};
    createInfo.sType = vk::StructureType::eShaderModuleCreateInfo;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

    vk::ShaderModule m = CHECK(device.createShaderModule(createInfo), "failed to create shader module");
    return ShaderModule(m, flagBits);
}

Shader::Shader(const vk::Device &device, const char *shaderPath)
        : shaderName(shaderPath) {

    const std::string shaderPathPrefix = "shaders/" + std::string(shaderPath);

    // create shader modules from code
    modules.push_back(createShaderModule(device, shaderPathPrefix + ".comp.spv",
                                         vk::ShaderStageFlagBits::eCompute));
}

std::vector<vk::PipelineShaderStageCreateInfo> Shader::shaderStages() const {
    // create pipeline shader stages for each shader present
    std::vector<vk::PipelineShaderStageCreateInfo> stages;
    stages.reserve(modules.size());
    for (const ShaderModule &m: modules) {
        vk::PipelineShaderStageCreateInfo stageInfo{};
        stageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
        stageInfo.stage = m.stage;
        stageInfo.module = m.handle;
        stageInfo.pName = "main";
        stages.push_back(stageInfo);
    }
    return stages;
}

void Shader::destroy(const vk::Device &device) {
    for (const ShaderModule &m: modules) {
        device.destroyShaderModule(m.handle);
    }
}