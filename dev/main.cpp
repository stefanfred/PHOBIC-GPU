#include <chrono>
#include <tlx/cmdline_parser.hpp>

#include <gpuptmphf.hpp>

using namespace gpupthash;

int main(int argc, char *argv[]) {
    std::cout.precision(3);
    std::cout << std::fixed;
    size_t size;

    tlx::CmdlineParser cmd;
    cmd.add_bytes('n', "size", size, "Number of objects to construct with");
    if (!cmd.process(argc, argv)) {
        return 1;
    }

    App::getInstance().debugInfo();


    std::vector<Key> keys;
    keys.reserve(size);
    std::random_device rd;
    std::mt19937_64 gen(0);
    std::uniform_int_distribution<uint32_t> dis;
    for (size_t i = 0; i < size; ++i) {
        keys.push_back(Key(dis(gen), dis(gen), dis(gen), dis(gen)));
    }

    MPHFbuilder builder(MPHFconfig(2.5f, 2048));
    MPHF<ortho_encoder<golomb>, diff_partition_encoder<compact>, nohash> f;
    //SmallMphf f;
    auto beginConstruction = std::chrono::high_resolution_clock::now();
    builder.build(keys, f);
    unsigned long constructionDurationMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::high_resolution_clock::now() - beginConstruction).count();
    std::cout << "Construction took " << constructionDurationMs << " ms" << std::endl;
    std::cout << "Space required " << f.getBitsPerKey() << " bits per key" << std::endl;

    std::vector<bool> taken(keys.size(), false);
    for (size_t i = 0; i < keys.size(); i++) {
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
