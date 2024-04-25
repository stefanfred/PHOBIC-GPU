#pragma once

#include "encoders/base/encoders.hpp"

#include "encoders/pilotEncoders/mono_encoders.hpp"
#include "encoders/pilotEncoders/interleaved_encoder.hpp"
#include "encoders/pilotEncoders/interleaved_encoder_dual.hpp"

#include "encoders/partitionOffsetEnocders/direct_partition_offset_encoder.hpp"
#include "encoders/partitionOffsetEnocders/diff_partition_offset_encoder.hpp"


#include "phobicGpu/mphf.hpp"
#include "phobicGpu/mphf_builder.h"

#include "phobicGpu/hasher.hpp"

namespace phobicgpu {

    typedef MPHF<interleaved_encoder<compact>, diff_partition_encoder<compact>, xxhash> FastQueryMphf;
    typedef MPHF<interleaved_encoder<rice>, diff_partition_encoder<compact>, xxhash> SmallSpaceMphf;

}