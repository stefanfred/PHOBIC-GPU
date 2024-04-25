#pragma once

#include "app/app.h"
#include "app/command_buffer.h"

namespace phobicgpu {

    struct PushStructBucketSort {
        uint32_t partitionMaxSize;
    };

    class BucketSortStage {
    private:
        App &app;
        const ShaderStage *bucketSortStage;

        uint32_t workGroupSize;
    public:
        BucketSortStage(App &app, uint32_t bucketCountPerPartition, uint32_t sortingBins,
                        uint32_t workGroupSize);

        void addCommands(CommandBuffer *cb, PushStructBucketSort constants, vk::Buffer bucketSizes, size_t partitions,
                         vk::Buffer debug, vk::Buffer histo, vk::Buffer partitionsSizes,
                         vk::Buffer bucketSortPermutation);
    };

}