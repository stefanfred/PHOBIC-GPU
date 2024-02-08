#include "mphf_builder.h"
#include "encoders/mono_encoders.hpp"
#include "encoders/ortho_encoder_dual.hpp"


using namespace pthash;


int main(int argc, char* argv[])
{
    std::cout.precision(3);
    std::cout << std::fixed;

    AppConfiguration config;
    config.appName = "";
    config.debugMode = false;
    App app(config);
    app.debugInfo();


    MPHFbuilder builder(app, MPHFconfig(9.f, 1024, PTBucketer()));

    MPHF<mono_encoder<compact>, compact> f;
    builder.buildRandomized(1e6, &f, 4);
    return 0;
}
