#include "search_stage.h"
#include "mphf_config.h"

SearchStage::SearchStage(App& app, uint32_t workGroupSize, MPHFconfig config) : app(app), workGroupSize(workGroupSize) {
    struct sc { uint32_t a; uint32_t b; uint32_t c;  uint32_t d;   uint32_t e;   };
    searchStage = app.computeStage(
        app.loadShader("search"),
        {
            {
                descr::storageBinding(0),
                descr::storageBinding(1),
                descr::storageBinding(2),
                descr::storageBinding(3),
                descr::storageBinding(4),
                descr::storageBinding(5),
                descr::storageBinding(6),
            }
        },
        {},
        {
            {0, sizeof(uint32_t) * 0, sizeof(uint32_t)},
            {1, sizeof(uint32_t) * 1, sizeof(uint32_t)},
            {2, sizeof(uint32_t) * 2, sizeof(uint32_t)},
            {3, sizeof(uint32_t) * 3, sizeof(uint32_t)},
            {4, sizeof(uint32_t) * 4, sizeof(uint32_t)}
        },
        sc{ workGroupSize, config.bucketCountPerPartition, config.sortingBins, config.partitionMaxSize(), config.sortingBins}
    );

}

void SearchStage::addCommands(CommandBuffer* cb, uint32_t partitions, 
    vk::Buffer keys, vk::Buffer bucketSizeHisto, vk::Buffer partitionSizes,
    vk::Buffer bucketPermuatation, vk::Buffer pilots, vk::Buffer partitionsOffsets, vk::Buffer debug) {

    DescriptorSetAllocation desc = app.descrAlloc.alloc(searchStage->descriptorLayouts[0]);
    desc.updateStorageBuffer(0, keys);
    desc.updateStorageBuffer(1, bucketSizeHisto);
    desc.updateStorageBuffer(2, partitionSizes);
    desc.updateStorageBuffer(3, bucketPermuatation);
    desc.updateStorageBuffer(4, partitionsOffsets);
    desc.updateStorageBuffer(5, pilots);
    desc.updateStorageBuffer(6, debug);

    cb->bindComputePipeline(searchStage->pipeline);
    cb->bindComputeDescriptorSet(searchStage->pipeline, desc);
    cb->dispatch(partitions);
}