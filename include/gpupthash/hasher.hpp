#pragma once

// See also https://github.com/jermp/bench_hash_functions

namespace gpupthash {

/*
    This code is an adaptation from
    https://github.com/aappleby/smhasher/blob/master/src/MurmurHash2.cpp
        by Austin Appleby
*/
    static uint64_t MurmurHash2_64(void const *key, size_t len, uint64_t seed) {
        const uint64_t m = 0xc6a4a7935bd1e995ULL;
        const int r = 47;

        uint64_t h = seed ^ (len * m);

#if defined(__arm) || defined(__arm__)
        const size_t ksize = sizeof(uint64_t);
        const unsigned char* data = (const unsigned char*)key;
        const unsigned char* end = data + (std::size_t)(len / 8) * ksize;
#else
        const uint64_t *data = (const uint64_t *) key;
        const uint64_t *end = data + (len / 8);
#endif

        while (data != end) {
#if defined(__arm) || defined(__arm__)
            uint64_t k;
            memcpy(&k, data, ksize);
            data += ksize;
#else
            uint64_t k = *data++;
#endif

            k *= m;
            k ^= k >> r;
            k *= m;

            h ^= k;
            h *= m;
        }

        const unsigned char *data2 = (const unsigned char *) data;

        switch (len & 7) {
            // fall through
            case 7:
                h ^= uint64_t(data2[6]) << 48;
                // fall through
            case 6:
                h ^= uint64_t(data2[5]) << 40;
                // fall through
            case 5:
                h ^= uint64_t(data2[4]) << 32;
                // fall through
            case 4:
                h ^= uint64_t(data2[3]) << 24;
                // fall through
            case 3:
                h ^= uint64_t(data2[2]) << 16;
                // fall through
            case 2:
                h ^= uint64_t(data2[1]) << 8;
                // fall through
            case 1:
                h ^= uint64_t(data2[0]);
                h *= m;
        };

        h ^= h >> r;
        h *= m;
        h ^= h >> r;

        return h;
    }

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

        Key(uint64_t v1, uint64_t v2) {
            partitioner = v1 >> 32;
            bucketer = uint32_t(v1);


            lower1 = v2 >> 32;
            lower2 = uint32_t(v2);
        }
    };


    struct nohash {
        static inline Key hash(Key val) {
            return val;
        }
    };

    struct murmurhash2 {

        // specialization for std::string
        static inline Key hash(std::string const &val) {
            return Key(MurmurHash2_64(val.data(), val.size(), 0),
                       MurmurHash2_64(val.data(), val.size(), 1));
        }

        // specialization for uint64_t
        static inline Key hash(uint64_t val) {
            return Key(MurmurHash2_64(reinterpret_cast<char const *>(&val), sizeof(val), 0),
                       MurmurHash2_64(reinterpret_cast<char const *>(&val), sizeof(val), 1));
        }

        // specialization for std::pair<uint64_t, uint64_t>
        static inline Key hash(std::pair<uint64_t, uint64_t> val) {
            return Key(MurmurHash2_64(reinterpret_cast<char const *>(&val.first), sizeof(val.first), 0),
                       MurmurHash2_64(reinterpret_cast<char const *>(&val.second), sizeof(val.second), 0));
        }

        // specialization for Key
        static inline Key hash(Key val) {
            return hash(std::pair<uint64_t, uint64_t>((uint64_t(val.partitioner) << 32U) | uint64_t(val.bucketer),(uint64_t(val.lower1) << 32U) | uint64_t(val.lower2)));
        }
    };

}  // namespace pthash