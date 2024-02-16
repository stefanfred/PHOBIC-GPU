#pragma once

#include "app/app.h"
#include "bucket_sizes_stage.h"
#include "bucket_sort_stage.h"
#include "prefix_sum_stage.h"
#include "redistribute_keys_stage.h"
#include "apply_partition_offset.h"
#include "mphf_config.h"
#include "search_stage.h"
#include "mphf.hpp"
#include "mphf_build_invocation.h"

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
                applyPartitionOffsetStage(PartitionOffsetStage(app, app.subGroupSize, config.bucketCountPerPartition)) {}


        template<typename Mphf, typename keyType>
        void build(const std::vector<keyType> &keys, Mphf &f) {
            BuildInvocation<Mphf, keyType> bd(f, keys, keys.size(), config, app, this);
            bd.run();
            bd.destroy();
        }
    };

}