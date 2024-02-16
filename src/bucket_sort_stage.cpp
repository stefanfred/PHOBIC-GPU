#include "gpupthash/bucket_sort_stage.h"

namespace gpupthash {

    BucketSortStage::BucketSortStage(App &app, uint32_t bucketCountPerPartition, uint32_t sortingBins) : app(app) {
        struct sc {
            uint32_t a;
            uint32_t b;
        };
        bucketSortStage = app.computeStage(
                app.loadShader("sort_sum"),
                {
                        {
                                descr::storageBinding(0),
                                descr::storageBinding(1),
                                descr::storageBinding(2),
                                descr::storageBinding(3),
                                descr::storageBinding(4),
                        }
                },
                PushConstants::ofStruct<PushStructBucketSort>(),
                {
                        {0, sizeof(uint32_t) * 0, sizeof(uint32_t)},
                        {1, sizeof(uint32_t) * 1, sizeof(uint32_t)},
                },
                sc{sortingBins / 2, bucketCountPerPartition}
        );
    }

    void BucketSortStage::addCommands(CommandBuffer *cb, PushStructBucketSort constants, vk::Buffer bucketSizes,
                                      size_t partitions, vk::Buffer debug, vk::Buffer histo, vk::Buffer partitionsSizes,
                                      vk::Buffer bucketSortPermutation) {
        DescriptorSetAllocation desc = app.descrAlloc.alloc(bucketSortStage->descriptorLayouts[0]);
        desc.updateStorageBuffer(0, bucketSizes);
        desc.updateStorageBuffer(1, debug);
        desc.updateStorageBuffer(2, histo);
        desc.updateStorageBuffer(3, partitionsSizes);
        desc.updateStorageBuffer(4, bucketSortPermutation);

        cb->bindComputePipeline(bucketSortStage->pipeline);
        cb->pushComputePushConstants(bucketSortStage->pipeline, constants);
        cb->bindComputeDescriptorSet(bucketSortStage->pipeline, desc);
        cb->dispatch(partitions);
    }

}