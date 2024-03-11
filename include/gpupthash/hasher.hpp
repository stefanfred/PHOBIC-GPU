#pragma once

#include "xxHash/xxh3.h"

namespace gpupthash {

    struct Key {
        uint32_t partitioner;
        uint32_t bucketer;
        uint32_t lower1;
        uint32_t lower2;

        Key() {}

        Key(uint32_t partitioner, uint32_t bucketer, uint32_t lower1, uint32_t lower2) : partitioner(partitioner),
                                                                                         bucketer(bucketer),
                                                                                         lower1(lower1),
                                                                                         lower2(lower2) {}

    };


    struct nohash {
        static inline const Key &hash(const Key &val) {
            return val;
        }
    };


    struct xxhash {

        // specialization for std::string
        static inline Key hash(std::string const &val) {
            XXH128_hash_t ha=XXH128(val.data(), val.size(), 0);
            Key out;
            ((uint64_t*)&out)[0] = ha.low64;
            ((uint64_t*)&out)[1] = ha.high64;
            return out;
        }

        // specialization for uint64_t
        static inline Key hash(uint64_t val) {
            XXH128_hash_t ha=XXH128(&val, sizeof(val), 0);
            Key out;
            ((uint64_t*)&out)[0] = ha.low64;
            ((uint64_t*)&out)[1] = ha.high64;
            return out;
        }

        // specialization for std::pair<uint64_t, uint64_t>
        static inline Key hash(std::pair<uint64_t, uint64_t> val) {
            XXH128_hash_t ha=XXH128(&val, sizeof(val), 0);
            Key out;
            ((uint64_t*)&out)[0] = ha.low64;
            ((uint64_t*)&out)[1] = ha.high64;
            return out;
        }

        // specialization for Key
        static inline Key hash(const Key &val) {
            XXH128_hash_t ha=XXH128(&val, sizeof(val), 0);
            Key out;
            ((uint64_t*)&out)[0] = ha.low64;
            ((uint64_t*)&out)[1] = ha.high64;
            return out;
        }
    };

}  // namespace pthash