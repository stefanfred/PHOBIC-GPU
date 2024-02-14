#pragma once

#include "app.h"
#include "bucket_sizes_stage.h"
#include "bucket_sort_stage.h"
#include "prefix_sum_stage.h"
#include "redistribute_keys_stage.h"
#include "apply_partition_offset.h"
#include "mphf_config.h"
#include "search_stage.h"
#include "mphf.hpp"
#include "mphf_build_invocation.h"


class MPHFbuilder {

    template <typename Mphf>
    friend class BuildInvocation;
private:
	MPHFconfig config;
	App& app;

	BucketSizesStage bucketSizesStage;
	BucketSortStage bucketSortStage;
	RedistributeKeysStage redistributeKeysStage;
	SearchStage searchStage;
	PrefixSumStage partitionOffsetPPSStage;
	PartitionOffsetStage applyPartitionOffsetStage;
public:
    MPHFbuilder(App& app, MPHFconfig config):
            config(config),
            app(app),
            bucketSizesStage(BucketSizesStage(app, 32)),
            bucketSortStage(BucketSortStage(app, config.bucketCountPerPartition, config.sortingBins)),
            redistributeKeysStage(RedistributeKeysStage(app, 32)),
            searchStage(SearchStage(app, 32, config)),
            partitionOffsetPPSStage(PrefixSumStage(app, 32, 32)),
            applyPartitionOffsetStage(PartitionOffsetStage(app, 32, config.bucketCountPerPartition))
    {}


    template <typename Mphf>
    void build(const std::vector<Key> &keys, Mphf& f) {
        BuildInvocation<Mphf> bd(f, keys, keys.size(), config, app, this);
        bd.run();
        bd.destroy();
    }
};