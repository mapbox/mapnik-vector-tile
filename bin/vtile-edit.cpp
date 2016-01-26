#include <mapnik/util/file_io.hpp>
#include "vector_tile.pb.h"
#include "vector_tile_compression.hpp"

#include <iostream>
#include <fstream>

#include <protozero/pbf_reader.hpp>

int main(int argc, char** argv)
{
    try
    {
        if (argc < 2)
        {
            std::clog << "usage: vtile-edit /path/to/tile.vector.mvt [--set-version version]" << std::endl;
            return -1;
        }
        std::string vtile(argv[1]);
        mapnik::util::file input(vtile);
        if (!input.open())
        {
            std::clog << std::string("failed to open ") + vtile << "\n";
            return -1;
        }

        unsigned version = 1;
        std::string flag = argv[2];
        if (flag == "--set-version")
        {
            version = std::stoi(argv[3]);
        }

        std::string message(input.data().get(), input.size());

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
            message = uncompressed;
        }

        vector_tile::Tile tile;
        tile.ParseFromString(message);
        
        for (int j=0;j<tile.layers_size(); ++j)
        {
            auto layer = tile.mutable_layers(j);
            std::clog << "layer: " << layer->name() << std::endl;
            std::clog << "old version: " << layer->version() << std::endl;
            if (version != layer->version())
            {
                layer->set_version(version);
            }
            std::clog << "new version: " << layer->version() << std::endl;
        }

        // Serialize
        std::string output;
        tile.SerializeToString(&output);

        // Recompress
        bool gzip = true;
        if (is_zlib || is_gzip)
        {
            if (is_zlib)
            {
                gzip = false;
                std::cout << "message: zlib compressing\n";
            }
            else if (is_gzip)
            {
                gzip = true;
                std::cout << "message: gzip compressing\n";
            }
            std::string compressed;
            mapnik::vector_tile_impl::zlib_compress(output,compressed,gzip);
            output = compressed;
        }

        std::ofstream file(vtile);
        file << output;

        std::clog << "wrote to: " << vtile << std::endl;
    }
    catch (std::exception const& ex)
    {
        std::clog << "error: " << ex.what() << "\n";
        return -1;
    }
    return 0;
}
