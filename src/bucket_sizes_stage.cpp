#include "bucket_sizes_stage.h"

BucketSizesStage::BucketSizesStage(App& app, uint32_t workGroupSize) : app(app), workGroupSize(workGroupSize) {
    bucketSizesStage = app.computeStage(
        app.loadShader("bucket_sizes"),
        {
            {
                descr::storageBinding(0),
                descr::storageBinding(1),
                descr::storageBinding(2),
                descr::storageBinding(3),//debug
            },
            {
                descr::storageBinding(0)
            }
        },
        PushConstants::ofStruct<PushStructBucketSizes>(),
        {
            {0, 0, sizeof(uint32_t)}
        },
        workGroupSize
    );
}

void BucketSizesStage::addCommands(CommandBuffer* cb, PushStructBucketSizes constants, vk::Buffer keys, vk::Buffer offsets, vk::Buffer counters, vk::Buffer debug, vk::Buffer fulcs) {
    DescriptorSetAllocation desc0 = app.descrAlloc.alloc(bucketSizesStage->descriptorLayouts[0]);
    desc0.updateStorageBuffer(0, keys);
    desc0.updateStorageBuffer(1, offsets);
    desc0.updateStorageBuffer(2, counters);
    desc0.updateStorageBuffer(3, debug);

    DescriptorSetAllocation desc1 = app.descrAlloc.alloc(bucketSizesStage->descriptorLayouts[1]);
    desc1.updateStorageBuffer(0, fulcs);

    cb->bindComputePipeline(bucketSizesStage->pipeline);
    cb->pushComputePushConstants(bucketSizesStage->pipeline, constants);
    cb->bindComputeDescriptorSet(bucketSizesStage->pipeline, desc0,0);
    cb->bindComputeDescriptorSet(bucketSizesStage->pipeline, desc1,1);
    cb->dispatch((constants.size + (workGroupSize * 4) - 1) / (workGroupSize * 4));
}