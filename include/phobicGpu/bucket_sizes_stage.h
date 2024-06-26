#pragma once

#include "app/app.h"
#include "app/command_buffer.h"

namespace phobicgpu {

    struct PushStructBucketSizes {
        uint32_t size;
        uint32_t partitionCount;
        uint32_t bucketCount;
    };

    class BucketSizesStage {
    private:
        App &app;
        const ShaderStage *bucketSizesStage;

        uint32_t workGroupSize;

    public:
        BucketSizesStage(App &app, uint32_t workGroupSize);

        void addCommands(CommandBuffer *cb, PushStructBucketSizes constants, vk::Buffer upperKeys, vk::Buffer offsets,
                         vk::Buffer counters, vk::Buffer debug, vk::Buffer fulcs);
    };
}