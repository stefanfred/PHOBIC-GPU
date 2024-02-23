#pragma once

#include "app/app.h"
#include "app/host_timer.h"
#include "app/utils.h"
#include "bucket_sizes_stage.h"
#include "bucket_sort_stage.h"
#include "prefix_sum_stage.h"
#include "redistribute_keys_stage.h"
#include "apply_partition_offset.h"
#include "mphf_config.h"
#include "search_stage.h"
#include "mphf.hpp"

namespace gpupthash {


    class MPHFbuilder {

        template<typename Mphf, typename keyType>
        friend
        class BuildInvocation;

    private:
        MPHFconfig config;
        App &app;

        BucketSizesStage bucketSizesStage;
        BucketSortStage bucketSortStage;
        RedistributeKeysStage redistributeKeysStage;
        SearchStage searchStage;
        PrefixSumStage partitionOffsetPPSStage;
        PartitionOffsetStage applyPartitionOffsetStage;

    public:
        MPHFbuilder(MPHFconfig config = MPHFconfig()) :
                config(config),
                app(App::getInstance()),
                bucketSizesStage(BucketSizesStage(app, app.subGroupSize)),
                bucketSortStage(BucketSortStage(app, config.bucketCountPerPartition, config.sortingBins)),
                redistributeKeysStage(RedistributeKeysStage(app, app.subGroupSize)),
                searchStage(SearchStage(app, app.subGroupSize, config)),
                partitionOffsetPPSStage(PrefixSumStage(app, app.subGroupSize, app.subGroupSize)),
                applyPartitionOffsetStage(
                        PartitionOffsetStage(app, app.subGroupSize, config.bucketCountPerPartition)) {}

        template<typename Mphf, typename keyType>
        void build(const std::vector<keyType> &keys, Mphf &f);
    };

    template<typename Mphf, typename keyType>
    class BuildInvocation {
        Mphf &f;
        const std::vector<keyType> &keysRaw;
        std::vector<Key> keys;
        uint32_t size;
        uint32_t partitions;
        MPHFconfig config;
        App &app;
        MPHFbuilder *builder;

        CommandBuffer *cb;
        uint32_t totalBucketCount;


        BufferAllocation debugBuffer;

        BufferAllocation keysSrc;

        BufferAllocation keysLowerDst;

        BufferAllocation bucketSizes;
        BufferAllocation bucketSizeHistogram;
        BufferAllocation keyOffsets;
        BufferAllocation partitionsSizes;
        BufferAllocation partitionsOffsetsDevice;
        BufferAllocation partitionsOffsetsHost;
        BufferAllocation bucketPermuatation;
        BufferAllocation pilotsDevice;
        BufferAllocation pilotsHost;
        BufferAllocation fulcrums;

        PrefixSumData ppsData;

    public:
        BuildInvocation(Mphf &f, const std::vector<keyType> &keysRaw, uint32_t size, MPHFconfig config, App &app,
                        MPHFbuilder *builder) : f(f), keysRaw(keysRaw), size(size), config(config), app(app),
                                                builder(builder) {
            size = keysRaw.size();
            partitions = (size + config.partitionSize - 1) / config.partitionSize;
            totalBucketCount = partitions * config.bucketCountPerPartition;
        }


        void allocateBuffers() {
            debugBuffer = app.memoryAlloc.createDeviceLocalBuffer(sizeof(uint32_t) * config.bucketCountPerPartition,
                                                                  vk::BufferUsageFlagBits::eTransferSrc);

            fulcrums = app.memoryAlloc.createDeviceLocalBuffer(sizeof(uint32_t) * FULCS_INTER,
                                                               vk::BufferUsageFlagBits::eTransferDst |
                                                               vk::BufferUsageFlagBits::eTransferSrc);

            keysSrc = app.memoryAlloc.createDeviceLocalBuffer(sizeof(uint64_t) * 2 * size,
                                                              vk::BufferUsageFlagBits::eTransferDst |
                                                              vk::BufferUsageFlagBits::eTransferSrc);
            keysLowerDst = app.memoryAlloc.createDeviceLocalBuffer(sizeof(uint64_t) * size,
                                                                   vk::BufferUsageFlagBits::eTransferSrc);

            keyOffsets = app.memoryAlloc.createDeviceLocalBuffer(size + (33 * 4));
            bucketSizeHistogram = app.memoryAlloc.createDeviceLocalBuffer(
                    sizeof(uint32_t) * config.sortingBins * partitions, vk::BufferUsageFlagBits::eTransferSrc);
            bucketSizes = app.memoryAlloc.createDeviceLocalBuffer(sizeof(uint32_t) * totalBucketCount,
                                                                  vk::BufferUsageFlagBits::eTransferSrc);
            partitionsSizes = app.memoryAlloc.createDeviceLocalBuffer(sizeof(uint32_t) * partitions,
                                                                      vk::BufferUsageFlagBits::eTransferSrc);
            bucketPermuatation = app.memoryAlloc.createDeviceLocalBuffer(sizeof(uint32_t) * totalBucketCount);


            partitionsOffsetsDevice = app.memoryAlloc.createDeviceLocalBuffer(sizeof(uint32_t) * partitions,
                                                                              vk::BufferUsageFlagBits::eTransferDst |
                                                                              vk::BufferUsageFlagBits::eTransferSrc);
            partitionsOffsetsHost = app.memoryAlloc.createBuffer(vk::BufferUsageFlagBits::eTransferDst,
                                                                 vk::MemoryPropertyFlagBits::eHostVisible |
                                                                 vk::MemoryPropertyFlagBits::eHostCached |
                                                                 vk::MemoryPropertyFlagBits::eHostCoherent,
                                                                 sizeof(uint32_t) * partitions);

            pilotsDevice = app.memoryAlloc.createDeviceLocalBuffer(sizeof(uint32_t) * totalBucketCount,
                                                                   vk::BufferUsageFlagBits::eTransferSrc);
            pilotsHost = app.memoryAlloc.createBuffer(vk::BufferUsageFlagBits::eTransferDst,
                                                      vk::MemoryPropertyFlagBits::eHostVisible |
                                                      vk::MemoryPropertyFlagBits::eHostCached |
                                                      vk::MemoryPropertyFlagBits::eHostCoherent,
                                                      sizeof(uint32_t) * totalBucketCount);
        }

        void addGpuCommands() {
            TimestampCreateInfo createInfo;
            TimestampHandle beginTS = createInfo.addTimestamp({""});
            TimestampHandle bucketSizesTS = createInfo.addTimestamp({"bucket sizes"});
            TimestampHandle bucketSortingTS = createInfo.addTimestamp({"bucket sorting"});
            TimestampHandle partitionOffsetsTS = createInfo.addTimestamp({"partition offsets"});
            TimestampHandle partitionOffsetsApplyTS = createInfo.addTimestamp({"apply partition offsets"});
            TimestampHandle keyRedistributeTS = createInfo.addTimestamp({"key redistribution"});
            TimestampHandle searchTS = createInfo.addTimestamp({"search"});
            TimestampHandle copyTS = createInfo.addTimestamp({"map output"});

            cb->attachTimestamps(app.device, createInfo);

            cb->begin();
            cb->writeTimeStamp(beginTS);

            // find the actual bucket sizes an store the local bucket offset of each key
            builder->bucketSizesStage.addCommands(cb, {size, partitions, config.bucketCountPerPartition},
                                                  keysSrc.buffer,
                                                  keyOffsets.buffer, bucketSizes.buffer, debugBuffer.buffer,
                                                  fulcrums.buffer);
            cb->writeTimeStamp(bucketSizesTS);
            cb->readWritePipelineBarrier();


            // sort the buckets by descending size and determine bucket offsets
            builder->bucketSortStage.addCommands(cb, {config.partitionMaxSize()}, bucketSizes.buffer, partitions,
                                                 debugBuffer.buffer, bucketSizeHistogram.buffer, partitionsSizes.buffer,
                                                 bucketPermuatation.buffer);
            cb->writeTimeStamp(bucketSortingTS);
            cb->readWritePipelineBarrier();

            // partition offset calculations
            cb->copyBuffer(partitionsSizes.buffer, partitionsOffsetsDevice.buffer, sizeof(uint32_t) * partitions);
            cb->readWritePipelineBarrier();
            builder->partitionOffsetPPSStage.addCommands(cb, partitions, partitionsOffsetsDevice.buffer, ppsData);
            cb->writeTimeStamp(partitionOffsetsTS);
            cb->readWritePipelineBarrier();

            // apply the partition offsets to the bucket offsets
            builder->applyPartitionOffsetStage.addCommands(cb, partitions, bucketSizes.buffer,
                                                           partitionsOffsetsDevice.buffer);
            cb->writeTimeStamp(partitionOffsetsApplyTS);
            cb->readWritePipelineBarrier();

            // redistribute the keys such that the next step can read them in a coalesced manner
            builder->redistributeKeysStage.addCommands(cb, {size, partitions, config.bucketCountPerPartition},
                                                       keysSrc.buffer, keysLowerDst.buffer, bucketSizes.buffer,
                                                       keyOffsets.buffer, fulcrums.buffer);
            cb->writeTimeStamp(keyRedistributeTS);
            cb->readWritePipelineBarrier();

            // perform the actual bijection searching
            builder->searchStage.addCommands(cb, partitions, keysLowerDst.buffer, bucketSizeHistogram.buffer,
                                             partitionsSizes.buffer, bucketPermuatation.buffer, pilotsDevice.buffer,
                                             partitionsOffsetsDevice.buffer, debugBuffer.buffer);
            cb->writeTimeStamp(searchTS);
            cb->readWritePipelineBarrier();

            cb->copyBuffer(pilotsDevice.buffer, pilotsHost.buffer, sizeof(uint32_t) * totalBucketCount);
            cb->copyBuffer(partitionsOffsetsDevice.buffer, partitionsOffsetsHost.buffer, sizeof(uint32_t) * partitions);
            cb->writeTimeStamp(copyTS);
        }

        const std::vector<Key> &initialHash() {
            if constexpr (std::is_same_v<keyType, Key>) {
                return keysRaw;
            } else {
                keys.resize(keysRaw.size());
#pragma omp parallel for
                for (int i = 0; i < keysRaw.size(); ++i) {
                    keys[i] = Mphf::initialHash(keysRaw[i]);
                }
                return const_cast<const std::vector<Key> &>(keys);
            }
        }

        std::filesystem::path getCsvPath(std::string name) {
            std::filesystem::path out = std::filesystem::current_path() / ".." / "results";
            std::filesystem::create_directory(out);
            out /= name + ".csv";
            return out;
        }

        bool run() {
            HostTimer totalTimer;

            const std::vector<Key> &keyUpload = initialHash();
            totalTimer.addLabel("initial hash");
            allocateBuffers();
            totalTimer.addLabel("allocation");

            fillDeviceWithStagingBufferVec(app.pDevice, app.device, app.transferCommandPool, app.transferQueue, keysSrc,
                                           keyUpload);
            fillDeviceWithStagingBufferVec(app.pDevice, app.device, app.transferCommandPool, app.transferQueue,
                                           fulcrums,
                                           config.getFulcs());
            totalTimer.addLabel("input transfer");

            cb = app.createCommandBuffer();
            addGpuCommands();
            double gpu2cpuOffset = totalTimer.elapsed();
            totalTimer.addLabel("setup commands");
            cb->submit(app.device, app.computeQueue, true);
            cb->readTimestamps(app.device, app.pDevice);
            std::vector<TimestampResult> resTS = cb->getTimestamps();

            for (int i = 1; i < resTS.size(); i++) {
                totalTimer.addLabelManual("GPU: " + resTS[i].handle.info.name,
                                          resTS[i].time - resTS[0].time + gpu2cpuOffset);
            }


            std::vector<uint32_t> partitionOffsetArray(partitions + 1);
            partitionOffsetArray[0] = 0;
            fillHostBuffer<uint32_t>(app.device, partitionsOffsetsHost.memory, partitionOffsetArray.data() + 1,
                                     partitions);

            std::vector<uint32_t> outputArray(totalBucketCount);
            fillHostBuffer<uint32_t>(app.device, pilotsHost.memory, outputArray);
            totalTimer.addLabel("result transfer");

            std::cout << "TIMINGS" << std::endl;
            totalTimer.printLabels(size);
            cb->destroy(app.device, app.computeCommandPool);

            // debug information
            BufferAllocation print = debugBuffer;
            size_t outSize = print.capacity / 4;
            std::vector<uint32_t> debug;
            debug.resize(outSize);
            fillHostWithStagingBuffer(app.pDevice, app.device, app.transferCommandPool, app.transferQueue, print,
                                      debug);

            for(auto v : debug) {
                std::cout<<v<<" ";
            }
            std::cout<<std::endl;

            std::ofstream csv = getCsvPath("probabilities");
            for (uint32_t j = 0; j < partitions; j++) {
                for (uint32_t i = 0; i < config.bucketCountPerPartition; i++) {
                    csv << std::to_string(debug[i + j * config.bucketCountPerPartition]) << ",";
                }
                csv << "\n";
            }
            csv.close();

            f.setData(outputArray, partitionOffsetArray, partitions, config);
            totalTimer.addLabel("encoding");
            return true;
        }


        void destroy() {
            partitionsSizes.free(app.memoryAlloc);
            partitionsOffsetsDevice.free(app.memoryAlloc);
            partitionsOffsetsHost.free(app.memoryAlloc);
            bucketSizeHistogram.free(app.memoryAlloc);
            debugBuffer.free(app.memoryAlloc);
            keysSrc.free(app.memoryAlloc);
            keysLowerDst.free(app.memoryAlloc);
            keyOffsets.free(app.memoryAlloc);
            bucketSizes.free(app.memoryAlloc);
            bucketPermuatation.free(app.memoryAlloc);
            pilotsDevice.free(app.memoryAlloc);
            pilotsHost.free(app.memoryAlloc);
            fulcrums.free(app.memoryAlloc);

            ppsData.destroy(app.memoryAlloc);
        }
    };


    template<typename Mphf, typename keyType>
    void MPHFbuilder::build(const std::vector<keyType> &keys, Mphf &f) {
        BuildInvocation<Mphf, keyType> bd(f, keys, keys.size(), config, app, this);
        bd.run();
        bd.destroy();
    }
}