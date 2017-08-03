#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include "vector_tile.pb.h"
#pragma GCC diagnostic pop

#include "vector_tile_compression.hpp"

#include <vector>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <sstream>

enum CommandType {
    SEG_END    = 0,
    SEG_MOVETO = 1,
    SEG_LINETO = 2,
    SEG_CLOSE = (0x40 | 0x0f)
};

enum eGeomType {
    Unknown = 0,
    Point = 1,
    LineString = 2,
    Polygon = 3
};

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
        std::clog << "please pass the path to an uncompressed, zlib-compressed, or gzip compressed protobuf tile\n";
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

        // now attempt to open protobuf
        vector_tile::Tile tile;
        bool is_zlib = mapnik::vector_tile_impl::is_zlib_compressed(message);
        bool is_gzip = mapnik::vector_tile_impl::is_gzip_compressed(message);
        if (is_zlib || is_gzip)
        {
            if (is_zlib)
            {
                std::cout << "message: zlib compressed\n";
            }
            else if (is_gzip)
            {
                std::cout << "message: gzip compressed\n";
            }
            std::string uncompressed;
            mapnik::vector_tile_impl::zlib_decompress(message,uncompressed);
            if (!tile.ParseFromString(uncompressed))
            {
                std::clog << "failed to parse compressed protobuf\n";
            }
        }
        else
        {
            std::cout << "message: appears not to be compressed\n";
            if (!tile.ParseFromString(message))
            {
                std::clog << "failed to parse protobuf\n";
            }
        }
        if (!verbose) {
            std::cout << "layers: " << static_cast<std::size_t>(tile.layers_size()) << "\n";
            for (std::size_t i=0;i<static_cast<std::size_t>(tile.layers_size());++i)
            {
                vector_tile::Tile_Layer const& layer = tile.layers(i);
                std::cout << layer.name() << ":\n";
                std::cout << "  version: " << layer.version() << "\n";
                std::cout << "  extent: " << layer.extent() << "\n";
                std::cout << "  features: " << static_cast<std::size_t>(layer.features_size()) << "\n";
                std::cout << "  keys: " << static_cast<std::size_t>(layer.keys_size()) << "\n";
                std::cout << "  values: " << static_cast<std::size_t>(layer.values_size()) << "\n";
                unsigned total_repeated = 0;
                unsigned num_commands = 0;
                unsigned num_move_to = 0;
                unsigned num_line_to = 0;
                unsigned num_close = 0;
                unsigned num_empty = 0;
                unsigned degenerate = 0;
                for (std::size_t j=0;j<static_cast<std::size_t>(layer.features_size());++j)
                {
                    vector_tile::Tile_Feature const & f = layer.features(j);
                    total_repeated += f.geometry_size();
                    int cmd = -1;
                    const int cmd_bits = 3;
                    unsigned length = 0;
                    unsigned g_length = 0;
                    for (int k = 0; k < f.geometry_size();)
                    {
                        if (!length) {
                            unsigned cmd_length = f.geometry(k++);
                            cmd = cmd_length & ((1 << cmd_bits) - 1);
                            length = cmd_length >> cmd_bits;
                            if (length <= 0) num_empty++;
                            num_commands++;
                        }
                        if (length > 0) {
                            length--;
                            if (cmd == SEG_MOVETO || cmd == SEG_LINETO)
                            {
                                f.geometry(k++);
                                f.geometry(k++);
                                g_length++;
                                if (cmd == SEG_MOVETO)
                                {
                                    num_move_to++;
                                }
                                else if (cmd == SEG_LINETO)
                                {
                                    num_line_to++;
                                }
                            }
                            else if (cmd == (SEG_CLOSE & ((1 << cmd_bits) - 1)))
                            {
                                if (g_length <= 2) degenerate++;
                                g_length = 0;
                                num_close++;
                            }
                            else
                            {
                                std::stringstream s;
                                s << "Unknown command type: " << cmd;
                                throw std::runtime_error(s.str());
                            }
                        }
                    }
                }
                std::cout << "  geometry summary:\n";
                std::cout << "    total: " << total_repeated << "\n";
                std::cout << "    commands: " << num_commands << "\n";
                std::cout << "    move_to: " << num_move_to << "\n";
                std::cout << "    line_to: " << num_line_to << "\n";
                std::cout << "    close: " << num_close << "\n";
                std::cout << "    degenerate polygons: " << degenerate << "\n";
                std::cout << "    empty geoms: " << num_empty << "\n";
            }
        } else {
            for (std::size_t j=0;j<static_cast<std::size_t>(tile.layers_size());++j)
            {
                vector_tile::Tile_Layer const& layer = tile.layers(j);
                std::cout << "layer: " << layer.name() << "\n";
                std::cout << "  version: " << layer.version() << "\n";
                std::cout << "  extent: " << layer.extent() << "\n";
                std::cout << "  keys: ";
                for (std::size_t k=0;k<static_cast<std::size_t>(layer.keys_size());++k)
                {
                     std::string const& key = layer.keys(k);
                     std::cout << key;
                     if (k<static_cast<std::size_t>(layer.keys_size())-1) {
                        std::cout << ",";
                     }
                }
                std::cout << "\n";
                std::cout << "  values: ";
                for (std::size_t l=0;l<static_cast<std::size_t>(layer.values_size());++l)
                {
                     vector_tile::Tile_Value const & value = layer.values(l);
                     if (value.has_string_value()) {
                          std::cout << value.string_value();
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
                     if (l<static_cast<std::size_t>(layer.values_size())-1) {
                        std::cout << ",";
                     }
                 }
                 std::cout << "\n";
                 for (std::size_t l=0;l<static_cast<std::size_t>(layer.features_size());++l)
                 {
                     vector_tile::Tile_Feature const & feat = layer.features(l);
                     if (feat.has_id()) {
                         std::cout << "  feature: " << feat.id() << "\n";
                     } else {
                         std::cout << "  feature: (no id set)\n";
                     }
                     std::cout << "    type: ";
                     unsigned feat_type = feat.type();
                     if (feat_type == 0) {
                         std::cout << "Unknown";
                     } else if (feat_type == 1) {
                         std::cout << "Point";
                     } else if (feat_type == 2) {
                         std::cout << "LineString";
                     } else if (feat_type == 3) {
                         std::cout << "Polygon";
                     }
                     std::cout << "\n";
                     std::cout << "    tags: ";
                     for (std::size_t m=0;m<static_cast<std::size_t>(feat.tags_size());++m)
                     {
                          uint32_t tag = feat.tags(j);
                          std::cout << tag;
                          if (m<static_cast<std::size_t>(feat.tags_size())-1) {
                            std::cout << ",";
                          }
                     }
                     std::cout << "\n";
                     std::cout << "    geometries: ";
                     for (std::size_t m=0;m<static_cast<std::size_t>(feat.geometry_size());++m)
                     {
                          uint32_t geom = feat.geometry(m);
                          std::cout << geom;
                          if (m<static_cast<std::size_t>(feat.geometry_size())-1) {
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
