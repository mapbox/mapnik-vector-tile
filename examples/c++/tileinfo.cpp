#include "../../src/vector_tile.pb.h"
#include "../../src/vector_tile_compression.hpp"
#include <vector>
#include <iostream>
#include <fstream>
#include <stdexcept>

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
            std::string uncompressed;
            mapnik::vector::decompress(message,uncompressed);
            if (!tile.ParseFromString(uncompressed))
            {
                std::clog << "failed to parse compressed protobuf\n";
            }
        }
        else
        {
            if (!tile.ParseFromString(message))
            {
                std::clog << "failed to parse protobuf\n";
            }
        }
        if (!verbose) {
            std::cout << "layers: " << tile.layers_size() << "\n";
            for (unsigned i=0;i<tile.layers_size();++i)
            {
                mapnik::vector::tile_layer const& layer = tile.layers(i);
                std::cout << layer.name() << ":\n";
                std::cout << "  version: " << layer.version() << "\n";
                std::cout << "  extent: " << layer.extent() << "\n";
                std::cout << "  features: " << layer.features_size() << "\n";
                std::cout << "  keys: " << layer.keys_size() << "\n";
                std::cout << "  values: " << layer.values_size() << "\n";
            }
        } else {
            for (unsigned i=0;i<tile.layers_size();++i)
            {
                mapnik::vector::tile_layer const& layer = tile.layers(i);
                std::cout << "layer: " << layer.name() << "\n";
                std::cout << "  version: " << layer.version() << "\n";
                std::cout << "  extent: " << layer.extent() << "\n";
                std::cout << "  keys: ";
                for (unsigned i=0;i<layer.keys_size();++i)
                {
                     std::string const& key = layer.keys(i);
                     std::cout << key << ",";
                }
                std::cout << "\n";
                std::cout << "  values: ";
                for (unsigned i=0;i<layer.values_size();++i)
                {
                     mapnik::vector::tile_value const & value = layer.values(i);
                     if (value.has_string_value()) {
                          std::cout << "'" << value.string_value();
                     } else if (value.has_int_value()) {
                          std::cout << value.int_value();
                     } else if (value.has_double_value()) {
                          std::cout << value.double_value();
                     } else if (value.has_float_value()) {
                          std::cout << value.float_value();
                     } else if (value.has_bool_value()) {
                          std::cout << value.bool_value();
                     } else if (value.has_sint_value()) {
                          std::cout << value.sint_value();
                     } else if (value.has_uint_value()) {
                          std::cout << value.uint_value();
                     } else {
                          std::cout << "null";
                     }
                     if (i<layer.values_size()-1) {
                        std::cout << ",";
                     }
                 }
                 std::cout << "\n";
                 for (unsigned i=0;i<layer.features_size();++i)
                 {
                     mapnik::vector::tile_feature const & feat = layer.features(i);
                     std::cout << "  feature: " << feat.id() << " " << feat.type() << "\n";
                     std::cout << "    tags: ";
                     for (unsigned j=0;j<feat.tags_size();++j)
                     {
                          uint32_t tag = feat.tags(j);
                          std::cout << tag;
                          if (j<feat.tags_size()-1) {
                            std::cout << ",";
                          }
                     }
                     std::cout << "\n";
                     std::cout << "    geometries: ";
                     for (unsigned j=0;j<feat.geometry_size();++j)
                     {
                          uint32_t geom = feat.geometry(j);
                          std::cout << geom;
                          if (j<feat.geometry_size()-1) {
                            std::cout << ",";
                          }
                     }
                     std::cout << "\n";
                 }
                 std::cout << "\n";
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
