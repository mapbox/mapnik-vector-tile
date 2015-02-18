// https://github.com/philsquared/Catch/wiki/Supplying-your-own-main()
#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#include <google/protobuf/stubs/common.h>

int main (int argc, char* const argv[])
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    int result = Catch::Session().run( argc, argv );
    if (!result) printf("\x1b[1;32m ✓ \x1b[0m\n");
    google::protobuf::ShutdownProtobufLibrary();
    return result;
}
