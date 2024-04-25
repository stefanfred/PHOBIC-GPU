#include <chrono>
#include <tlx/cmdline_parser.hpp>

#include <gpuptmphf.hpp>
#include <omp.h>
#include <vector>
#include <iostream>
#include <cstdint>

using namespace phobicgpu;

#define DO_NOT_OPTIMIZE(value) asm volatile("" : : "r,m"(value) : "memory")

size_t size = 1e8;
size_t queries = 1e8;
double lambda = 7.5;
double tradeoff = 0.5;
size_t partitionSize = 2048;
std::string pilotencoderstrat = "dualinter";
std::string pilotencoderbase = "c";
std::string partitionencoderstrat = "diff";
std::string partitionencoderbase = "c";
std::string hashfunctionstring = "xx";
std::string keytypestring = "string";
bool validate = false;

std::random_device rd;
std::mt19937_64 gen(rd());
std::uniform_int_distribution<uint32_t> dis;

template<typename pilotencoder, typename offsetencoder, typename hashfunction, typename keytype>
bool benchmark(const std::vector<keytype> &keys) {
    MPHFconfig conf(lambda, partitionSize);
    MPHFbuilder builder(conf);
    MPHF<pilotencoder, offsetencoder, hashfunction> f;

    if constexpr (std::is_same<pilotencoder, interleaved_encoder_dual<rice, compact>>::value) {
        f.getPilotEncoder().setEncoderTradeoff(tradeoff);
    }

    HostTimer timerConstruct;
    HostTimer timerInternal = builder.build(keys, f);
    timerConstruct.addLabel("total_construct");

    if (validate) {
        // check valid
        std::vector<bool> taken(keys.size(), false);
        for (size_t i = 0; i < keys.size(); i++) {
            size_t hash = f(keys.at(i));
            if (hash >= keys.size()) {
                std::cerr << "Out of range!" << std::endl;
                return 1;
            }
            if (taken[hash]) {
                std::cerr << "Collision by key " << i << "!" << std::endl;
                return 1;
            }
            taken[hash] = true;
        }
        // result is valid
        std::cout << "Valid result" << std::endl;
    }

    const std::string querytimeKey = "query_time";
    std::string benchResult = querytimeKey + "=--- ";
    if (queries > 0) {
        // bench
        std::vector<keytype> queryInputs;
        queryInputs.reserve(queries);
        for (int i = 0; i < queries; ++i) {
            uint64_t pos = dis(gen) % size;
            queryInputs.push_back(keys[pos]);
        }

        HostTimer timerQuery;
        for (int i = 0; i < queries; ++i) { DO_NOT_OPTIMIZE(f(queryInputs[i])); }
        timerQuery.addLabel(querytimeKey);
        benchResult = timerQuery.getResultStyle(queries);
    }

    std::cout << "RESULT " << f.getResultLine() << " " << benchResult
              << timerConstruct.getResultStyle(size) << " "
              << timerInternal.getResultStyle(size) << "size=" << size << " queries=" << queries
              << " l=" << lambda << " partition_size=" << partitionSize
              << " pilotencoder=" << f.getPilotEncoder().name()
              << " partitionencoder=" << offsetencoder::name()
              << " hashfunction=" << hashfunctionstring
              << " validated=" << validate
              << " buckets_per_partition=" << conf.bucketCountPerPartition << " "
              << App::getInstance().getInfoResultStyle() << std::endl;
    return true;
}


class XorShift64 {
private:
    uint64_t x64;
public:
    explicit XorShift64(uint64_t seed = 88172645463325252ull) : x64(seed) {
    }

    inline uint64_t operator()() {
        x64 ^= x64 << 13;
        x64 ^= x64 >> 7;
        x64 ^= x64 << 17;
        return x64;
    }

    inline uint64_t operator()(uint64_t range) {
#ifdef __SIZEOF_INT128__ // then we know we have a 128-bit int
        return (uint64_t)(((__uint128_t)operator()() * (__uint128_t)range) >> 64);
#elif defined(_MSC_VER) && defined(_WIN64)
        // supported in Visual Studio 2005 and better
        uint64_t highProduct;
        _umul128(operator()(), range, &highProduct); // ignore output
        return highProduct;
        unsigned __int64 _umul128(
            unsigned __int64 Multiplier,
            unsigned __int64 Multiplicand,
            unsigned __int64 *HighProduct
        );
#else
        return word / (UINT64_MAX / p); // fallback
#endif // __SIZEOF_INT128__
    }
};

std::vector<std::string> generateBenchmarkStringInput(size_t n) {
    std::vector<std::string> inputData;
    inputData.reserve(n);
    auto time = std::chrono::system_clock::now();
    long constructionTime = std::chrono::duration_cast<std::chrono::milliseconds>(time.time_since_epoch()).count();
    XorShift64 prng(constructionTime);
    std::cout<<"Generating input"<<std::flush;
    char string[200];
    for (size_t i = 0; i < n; i++) {
        if ((i % (n/5)) == 0) {
            std::cout<<"\rGenerating input: "<<100l*i/n<<"%"<<std::flush;
        }
        size_t length = 10 + prng((30 - 10) * 2);
        for (std::size_t k = 0; k < (length + sizeof(uint64_t))/sizeof(uint64_t); ++k) {
            ((uint64_t*) string)[k] = prng();
        }
        // Repair null bytes
        for (std::size_t k = 0; k < length; ++k) {
            if (string[k] == 0) {
                string[k] = 1 + prng(254);
            }
        }
        string[length] = 0;
        inputData.emplace_back(string, length);
    }
    std::cout<<"\rInput generation complete."<<std::endl;
    return inputData;
}

template<typename pilotstrat, typename partitionstrat, typename hashfunction>
bool dispatchKeyType() {
    if (keytypestring == "direct") {
        std::vector<Key> keys;
        keys.reserve(size);
        for (size_t i = 0; i < size; ++i) {
            keys.emplace_back(dis(gen), dis(gen), dis(gen), dis(gen));
        }
        return benchmark<pilotstrat, partitionstrat, hashfunction, Key>(keys);
    }
    if constexpr (!std::is_same<hashfunction, nohash>::value) {
        // all input types which are not supported by identity hashing
        if (keytypestring == "string") {
            return benchmark<pilotstrat, partitionstrat, hashfunction, std::string>(generateBenchmarkStringInput(size));
        }
    }
    return false;
}

template<typename pilotstrat, typename partitionstrat>
bool dispatchHashFunction() {
    if (hashfunctionstring == "none") {
        return dispatchKeyType<pilotstrat, partitionstrat, nohash>();
    }
    if (hashfunctionstring == "xx") {
        return dispatchKeyType<pilotstrat, partitionstrat, xxhash>();
    }
    return false;
}

template<typename pilotstrat, typename partitionbase>
bool dispatchPartitionEncoderStrat() {
    if (partitionencoderstrat == "diff") {
        return dispatchHashFunction<pilotstrat, diff_partition_encoder<partitionbase>>();
    }
    if (partitionencoderstrat == "direct") {
        return dispatchHashFunction<pilotstrat, direct_partition_encoder<partitionbase>>();
    }
    return false;
}

template<typename pilotencoderstrat>
bool dispatchEncoderBase(std::string basestring);

template<typename pilotbaseencoder>
bool dispatchPilotEncoderStrat() {
    if constexpr (!std::is_same<pilotbaseencoder, void>::value) {
        //requires base encoder
        if (pilotencoderstrat == "mono") {
            return dispatchEncoderBase<mono_encoder<pilotbaseencoder>>(partitionencoderbase);
        }
        if (pilotencoderstrat == "inter") {
            return dispatchEncoderBase<interleaved_encoder<pilotbaseencoder>>(partitionencoderbase);
        }
    }
    if (pilotencoderstrat == "dualinter") {
        return dispatchEncoderBase<interleaved_encoder_dual<rice, compact>>(partitionencoderbase);
    }
    return false;
}

template<typename pilotencoderstrat, typename base>
bool dispatchDynamic() {
    if constexpr (std::is_same<pilotencoderstrat, void>::value) {
        return dispatchPilotEncoderStrat<base>();
    } else {
        return dispatchPartitionEncoderStrat<pilotencoderstrat, base>();
    }
}

template<typename pilotencoderstrat>
bool dispatchEncoderBase(std::string basestring) {
    if (basestring == "c") { return dispatchDynamic<pilotencoderstrat, compact>(); }
    if (basestring == "ef") { return dispatchDynamic<pilotencoderstrat, elias_fano>(); }
    if (basestring == "d") { return dispatchDynamic<pilotencoderstrat, dictionary>(); }
    if (basestring == "r") { return dispatchDynamic<pilotencoderstrat, rice>(); }
    if (basestring == "sdc") { return dispatchDynamic<pilotencoderstrat, sdc>(); }
    // workaround for dual
    if constexpr (std::is_same<pilotencoderstrat, void>::value) {
        return dispatchDynamic<void, void>();
    }
    return false;
}

int main(int argc, char *argv[]) {
    App::getInstance().printDebugInfo();
    tlx::CmdlineParser cmd;
    uint64_t threads = 8;
    cmd.add_bytes('n', "size", size, "Number of objects to construct with");
    cmd.add_bytes('q', "queries", queries, "Number of queries for benchmarking or 0 for no benchmarking");
    cmd.add_double('l', "lambda", lambda, "Average number of elements in one bucket");
    cmd.add_bytes('p', "partitionsize", partitionSize, "Expected size of the partitions");
    cmd.add_string('e', "pilotencoderstrat", pilotencoderstrat, "The pilot encoding strategy");
    cmd.add_string('b', "pilotencoderbase", pilotencoderbase,
                   "The pilot encoding technique (ignored for dual)");
    cmd.add_double('d', "dualtradeoff", tradeoff,
                   "relative number of compact and rice encoder (only for dual)");
    cmd.add_string('s', "offsetencoderstrat", partitionencoderstrat,
                   "The partition offset encoding strategy");
    cmd.add_string('o', "offsetencoderbase", partitionencoderbase,
                   "The partition offset encoding technique");
    cmd.add_string('i', "hashfunction", hashfunctionstring,
                   "The initial hash function for the keys");
    cmd.add_string('k', "keytype", keytypestring,
                   "The type of the input keys");
    cmd.add_bool('v', "validate", validate, "Wether the MPHF is validated");
    cmd.add_bytes('t', "threads", threads, "omp_set_num_threads(t)");

    bool valid = cmd.process(argc, argv);
    if(valid) {
        omp_set_num_threads(threads);
        valid = dispatchEncoderBase<void>(pilotencoderbase);
    }
    if (!valid) {
        cmd.print_usage();
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
