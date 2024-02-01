#pragma once
#include <cmath>
#include "shader_constants.h"
#include "spline.h"
#include <numeric>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

#include "bucketers.h"

struct MPHFconfig {
private:
	std::vector<uint32_t> fulcrums;

public:
	uint32_t partitionSize;
	uint32_t bucketCountPerPartition;
	uint32_t sortingBins;


	MPHFconfig(float averageBucketSize = 8.0, uint32_t partitionSize = 1024, const Bucketer &bucketer = Theo1Bucketer()) :
		partitionSize(partitionSize),
		sortingBins(256),
		bucketCountPerPartition(uint32_t(round(partitionSize / averageBucketSize))) {
		setBucketer(bucketer);
	}

	void setBucketer(const Bucketer &bucketer) {
		// initialize fulcrums for bucket assignment

		for (size_t xi = 0; xi < FULCS_INTER; xi++) {
			double x = double(xi) / double(FULCS_INTER - 1);
			double y = bucketer.getBucketRel(x);
			uint32_t fulcV = uint32_t(y * double(bucketCountPerPartition) * double(1 << 16));
			fulcrums.push_back(fulcV);
		}
	}



	const std::vector<uint32_t> &getFulcs() const {
		return fulcrums;
	}

	uint32_t partitionMaxSize() const {
		return partitionSize + partitionSize / 2;
	}
};