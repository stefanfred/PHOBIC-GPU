#include <chrono>
#include <mphf_builder.h>
#include <encoders/pilotEncoders/mono_encoders.hpp>
#include <encoders/partitionOffsetEnocders/direct_partition_offset_encoder.hpp>
#include <encoders/partitionOffsetEnocders/diff_partition_offset_encoder.hpp>
#include <tlx/cmdline_parser.hpp>

int main(int argc, char *argv[]) {
    std::cout.precision(3);
    std::cout << std::fixed;
    size_t size = 1e7;

    tlx::CmdlineParser cmd;
    cmd.add_bytes('n', "size", size, "Number of objects to construct with");
    if (!cmd.process(argc, argv)) {
        return 1;
    }

    AppConfiguration config;
    config.appName = "MPHF";
    config.debugMode = true;
    App app(config);
    app.debugInfo();

    std::vector<Key> keys;
    keys.reserve(size);
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint32_t> dis;
    for (size_t i = 0; i < size; ++i) {
        keys.push_back({dis(gen), dis(gen), dis(gen), dis(gen)});
    }

    MPHFbuilder builder(app, MPHFconfig(7.f, 1024));
    MPHF<mono_encoder<compact>, diff_partition_encoder<compact>> f;
    auto beginConstruction = std::chrono::high_resolution_clock::now();
    builder.build(keys, f);
    unsigned long constructionDurationMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::high_resolution_clock::now() - beginConstruction).count();
    std::cout << "Construction took " << constructionDurationMs << " ms" << std::endl;

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
