#include "apply_partition_offset.h"

PartitionOffsetStage::PartitionOffsetStage(App& app, uint32_t workGroupSize, uint32_t bucketCountPerPartition) : app(app), workGroupSize(workGroupSize) {
    struct sc { uint32_t a; uint32_t b;};
    partitionOffsetStage = app.computeStage(
        app.loadShader("apply_partition_offset"),
        {
            {
                descr::storageBinding(0),
                descr::storageBinding(1),
            }
        },
        {},
        {
            {0, sizeof(uint32_t) * 0, sizeof(uint32_t)},
            {1, sizeof(uint32_t) * 1, sizeof(uint32_t)},
        },
        sc{ workGroupSize, bucketCountPerPartition}
    );
}

void PartitionOffsetStage::addCommands(CommandBuffer* cb, uint32_t partitions, vk::Buffer bucketOffsets, vk::Buffer partitionOffsets) {
    DescriptorSetAllocation desc = app.descrAlloc.alloc(partitionOffsetStage->descriptorLayouts[0]);
    desc.updateStorageBuffer(0, partitionOffsets);
    desc.updateStorageBuffer(1, bucketOffsets);

    cb->bindComputePipeline(partitionOffsetStage->pipeline);
    cb->bindComputeDescriptorSet(partitionOffsetStage->pipeline, desc);
    cb->dispatch(partitions);
}