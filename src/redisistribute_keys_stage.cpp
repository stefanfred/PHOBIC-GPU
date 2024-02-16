#include "gpupthash/redistribute_keys_stage.h"

namespace gpupthash {

    RedistributeKeysStage::RedistributeKeysStage(App &app, uint32_t workGroupSize) : app(app),
                                                                                     workGroupSize(workGroupSize) {
        redistributeKeysStage = app.computeStage(
                app.loadShader("redistribute_keys"),
                {
                        {
                                descr::storageBinding(0),
                                descr::storageBinding(1),
                                descr::storageBinding(2),
                                descr::storageBinding(3),
                        },
                        {
                                descr::storageBinding(0)
                        }
                },
                PushConstants::ofStruct<PushStructRedistributeKeys>(),
                {
                        {0, 0, sizeof(uint32_t)}
                },
                workGroupSize
        );
    }

    void RedistributeKeysStage::addCommands(CommandBuffer *cb, PushStructRedistributeKeys constants,
                                            vk::Buffer keySrc, vk::Buffer lowerKeysDst, vk::Buffer bucketOffset,
                                            vk::Buffer keyOffset, vk::Buffer fulcs) {
        DescriptorSetAllocation desc0 = app.descrAlloc.alloc(redistributeKeysStage->descriptorLayouts[0]);
        desc0.updateStorageBuffer(0, keySrc);
        desc0.updateStorageBuffer(1, lowerKeysDst);
        desc0.updateStorageBuffer(2, bucketOffset);
        desc0.updateStorageBuffer(3, keyOffset);

        DescriptorSetAllocation desc1 = app.descrAlloc.alloc(redistributeKeysStage->descriptorLayouts[1]);
        desc1.updateStorageBuffer(0, fulcs);

        cb->bindComputePipeline(redistributeKeysStage->pipeline);
        cb->pushComputePushConstants(redistributeKeysStage->pipeline, constants);
        cb->bindComputeDescriptorSet(redistributeKeysStage->pipeline, desc0, 0);
        cb->bindComputeDescriptorSet(redistributeKeysStage->pipeline, desc1, 1);
        cb->dispatch((constants.size + (workGroupSize * 4) - 1) / (workGroupSize * 4));
    }

}