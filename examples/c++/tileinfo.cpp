#include "../../src/vector_tile.pb.h"
#include <vector>
#include <iostream>
#include <fstream>
#include <stdexcept>


int main(int argc, char** argv)
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    std::vector<std::string> args;
    for (int i=1;i<argc;++i)
    {
        args.push_back(argv[i]);
    }
    
    if (args.empty())
    {
        std::clog << "please pass the path to an uncompressed protobuf tile (.vector.pbf)\n";
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
        if (!tile.ParseFromString(message))
        {
            std::clog << "failed to parse protobuf\n";
        }
        
        std::cout << "number of layers: " << tile.layers_size() << "\n";

        for (unsigned i=0;i<tile.layers_size();++i)
        {
            mapnik::vector::tile_layer const& layer = tile.layers(i);
            std::cout << "  layer: " << i << "\n";
            std::cout << "  layer name: " << layer.name() << "\n";
            std::cout << "  layer extent: " << layer.extent() << "\n";
            std::cout << "  layer features: " << layer.features_size() << "\n";
            for (unsigned j=0;j<layer.features_size();++j)
            {
                mapnik::vector::tile_feature const& feature = layer.features(j);
                std::cout << "    feature: " << j << "\n";
                std::cout << "    feature id: " << feature.id() << "\n";
                std::cout << "    feature type: " << feature.type() << "\n";
                // TODO - dump tags and geometries
            }
        }

    }
    catch (std::exception const& ex)
    {
        std::clog << "error: " << ex.what() << "\n";
        return -1;
    }
    google::protobuf::ShutdownProtobufLibrary();

}