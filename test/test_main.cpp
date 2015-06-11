// https://github.com/philsquared/Catch/wiki/Supplying-your-own-main()
#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#include <google/protobuf/stubs/common.h>
#include <mapnik/datasource_cache.hpp>

int main (int argc, char* const argv[])
{
    try
    {
        GOOGLE_PROTOBUF_VERIFY_VERSION;
    }
    catch (std::exception const& ex) {
        std::clog << ex.what() << "\n";
        return -1;
    }
    mapnik::datasource_cache::instance().register_datasources(MAPNIK_PLUGINDIR);
    int result = Catch::Session().run( argc, argv );
    if (!result) printf("\x1b[1;32m ✓ \x1b[0m\n");
    google::protobuf::ShutdownProtobufLibrary();
    return result;
}
