#include <algorithm>
#include <iostream>
#include <set>

#include "app/vulkan_api.h"
#include "app/app.h"
#include "app/check.h"
#include "app/shader.h"


void AppConfiguration::addRequiredExtensions(const std::vector<const char *> &extensions) {
    this->requiredExtensions.insert(this->requiredExtensions.end(), extensions.begin(), extensions.end());
}

static const std::vector<const char *> DEBUG_MODE_VALIDATION_LAYERS = {
        "VK_LAYER_KHRONOS_validation"
};

static const std::vector<const char *> REQUIRED_DEVICE_EXTENSIONS = {
        VK_KHR_MAINTENANCE_4_EXTENSION_NAME
        //"VK_EXT_pipeline_creation_feedback"
};

static bool checkValidationLayerSupport(const std::vector<const char *> &validationLayers) {
    // first we count the number of available validation layers
    uint32_t layerCount;
    CHECK(vk::enumerateInstanceLayerProperties(&layerCount, nullptr),
          "unable to count instance layer properties!");

    // then we allocate enough data and actually list them
    std::vector<vk::LayerProperties> availableLayers(layerCount);
    CHECK(vk::enumerateInstanceLayerProperties(&layerCount, availableLayers.data()),
          "unable to fetch instance layer properties!");

    // at the end we check whether all requested validation layers are actually available
    for (const char *layerName: validationLayers) {
        if (std::find_if(availableLayers.begin(), availableLayers.end(),
                         [layerName](vk::LayerProperties layerProperties) {
                             return strcmp(layerProperties.layerName, layerName) == 0;
                         }) == availableLayers.end()) {
            // layer not found
            return false;
        }
    }

    return true;
}

static bool
checkDeviceExtensionSupport(const vk::PhysicalDevice &pDevice, const std::vector<const char *> &deviceExtensions) {
    uint32_t extensionCount;
    CHECK(pDevice.enumerateDeviceExtensionProperties(nullptr, &extensionCount, nullptr),
          "unable to count device extension properties!");

    std::vector<vk::ExtensionProperties> availableExtensions(extensionCount);
    CHECK(pDevice.enumerateDeviceExtensionProperties(nullptr, &extensionCount, availableExtensions.data()),
          "unable to fetch device extension properties!");

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto &extension: availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

static vk::Instance createInstance(const AppConfiguration &config) {
    // specify information about our application and its requirements
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.apiVersion = VK_API_VERSION_1_3;

    // specify global extensions and validation layers
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // the required extensions are for example given by GLFW
    // beginning with the 1.3.216 Vulkan SDK, the VK_KHR_PORTABILITY_subset extension is mandatory,
    // so we also add it
    std::vector<const char *> extensions(config.requiredExtensions);
    //extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    //extensions.push_back(VK_KHR_MAINTENANCE_4_EXTENSION_NAME);

    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    vk::PhysicalDeviceMaintenance4FeaturesKHR maintenance4Features{};
    maintenance4Features.sType = vk::StructureType::ePhysicalDeviceMaintenance4FeaturesKHR;
    maintenance4Features.maintenance4 = true;
    maintenance4Features.pNext = nullptr;


    // optionally enable validation layers
    if (config.debugMode) {
        // ensure that there is support for all requested validation layers
        CHECK(checkValidationLayerSupport(DEBUG_MODE_VALIDATION_LAYERS),
              "no validation layer support");

        createInfo.enabledLayerCount = static_cast<uint32_t>(DEBUG_MODE_VALIDATION_LAYERS.size());
        createInfo.ppEnabledLayerNames = DEBUG_MODE_VALIDATION_LAYERS.data();

        // at this point a custom debug messenger could be specified
        // TODO
    } else {
        createInfo.enabledLayerCount = 0;
    }

    // now the actual instance is created for the given requirements
    return CHECK(vk::createInstance(createInfo), "failed to create vulkan instance");
}

const QueueFamilyIndices findQueueFamilies(const vk::PhysicalDevice &pDevice) {
    QueueFamilyIndices indices;
    // Logic to find queue family indices to populate struct with

    uint32_t queueFamilyCount = 0;
    pDevice.getQueueFamilyProperties(&queueFamilyCount, nullptr);

    std::vector<vk::QueueFamilyProperties> queueFamilies(queueFamilyCount);
    pDevice.getQueueFamilyProperties(&queueFamilyCount, queueFamilies.data());

    uint32_t index = 0;

    for (const vk::QueueFamilyProperties &queueFamily: queueFamilies) {
        // detect compute family
        if (queueFamily.queueFlags & vk::QueueFlagBits::eCompute) {
            indices.computeFamily = index;
        }

        // detect transfer family
        if (queueFamily.queueFlags & vk::QueueFlagBits::eTransfer) {
            indices.transferFamily = index;
        }
        index++;
    }

    return indices;
}

static const uint32_t DEVICE_NOT_SUITABLE = 0;

// returns 0 if a device is not suitable at all
// higher numbers are returned for more capable devices
static uint32_t scoreDeviceSuitability(const vk::PhysicalDevice &pDevice) {
    // fetch features and properties of device
    vk::PhysicalDeviceProperties deviceProperties;
    pDevice.getProperties(&deviceProperties);
    vk::PhysicalDeviceFeatures deviceFeatures;
    pDevice.getFeatures(&deviceFeatures);

    // we need the specified device extensions
    if (!checkDeviceExtensionSupport(pDevice, REQUIRED_DEVICE_EXTENSIONS)) {
        return DEVICE_NOT_SUITABLE;
    }

    // device at least fulfills requirements so the score is at least one
    uint32_t deviceScore = 1;

    // increase score if device is even more suitable
    if (deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
        deviceScore++;
    }
    return deviceScore;
}

static vk::PhysicalDevice pickPhysicalDevice(const vk::Instance &instance) {
    // choose a graphics card to use suitable for our rendering purpose
    uint32_t deviceCount = 0;
    CHECK(instance.enumeratePhysicalDevices(&deviceCount, nullptr),
          "failed to enumerate physical devices!");

    // we need at least one GPU to proceed
    CHECK(deviceCount != DEVICE_NOT_SUITABLE, "no GPU with vulkan support!");

    // fetch all GPUs
    std::vector<vk::PhysicalDevice> devices(deviceCount);
    CHECK(instance.enumeratePhysicalDevices(&deviceCount, devices.data()),
          "failed to list physical devices!");

    // find suitable GPU
    vk::PhysicalDevice pDevice;
    uint32_t pDeviceScore = 0; // score of 0 <=> device not suitable

    for (auto const &p: devices) {
        uint32_t score = scoreDeviceSuitability(p);
        if (score > pDeviceScore) {
            pDevice = p;
            pDeviceScore = score;
        }
    }

    CHECK(pDeviceScore > 0, "no suitable GPU found!");
    return pDevice;
}

static uint32_t getSubgroupSize(vk::PhysicalDevice pDevice) {
    vk::PhysicalDeviceSubgroupProperties subgroupProperties;
    vk::PhysicalDeviceProperties2 deviceProperties2;
    deviceProperties2.pNext = &subgroupProperties;
    pDevice.getProperties2(&deviceProperties2);
    return subgroupProperties.subgroupSize;
}


static vk::Device createLogicalDevice(
        const AppConfiguration &config, const QueueFamilyIndices &indices,
        const vk::Instance &instance, const vk::PhysicalDevice &pDevice) {

    // TODO: we need to assign individual queue priorities in the future
    const float queuePriority = 1.0f;

    const std::set<uint32_t> queueIndicesSet = {
            indices.transferFamily.value(),
            indices.computeFamily.value()
    };

    // the actual number of created queues is between 1 and 3
    std::vector<vk::DeviceQueueCreateInfo> queues;
    for (const uint32_t queueFamilyIndex: queueIndicesSet) {
        vk::DeviceQueueCreateInfo graphicsQueueCreateInfo{};
        graphicsQueueCreateInfo.sType = vk::StructureType::eDeviceQueueCreateInfo;
        graphicsQueueCreateInfo.queueFamilyIndex = queueFamilyIndex;
        graphicsQueueCreateInfo.queueCount = 1;
        graphicsQueueCreateInfo.pQueuePriorities = &queuePriority;
        queues.push_back(graphicsQueueCreateInfo);
    }

    vk::PhysicalDeviceVulkan11Features vulkan11Features{};
    vulkan11Features.shaderDrawParameters = true;
    vulkan11Features.pNext = nullptr;


    vk::PhysicalDeviceFeatures deviceFeatures{};


    vk::DeviceCreateInfo createInfo{};
    createInfo.sType = vk::StructureType::eDeviceCreateInfo;
    createInfo.pQueueCreateInfos = queues.data();
    createInfo.queueCreateInfoCount = queues.size();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.pNext = &vulkan11Features;

    // enable extensions
    createInfo.enabledExtensionCount = static_cast<uint32_t>(REQUIRED_DEVICE_EXTENSIONS.size());
    createInfo.ppEnabledExtensionNames = REQUIRED_DEVICE_EXTENSIONS.data();

    // optionally enable validation layers in debug mode
    // it is usually enough to do it for the physical device but we do it anyways
    if (config.debugMode) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(DEBUG_MODE_VALIDATION_LAYERS.size());
        createInfo.ppEnabledLayerNames = DEBUG_MODE_VALIDATION_LAYERS.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    // not we can create the actual device
    return CHECK(pDevice.createDevice(createInfo), "failed to create logical vulkan device");
}

static vk::CommandPool createCommandPool(const vk::Device &device, uint32_t index) {
    vk::CommandPoolCreateInfo poolInfo{};
    poolInfo.sType = vk::StructureType::eCommandPoolCreateInfo;
    // allow command buffers to be rerecorded individually
    poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    poolInfo.queueFamilyIndex = index;
    return CHECK(device.createCommandPool(poolInfo), "failed to create command pool");
}

App::App() {
    AppConfiguration config;

    instance = createInstance(config);
    pDevice = pickPhysicalDevice(instance);
    subGroupSize = getSubgroupSize(pDevice);

    indices = findQueueFamilies(pDevice);
    device = createLogicalDevice(config, indices, instance, pDevice);
    transferQueue = device.getQueue(indices.transferFamily.value(), 0);
    computeQueue = device.getQueue(indices.computeFamily.value(), 0);

    transferCommandPool = createCommandPool(device, indices.transferFamily.value());
    computeCommandPool = createCommandPool(device, indices.computeFamily.value());

    memoryAlloc = MemoryAllocator(pDevice, device, indices,
                                  transferQueue, transferCommandPool);

    descrAlloc = DescriptorAllocator(device);
}

App::~App() {
    for (ShaderStage *stage: stages) {
        stage->pipeline.deleteBuilder();
        stage->destroy(device);
        delete stage;
    }
    for (Shader *shader: loadedShaders) {
        shader->destroy(device);
        delete shader;
    }
    descrAlloc.destroy();
    device.destroyCommandPool(transferCommandPool);
    device.destroyCommandPool(computeCommandPool);
    device.destroy();
    instance.destroy();
}

std::string App::getInfoResultStyle() {
    vk::PhysicalDeviceProperties deviceProperties;
    pDevice.getProperties(&deviceProperties);
    std::string res;
    res += "device_name=";
    std::string deviceName = std::string(deviceProperties.deviceName);
    std::replace(deviceName.begin(), deviceName.end(), ' ', '_');
    res += deviceName;
    res += " subgroupsize=";
    res += std::to_string(subGroupSize);
    res += " computeQueues=";
    res += std::to_string(indices.computeFamily.has_value());
    res += " transferQueues=";
    res += std::to_string(indices.transferFamily.has_value());
    return res;
}

void App::printDebugInfo() {
    // fetch features and properties of device
    vk::PhysicalDeviceProperties deviceProperties;
    pDevice.getProperties(&deviceProperties);

    std::cout << "Device: " << deviceProperties.deviceName << std::endl;
    std::cout << "SubGroupSize: " << subGroupSize << std::endl;
    std::cout << "Queues: C = " << indices.computeFamily.has_value()
              << ", T = " << indices.transferFamily.has_value() << std::endl;
}


const Shader *App::loadShader(const char *shaderName) {
    loadedShaders.push_back(new Shader(device, shaderName));
    return loadedShaders[loadedShaders.size() - 1];
}

CommandBuffer *App::createCommandBuffer() {
    return new CommandBuffer(device, computeCommandPool);
}


const ShaderStage *App::computeStage(
        const Shader *shader,
        const std::vector<std::vector<vk::DescriptorSetLayoutBinding>> &bindings,
        const std::vector<vk::PushConstantRange> &pushRanges,
        const std::vector<vk::SpecializationMapEntry> specMap,
        const void *specData,
        const uint32_t specDataSize) {

    const std::vector<vk::DescriptorSetLayout> layouts = DescriptorAllocator::createLayouts(device, bindings);

    ComputePipelineBuilder builder(shader);
    for (const vk::DescriptorSetLayout &layout: layouts) builder.addDescriptorSetLayout(layout);
    for (const vk::PushConstantRange &r: pushRanges) builder.addPushConstantRange(r);
    Pipeline pipeline = builder.buildSpecialization(device, specMap, specData, specDataSize);

    stages.push_back(new ShaderStage(pipeline, layouts));
    return stages[stages.size() - 1];
}

void ShaderStage::destroy(const vk::Device &device) {
    pipeline.destroy(device);
    for (const vk::DescriptorSetLayout &layout: descriptorLayouts) {
        device.destroyDescriptorSetLayout(layout);
    }
}