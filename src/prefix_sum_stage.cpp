#include "phobicGpu/prefix_sum_stage.h"
#include "app/utils.h"

namespace phobicgpu {

    PrefixSumStage::PrefixSumStage(App &app, uint32_t workGroupSizeLocal, uint32_t workGroupSizeOffset) : app(app),
                                                                                                          workGroupSizeLocal(
                                                                                                                  workGroupSizeLocal),
                                                                                                          workGroupSizeOffset(
                                                                                                                  workGroupSizeOffset) {
        localStage = app.computeStage(
                app.loadShader("prefix_sum"),
                {
                        {
                                descr::storageBinding(0),
                                descr::storageBinding(1)
                        }
                },
                PushConstants::ofStruct<PushStructPPS>(),
                {
                        {0, 0, sizeof(uint32_t)}
                },
                workGroupSizeLocal
        );

        offsetStage = app.computeStage(
                app.loadShader("prefix_sum_offset"),
                {
                        {
                                descr::storageBinding(0),
                                descr::storageBinding(1)
                        }
                },
                PushConstants::ofStruct<PushStructPPS>(),
                {
                        {0, 0, sizeof(uint32_t)}
                },
                workGroupSizeOffset
        );
    }

    void PrefixSumStage::addCommands(CommandBuffer *cb, uint32_t totalSize, vk::Buffer work, PrefixSumData &data) {
        // create buffers and descriptors
        vk::Buffer last = work;
        uint32_t size = totalSize;
        while (size > 1) {
            size = (size + workGroupSizeLocal * 2 - 1) / (workGroupSizeLocal * 2);
            BufferAllocation nextBuffer = app.memoryAlloc.createDeviceLocalBuffer(size * sizeof(uint32_t));
            data.buffers.push_back(nextBuffer);
            DescriptorSetAllocation desc = app.descrAlloc.alloc(localStage->descriptorLayouts[0]);
            desc.updateStorageBuffer(0, last);
            desc.updateStorageBuffer(1, nextBuffer.buffer);
            data.descriptors.push_back(desc);
            last = nextBuffer.buffer;
        }

        for (int i = data.buffers.size() - 3; i > -2; i--) {
            DescriptorSetAllocation desc = app.descrAlloc.alloc(offsetStage->descriptorLayouts[0]);
            desc.updateStorageBuffer(1, data.buffers[i + 1].buffer);
            desc.updateStorageBuffer(0, i == -1 ? work : data.buffers[i].buffer);
            data.offsetDescriptors.push_back(desc);
        }


        // add commands
        cb->bindComputePipeline(localStage->pipeline);
        size = totalSize;
        std::vector<uint32_t> sizes;
        for (DescriptorSetAllocation &descr: data.descriptors) {
            cb->readWritePipelineBarrier();
            cb->pushComputePushConstants(localStage->pipeline, PushStructPPS{size});
            sizes.push_back(size);
            size = (size + workGroupSizeLocal * 2 - 1) / (workGroupSizeLocal * 2);
            cb->bindComputeDescriptorSet(localStage->pipeline, descr);
            cb->dispatch(size);

        }

        cb->bindComputePipeline(offsetStage->pipeline);
        for (size_t i = 0; i < data.offsetDescriptors.size(); i++) {
            cb->readWritePipelineBarrier();
            cb->pushComputePushConstants(offsetStage->pipeline, PushStructPPS{sizes[sizes.size() - i - 2]});
            cb->bindComputeDescriptorSet(offsetStage->pipeline, data.offsetDescriptors[i]);
            cb->dispatch(sizes[sizes.size() - i - 1] - 1);
        }
    }

    void PrefixSumData::destroy(const MemoryAllocator &allocator) {
        for (BufferAllocation &ba: buffers) {
            ba.free(allocator);
        }
    }

}