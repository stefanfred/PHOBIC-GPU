#include "mphf_builder.h"
#include "encoders/mono_encoders.hpp"


using namespace pthash;


int main(int argc, char* argv[])
{
    try
    {
        std::cout.precision(3);
        std::cout << std::fixed;

        AppConfiguration config;
        config.appName = "";
        config.debugMode = false;
        App app(config);
        app.debugInfo();
        

        MPHFbuilder builder(app, MPHFconfig(3.f, 1024, PTBucketer()));

        MPHF<mono_encoder<compact>, compact> f;
        builder.buildRandomized(1e6, &f, 4);
    }
    catch (std::exception &err)
    {
        std::cout << "std::exception: " << err.what() << std::endl;
        exit(-1);
    }
    catch (...)
    {
        std::cout << "unknown error\n";
        exit(-1);
    }
    return 0;
}
