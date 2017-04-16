#include "../../src/vector_tile.pb.h"
#include "../../src/vector_tile_compression.hpp"
#include <vector>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <sstream>

int main(int argc, char** argv)
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    std::vector<std::string> args;
    bool verbose = false;
    for (int i=1;i<argc;++i)
    {
        if (strcmp(argv[i], "--verbose") == 0) {
            verbose = true;
        } else {
            args.push_back(argv[i]);
        }
    }
    
    if (args.empty())
    {
        std::clog << "please pass the path to an uncompressed or zlib-compressed protobuf tile\n";
        return -1;
    }
    
    try
    {
        std::string filename = args[0];
        std::ifstream stream(filename.c_str(),std::ios_base::in|std::ios_base::binary);
        if (!stream.is_open())
        {
            throw std::runtime_error("could not open: '" + filename + "'");
        }
        
        // we are using lite library, so we copy to a string instead of using ParseFromIstream
        std::string message(std::istreambuf_iterator<char>(stream.rdbuf()),(std::istreambuf_iterator<char>()));
        stream.close();

        // now attemp to open protobuf
        mapnik::vector::tile tile;
        if (mapnik::vector::is_compressed(message))
        {
            std::clog << "INPUT IS COMPRESSED, nothing to do\n";
        }
        else
        {
            std::string compressed;
            mapnik::vector::compress(message,compressed);
            filename += 'z';
            std::ofstream ostream(filename.c_str(),std::ios_base::out|std::ios_base::binary);
            ostream << compressed;
        }
    }
    catch (std::exception const& ex)
    {
        std::clog << "error: " << ex.what() << "\n";
        return -1;
    }
    google::protobuf::ShutdownProtobufLibrary();

}
