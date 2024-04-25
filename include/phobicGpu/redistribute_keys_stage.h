#pragma once

#include "app/app.h"
#include "app/command_buffer.h"

namespace phobicgpu {

    struct PushStructRedistributeKeys {
        uint32_t size;
        uint32_t partitionCount;
        uint32_t bucketCount;
    };

    class RedistributeKeysStage {
    private:
        App &app;
        const ShaderStage *redistributeKeysStage;

        uint32_t workGroupSize;

    public:
        RedistributeKeysStage(App &app, uint32_t workGroupSize);

        void addCommands(CommandBuffer *cb, PushStructRedistributeKeys constants,
                         vk::Buffer keySrc, vk::Buffer lowerKeysDst, vk::Buffer bucketOffset, vk::Buffer keyOffset,
                         vk::Buffer fulcs);
    };

}