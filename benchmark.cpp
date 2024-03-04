#include <chrono>
#include <tlx/cmdline_parser.hpp>

#include <gpuptmphf.hpp>

using namespace gpupthash;

#define DO_NOT_OPTIMIZE(value) asm volatile ("" : : "r,m"(value) : "memory")

size_t size = 1e8;
size_t queries = 1e6;
double A = 7.5;
double tradeoff = 0.5;
size_t partitionSize = 2048;
std::string pilotencoderstrat = "dualortho";
std::string pilotencoderbase = "c";
std::string partitionencoderstrat = "diff";
std::string partitionencoderbase = "c";
std::string hashfunctionstring = "none";

template<typename pilotencoder, typename offsetencoder, typename hashfunction>
int benchmark() {
    std::vector<Key> keys;
    keys.reserve(size);
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint32_t> dis;
    for (size_t i = 0; i < size; ++i) {
        keys.push_back(Key(dis(gen), dis(gen), dis(gen), dis(gen)));
    }

    MPHFbuilder builder(MPHFconfig(A, partitionSize));
    MPHF<pilotencoder, offsetencoder, hashfunction> f;

    if constexpr (std::is_same<pilotencoder, ortho_encoder_dual<compact, golomb>>::value) {
        f.getPilotEncoder().setEncoderTradeoff(tradeoff);
    }

    HostTimer timerConstruct;
    HostTimer timerInternal = builder.build(keys, f);
    timerConstruct.addLabel("total_construct");


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


    //bench
    std::vector<Key> queryInputs;
    queryInputs.reserve(size);
    for (int i = 0; i < queries; ++i) {
        uint64_t pos = dis(gen) % size;
        queryInputs.push_back(keys[pos]);
    }

    HostTimer timerQuery;
    for (int i = 0; i < queries; ++i) {
        DO_NOT_OPTIMIZE(f(queryInputs[i]));
    }
    timerQuery.addLabel("query_time");

    std::cout << "RESULT "
              << timerQuery.getResultStyle(queries)
              << timerConstruct.getResultStyle(size)
              << "bits_per_key=" << f.getBitsPerKey() << " "
              << timerInternal.getResultStyle(size)
              << "size=" << size
              << " queries=" << queries
              << " A=" << A
              << " partition_size=" << partitionSize
              << " pilotencoder=" << f.getPilotEncoder().name()
              << " partitionencoder=" << offsetencoder::name()
              << " hashfunction=" << hashfunctionstring
              << " " << App::getInstance().getInfoResultStyle()
              << std::endl;
    return 0;
}

template<typename pilotstrat, typename partitionstrat>
int dispatchHashFunction() {
    if (hashfunctionstring == "none") {
        return benchmark<pilotstrat, partitionstrat, nohash>();
    }
    if (hashfunctionstring == "murmur2") {
        return benchmark<pilotstrat, partitionstrat, murmurhash2>();
    }
    return 1;
}

template<typename pilotstrat, typename partitionbase>
int dispatchPartitionEncoderStrat() {
    if (partitionencoderstrat == "diff") {
        return dispatchHashFunction<pilotstrat, diff_partition_encoder<partitionbase>>();
    }
    if (partitionencoderstrat == "direct") {
        return dispatchHashFunction<pilotstrat, direct_partition_encoder<partitionbase>>();
    }
    return 1;
}

template<typename pilotencoderstrat>
int dispatchPilotEncoderBase();

template<typename pilotbaseencoder>
int dispatchPilotEncoderStrat() {
    if (pilotencoderstrat == "mono") {
        return dispatchPilotEncoderBase<mono_encoder<pilotbaseencoder>>();
    }
    if (pilotencoderstrat == "ortho") {
        return dispatchPilotEncoderBase<ortho_encoder<pilotbaseencoder>>();
    }
    if (pilotencoderstrat == "dualortho") {
        return dispatchPilotEncoderBase<ortho_encoder_dual<golomb, compact>>();
    }
    return 1;
}


template<typename pilotencoderstrat, typename base>
int dispatchDynamic() {
    if constexpr (std::is_same<pilotencoderstrat, void>::value) {
        return dispatchPilotEncoderStrat<base>();
    } else {
        return dispatchPartitionEncoderStrat<pilotencoderstrat, base>();
    }
}

template<typename pilotencoderstrat>
int dispatchPilotEncoderBase() {
    if (pilotencoderbase == "c") {
        return dispatchDynamic<pilotencoderstrat, compact>();
    }
    if (pilotencoderbase == "ef") {
        return dispatchDynamic<pilotencoderstrat, elias_fano>();
    }
    if (pilotencoderbase == "d") {
        return dispatchDynamic<pilotencoderstrat, dictionary>();
    }
    if (pilotencoderbase == "r") {
        return dispatchDynamic<pilotencoderstrat, golomb>();
    }
    if (pilotencoderbase == "sdc") {
        return dispatchDynamic<pilotencoderstrat, sdc>();
    }
    // workaround for dual, compact will be ignored
    return dispatchDynamic<pilotencoderstrat, compact>();
}


int main(int argc, char *argv[]) {
    App::getInstance().printDebugInfo();

    tlx::CmdlineParser cmd;
    cmd.add_bytes('n', "size", size, "Number of objects to construct with");
    cmd.add_bytes('q', "queries", size, "Number of queries for benchmarking");
    cmd.add_double('a', "avgbucketsize", A, "Average number of objects in one bucket");
    cmd.add_bytes('p', "partitionsize", partitionSize, "Expected size of the partitions");
    cmd.add_string('e', "pilotencoderstrat", pilotencoderstrat, "The pilot encoding strategy");
    cmd.add_string('b', "pilotencoderbase", pilotencoderbase, "The pilot encoding technique (ignored for dual)");
    cmd.add_double('t', "dualtradeoff", tradeoff, "relative number of compact and golomb encoder (only for dual)");
    cmd.add_string('s', "offsetencoderstrat", partitionencoderstrat, "The partition offset encoding strategy");
    cmd.add_string('o', "offsetencoderbase", partitionencoderbase, "The partition offset encoding technique");
    cmd.add_string('h', "hashfunction", hashfunctionstring, "The initial hash function for the keys");

    if (!cmd.process(argc, argv)) {
        cmd.print_usage();
        return 1;
    }


    return dispatchPilotEncoderBase<void>();
}
