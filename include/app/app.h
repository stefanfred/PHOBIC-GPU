#pragma once

#include <functional>
#include <vector>
#include <optional>

#include "vulkan_api.h"
#include "shader.h"
#include "encoders/base/command_buffer.h"
#include "memory.h"
#include "pipeline.h"
#include "encoders/base/command_buffer.h"

enum AppEvent {
    eSwapChainRecreation
};

struct AppCallback {
    AppEvent evt;
    std::function<void()> callback;
};


class ShaderStage {
public:
    Pipeline pipeline;
    std::vector<vk::DescriptorSetLayout> descriptorLayouts;
public:
    ShaderStage(
            Pipeline &pipeline,
            const std::vector<vk::DescriptorSetLayout> layouts) : pipeline(pipeline), descriptorLayouts(layouts) {};

    void destroy(const vk::Device &device);
};

struct AppConfiguration {
    bool debugMode = false;
    const char *appName = "";
    std::vector<const char *> requiredExtensions;

    void addRequiredExtensions(const std::vector<const char *> &extensions);
};

class App {
private:
    QueueFamilyIndices indices;
    std::vector<Shader *> loadedShaders;
    std::vector<ShaderStage *> stages;
public:
    vk::Instance instance;

    vk::Device device;
    vk::PhysicalDevice pDevice;
    vk::Queue transferQueue;
    vk::Queue computeQueue;

    vk::CommandPool transferCommandPool;
    vk::CommandPool computeCommandPool;

    MemoryAllocator memoryAlloc;
    DescriptorAllocator descrAlloc;

public:
    App(AppConfiguration &config);

    ~App();

    void debugInfo();

    CommandBuffer *createCommandBuffer();

    const Shader *loadShader(const char *shaderPath);

    const ShaderStage *computeStage(
            const Shader *shader,
            const std::vector<std::vector<vk::DescriptorSetLayoutBinding>> &bindings,
            const std::vector<vk::PushConstantRange> &pushRanges,
            const std::vector<vk::SpecializationMapEntry> specMap,
            const void *specData,
            const uint32_t specDataSize);

    const inline ShaderStage *computeStage(
            const Shader *shader,
            const std::vector<std::vector<vk::DescriptorSetLayoutBinding>> &bindings,
            const std::vector<vk::PushConstantRange> &pushRanges = {}) {

        return computeStage(shader, bindings, pushRanges, {}, nullptr, 0);
    }

    template<typename SPEC_DATA>
    const inline ShaderStage *computeStage(
            const Shader *shader,
            const std::vector<std::vector<vk::DescriptorSetLayoutBinding>> &bindings,
            const std::vector<vk::PushConstantRange> &pushRanges,
            const std::vector<vk::SpecializationMapEntry> specMap,
            const SPEC_DATA data) {

        return computeStage(shader, bindings, pushRanges,
                            specMap, &data, sizeof(SPEC_DATA));
    }
};