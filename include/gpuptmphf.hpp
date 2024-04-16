#pragma once

#include "encoders/base/encoders.hpp"

#include "encoders/pilotEncoders/mono_encoders.hpp"
#include "encoders/pilotEncoders/interleaved_encoder.hpp"
#include "encoders/pilotEncoders/interleaved_encoder_dual.hpp"

#include "encoders/partitionOffsetEnocders/direct_partition_offset_encoder.hpp"
#include "encoders/partitionOffsetEnocders/diff_partition_offset_encoder.hpp"


#include "gpupthash/mphf.hpp"
#include "gpupthash/mphf_builder.h"

#include "gpupthash/hasher.hpp"

namespace gpupthash {

    typedef MPHF<interleaved_encoder<compact>, diff_partition_encoder<compact>, xxhash> FastMphf;
    typedef MPHF<interleaved_encoder<rice>, diff_partition_encoder<compact>, xxhash> SmallMphf;

}