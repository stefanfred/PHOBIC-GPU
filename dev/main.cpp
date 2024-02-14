#include "mphf_builder.h"
#include "encoders/mono_encoders.hpp"
#include "encoders/ortho_encoder_dual.hpp"




using namespace pthash;


#define DO_NOT_OPTIMIZE(value) asm volatile("" : : "r,m"(value) : "memory")


int main(int argc, char* argv[])
{
    std::cout.precision(3);
    std::cout << std::fixed;

    AppConfiguration config;
    config.appName = "MPHF";
    config.debugMode = true;
    App app(config);
    app.debugInfo();

    size_t size=1e7;

    MPHFbuilder builder(app, MPHFconfig(7.f, 1024));

    MPHF<mono_encoder<compact>, compact> f;

    std::vector<Key> keys;
    keys.reserve(size);

    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint32_t> dis;

    for (size_t i = 0; i < size; ++i) {
        keys.push_back({ dis(gen),dis(gen),dis(gen),dis(gen) });
    }

    builder.build(keys, f);

    std::vector<bool> taken(keys.size(), false);
    for (size_t i = 0; i < keys.size(); i++) {
        size_t hash = f(keys.at(i));
        if (hash > keys.size()) {
            std::cerr << "Out of range!" << std::endl;
            exit(1);
        }
        if (taken[hash]) {
            std::cerr << "Collision by key " << i << "!" << std::endl;
            exit(1);
        }
        taken[hash] = true;
    }
    std::cout<<"valid for "<< keys.size()<<std::endl;
}
