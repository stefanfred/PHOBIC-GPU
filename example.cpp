#include <chrono>

// include all required gpuMPHF headers
#include <gpuptmphf.hpp>

using namespace gpupthash;

int main(int argc, char *argv[]) {
    std::cout.precision(3);
    std::cout << std::fixed;

    // print name of the GPU which will be used
    App::getInstance().printDebugInfo();

    // number of elements
    size_t size = 1e5;

    // create some random input
    std::vector<std::string> keys;
    keys.reserve(size);
    for (int i = 0; i < size; ++i) {
        keys.push_back(std::to_string(i));
    }

    // initialize the GPU builder using a particular config
    // the average bucket size tradeoffs construction time with space consumption
    // the partition size should be tuned for the particular hardware
    // the builder can be reused
    MPHFbuilder builder(MPHFconfig(2.5f, 2048));

    // the minimal perfect hash function
    // the templates are <pilot_encoding, partition_offset_encoding, initial_hash_function>
    MPHF<ortho_encoder<golomb>, diff_partition_encoder<compact>, murmurhash2> f;

    // measure construction time
    auto beginConstruction = std::chrono::high_resolution_clock::now();
    builder.build(keys, f);
    unsigned long constructionDurationMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::high_resolution_clock::now() - beginConstruction).count();
    std::cout << "Construction took " << constructionDurationMs << " ms" << std::endl;
    std::cout << "Space required " << f.getBitsPerKey() << " bits per key" << std::endl;

    // check that the MPHF is valid
    std::vector<bool> taken(keys.size(), false);
    for (size_t i = 0; i < keys.size(); i++) {
        // use f(key) to query the MPHF

        size_t hash = f(keys.at(i));
        if (hash >= keys.size()) {
            std::cerr << "Out of range!" << std::endl;
            exit(1);
        }
        if (taken[hash]) {
            std::cerr << "Collision by key " << i << "!" << std::endl;
            exit(1);
        }
        taken[hash] = true;
    }
    std::cout << "valid for " << keys.size() << std::endl;
}
