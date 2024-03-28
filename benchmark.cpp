#include <chrono>
#include <tlx/cmdline_parser.hpp>

#include <gpuptmphf.hpp>

using namespace gpupthash;

#define DO_NOT_OPTIMIZE(value) asm volatile("" : : "r,m"(value) : "memory")

size_t size = 1e8;
size_t queries = 1e8;
double lambda = 7.5;
double tradeoff = 0.5;
size_t partitionSize = 2048;
std::string pilotencoderstrat = "dualmulti";
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

    if constexpr (std::is_same<pilotencoder, multi_encoder_dual<rice, compact>>::value) {
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
            std::vector<std::string> keys;
            keys.reserve(size);
            for (size_t i = 0; i < size; ++i) { keys.push_back(std::to_string(i)); }
            return benchmark<pilotstrat, partitionstrat, hashfunction, std::string>(keys);
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
bool dispatchPilotEncoderBase();

template<typename pilotbaseencoder>
bool dispatchPilotEncoderStrat() {
    if (pilotencoderstrat == "mono") {
        return dispatchPilotEncoderBase<mono_encoder<pilotbaseencoder>>();
    }
    if (pilotencoderstrat == "multi") {
        return dispatchPilotEncoderBase<multi_encoder<pilotbaseencoder>>();
    }
    if (pilotencoderstrat == "dualmulti") {
        return dispatchPilotEncoderBase<multi_encoder_dual<rice, compact>>();
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
bool dispatchPilotEncoderBase() {
    if (pilotencoderbase == "c") { return dispatchDynamic<pilotencoderstrat, compact>(); }
    if (pilotencoderbase == "ef") { return dispatchDynamic<pilotencoderstrat, elias_fano>(); }
    if (pilotencoderbase == "d") { return dispatchDynamic<pilotencoderstrat, dictionary>(); }
    if (pilotencoderbase == "r") { return dispatchDynamic<pilotencoderstrat, rice>(); }
    if (pilotencoderbase == "sdc") { return dispatchDynamic<pilotencoderstrat, sdc>(); }
    // workaround for dual, compact will be ignored
    return dispatchDynamic<pilotencoderstrat, compact>();
}

int main(int argc, char *argv[]) {
    App::getInstance().printDebugInfo();

    tlx::CmdlineParser cmd;
    cmd.add_bytes('n', "size", size, "Number of objects to construct with");
    cmd.add_bytes('q', "queries", queries, "Number of queries for benchmarking or 0 for no benchmarking");
    cmd.add_double('l', "lambda", lambda, "Average number of elements in one bucket");
    cmd.add_bytes('p', "partitionsize", partitionSize, "Expected size of the partitions");
    cmd.add_string('e', "pilotencoderstrat", pilotencoderstrat, "The pilot encoding strategy");
    cmd.add_string('b', "pilotencoderbase", pilotencoderbase,
                   "The pilot encoding technique (ignored for dual)");
    cmd.add_double('t', "dualtradeoff", tradeoff,
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

    bool valid = cmd.process(argc, argv) && dispatchPilotEncoderBase<void>();
    if (!valid) {
        cmd.print_usage();
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
