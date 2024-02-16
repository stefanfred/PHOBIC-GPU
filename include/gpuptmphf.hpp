#pragma once

#include "encoders/base/encoders.hpp"

#include "encoders/pilotEncoders/mono_encoders.hpp"
#include "encoders/pilotEncoders/ortho_encoder.hpp"
#include "encoders/pilotEncoders/ortho_encoder_dual.hpp"

#include "encoders/partitionOffsetEnocders/direct_partition_offset_encoder.hpp"
#include "encoders/partitionOffsetEnocders/diff_partition_offset_encoder.hpp"


#include "gpupthash/mphf.hpp"
#include "gpupthash/mphf_builder.h"
#include "gpupthash/mphf_builder.h"

#include "gpupthash/hasher.hpp"

namespace gpupthash {

    typedef MPHF<ortho_encoder<compact>, diff_partition_encoder<compact>, nohash> FastMphf;
    typedef MPHF<ortho_encoder<golomb>, diff_partition_encoder<compact>, nohash> SmallMphf;

}