#include "mphf_builder.h"
#include <host_timer.h>
#include "utils.h"
#include "shader_constants.h"
#include <bitset>
#include <immintrin.h>


#define DO_NOT_OPTIMIZE(value) asm volatile("" : : "r,m"(value) : "memory");

MPHFbuilder::MPHFbuilder(App& app, MPHFconfig config):
	config(config),
	app(app),
	bucketSizesStage(BucketSizesStage(app, 32)),
	bucketSortStage(BucketSortStage(app, config.bucketCountPerPartition, config.sortingBins)),
	redistributeKeysStage(RedistributeKeysStage(app, 32)),
	searchStage(SearchStage(app, 32, config)),
	partitionOffsetPPSStage(PrefixSumStage(app, 32, 32)),
	applyPartitionOffsetStage(PartitionOffsetStage(app, 32, config.bucketCountPerPartition))
{}


void MPHFbuilder::BuildInvocation::allocateBuffers() {
	debugBuffer = app.memoryAlloc.createDeviceLocalBuffer(sizeof(uint32_t) * config.bucketCountPerPartition, vk::BufferUsageFlagBits::eTransferSrc);

	fulcrums = app.memoryAlloc.createDeviceLocalBuffer(sizeof(uint32_t) * FULCS_INTER, vk::BufferUsageFlagBits::eTransferDst);

	keysSrc = app.memoryAlloc.createDeviceLocalBuffer(sizeof(uint64_t) * 2 * size, vk::BufferUsageFlagBits::eTransferDst);
	keysLowerDst = app.memoryAlloc.createDeviceLocalBuffer(sizeof(uint64_t) * size, vk::BufferUsageFlagBits::eTransferSrc);

	keyOffsets = app.memoryAlloc.createDeviceLocalBuffer(size + (33 * 4));
	bucketSizeHistogram = app.memoryAlloc.createDeviceLocalBuffer(sizeof(uint32_t) * config.sortingBins * partitions, vk::BufferUsageFlagBits::eTransferSrc);
	bucketSizes = app.memoryAlloc.createDeviceLocalBuffer(sizeof(uint32_t) * totalBucketCount);
	partitionsSizes = app.memoryAlloc.createDeviceLocalBuffer(sizeof(uint32_t) * partitions, vk::BufferUsageFlagBits::eTransferSrc);
	partitionsOffsets = app.memoryAlloc.createDeviceLocalBuffer(sizeof(uint32_t) * partitions, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc);
	bucketPermuatation = app.memoryAlloc.createDeviceLocalBuffer(sizeof(uint32_t) * totalBucketCount);

    pilotsDevice = app.memoryAlloc.createDeviceLocalBuffer(sizeof(uint32_t) * totalBucketCount, vk::BufferUsageFlagBits::eTransferSrc);
	pilotsHost = app.memoryAlloc.createBuffer(vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCached | vk::MemoryPropertyFlagBits::eHostCoherent, sizeof(uint32_t) * totalBucketCount);
}

void MPHFbuilder::BuildInvocation::addGpuCommands() {
	TimestampCreateInfo createInfo;
	TimestampHandle beginTS = createInfo.addTimestamp({ "" });
	TimestampHandle bucketSizesTS = createInfo.addTimestamp({"bucket sizes"});
	TimestampHandle bucketSortingTS = createInfo.addTimestamp({ "bucket sorting" });
	TimestampHandle partitionOffsetsTS = createInfo.addTimestamp({ "partition offsets" });
	TimestampHandle partitionOffsetsApplyTS = createInfo.addTimestamp({"apply partition offsets"});
	TimestampHandle keyRedistributeTS = createInfo.addTimestamp({ "key redistribution" });
	TimestampHandle searchTS = createInfo.addTimestamp({"search"});
	TimestampHandle copyTS = createInfo.addTimestamp({ "copy output" });

	cb->attachTimestamps(app.device, createInfo);

	cb->begin();
	cb->writeTimeStamp(beginTS);

	// find the actual bucket sizes an store the local bucket offset of each key
	builder.bucketSizesStage.addCommands(cb, { size, partitions, config.bucketCountPerPartition }, keysSrc.buffer, keyOffsets.buffer, bucketSizes.buffer, debugBuffer.buffer, fulcrums.buffer);
	cb->writeTimeStamp(bucketSizesTS);
	cb->readWritePipelineBarrier();


	// sort the buckets by descending size and determine bucket offsets
	builder.bucketSortStage.addCommands(cb, { config.partitionMaxSize()}, bucketSizes.buffer, partitions, debugBuffer.buffer, bucketSizeHistogram.buffer, partitionsSizes.buffer, bucketPermuatation.buffer);
	cb->writeTimeStamp(bucketSortingTS);
	cb->readWritePipelineBarrier();

	// partition offset calculations
	cb->copyBuffer(partitionsSizes.buffer, partitionsOffsets.buffer, sizeof(uint32_t) * partitions);
	cb->readWritePipelineBarrier();
	builder.partitionOffsetPPSStage.addCommands(cb, partitions, partitionsOffsets.buffer, ppsData);
	cb->writeTimeStamp(partitionOffsetsTS);
	cb->readWritePipelineBarrier();

	// apply the partition offsets to the bucket offsets
	builder.applyPartitionOffsetStage.addCommands(cb, partitions, bucketSizes.buffer, partitionsOffsets.buffer);
	cb->writeTimeStamp(partitionOffsetsApplyTS);
	cb->readWritePipelineBarrier();

	// redistribute the keys such that the next step can read them in a coalesced manner
	builder.redistributeKeysStage.addCommands(cb, { size, partitions, config.bucketCountPerPartition }, keysSrc.buffer, keysLowerDst.buffer, bucketSizes.buffer, keyOffsets.buffer, fulcrums.buffer);
	cb->writeTimeStamp(keyRedistributeTS);
	cb->readWritePipelineBarrier();

	// perform the actual bijection searching
	builder.searchStage.addCommands(cb, partitions, keysLowerDst.buffer, bucketSizeHistogram.buffer, partitionsSizes.buffer, bucketPermuatation.buffer, pilotsDevice.buffer, partitionsOffsets.buffer, debugBuffer.buffer);
	cb->writeTimeStamp(searchTS);
	cb->readWritePipelineBarrier();

	//cb->copyBuffer(pilotsDevice.buffer, pilotsHost.buffer, sizeof(uint32_t) * totalBucketCount);
	cb->writeTimeStamp(copyTS);
}

bool MPHFbuilder::BuildInvocation::run() {
	HostTimer totalTimer;

	totalBucketCount = partitions * config.bucketCountPerPartition;

	allocateBuffers();

	totalTimer.addLabel("allocation");
	
	fillDeviceWithStagingBufferVec(app.pDevice, app.device, app.transferCommandPool, app.transferQueue, keysSrc, keys);
	fillDeviceWithStagingBufferVec(app.pDevice, app.device, app.transferCommandPool, app.transferQueue, fulcrums, config.getFulcs());

	totalTimer.addLabel("input transfer");

	cb = app.createCommandBuffer();
	addGpuCommands();
	double gpu2cpuOffset = totalTimer.elapsed();
	totalTimer.addLabel("setup commands");
	cb->submit(app.device, app.computeQueue);
	CHECK(app.device.waitIdle(), "failed to wait for until device is idle!");
	// read GPU timestamps
	cb->readTimestamps(app.device, app.pDevice);
	std::vector<TimestampResult> resTS = cb->getTimestamps();

	for (int i = 1; i < resTS.size(); i++) {
		totalTimer.addLabelManual("GPU: " + resTS[i].handle.info.name, resTS[i].time - resTS[0].time + gpu2cpuOffset);
	}

	// get data from GPU to CPU
	double transferStartTime = totalTimer.elapsed();
	std::vector<uint32_t> partitionOffsetArray(partitions);
	fillHostWithStagingBuffer(app.pDevice, app.device, app.transferCommandPool, app.transferQueue, partitionsOffsets, partitionOffsetArray);

	std::vector<uint32_t> outputArray(totalBucketCount);
	fillHostBuffer<uint32_t>(app.device, pilotsHost.memory, outputArray);
	totalTimer.addLabel("transfer");

	//std::cout << "GPU to CPU speed " << (pilotBits1 + pilotBits2) / 8.0 / (totalTimer.elapsed() - transferStartTime) << " GB/s" << std::endl;
	
	//for (size_t i=0; i < partitionOffsetArray.size(); i++) {
	//	int32_t diff = int32_t(partitionOffsetArray[i]) - int32_t((i+1)*config.partitionSize());
	//	partitionOffsetArray[i] = (uint32_t(std::abs(diff)) << 1) | (diff < 0);
	//}
	//f.partitionOffsetsEnc.encode(partitionOffsetArray.begin(), f.partitions);
	//uint32_t partitionBits = f.partitionOffsetsEnc.num_bits();


	//f.bitsPerKey = double(pilotBits2 + pilotBits1 + partitionBits) / size;
	//std::cout << "Total Bits per Key " << f.bitsPerKey << " (" << float(pilotBits1) / size << " + " << float(pilotBits2) / size << " + " << float(partitionBits) / size << ")" << std::endl;
	std::cout << "TIMINGS" << std::endl;
	totalTimer.printLabels(size);
	cb->destroy(app.device, app.computeCommandPool);

	//f.constrTimePerKeyNs = totalTimer.elapsed() / size;

	uint32_t debugCode = getBufferValue<uint32_t>(app.pDevice, app.device, app.transferCommandPool, app.transferQueue, debugBuffer);
	std::cout << "Debug Code " << debugCode << std::endl;
	
	// debug information
	size_t outSize = debugBuffer.capacity / 4;
	//f.debug.resize(outSize);
	//fillHostWithStagingBuffer(app.pDevice, app.device, app.transferCommandPool, app.transferQueue, debugBuffer, f.debug);


	f->setData(outputArray, partitionOffsetArray, partitions, config);


	totalTimer.addLabel("encoding");

	return true;
}

bool MPHFbuilder::build(const std::vector<Key> &keys, MPHFInterface* f) {
	BuildInvocation bd(f, keys, keys.size(), config, app, *this);
	bool succ = bd.run();

	bd.destroy();

	return succ;
}

bool MPHFbuilder::buildRandomized(size_t size, MPHFInterface* f, uint64_t seed) {
	std::vector<Key> keys;
	keys.reserve(size);

	std::random_device rd;
	auto seed2 = rd();
	//seed2 = 2110063238;
	//std::mt19937_64 gen();
	std::cout << "RANDOM SEED " << seed2 << std::endl;
	std::mt19937_64 gen(seed2);
	std::uniform_int_distribution<uint32_t> dis;

	for (size_t i = 0; i < size; ++i) {
		keys.push_back({ dis(gen),dis(gen),dis(gen),dis(gen) });
	}

	return build(keys, f);
}

void MPHFbuilder::BuildInvocation::destroy() {
	partitionsSizes.free(app.memoryAlloc);
	partitionsOffsets.free(app.memoryAlloc);
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

	dictKeys.free(app.memoryAlloc);
	maxPilot.free(app.memoryAlloc);

	pilotLimitPilotCompact.free(app.memoryAlloc);
	pilotLimitIterations.free(app.memoryAlloc);
	pilotLimitMaxIter.free(app.memoryAlloc);
	pilotLimitIterCompact.free(app.memoryAlloc);

	ppsData.destroy(app.memoryAlloc);
}