#include <mapnik/timer.hpp>
#include <mapnik/util/file_io.hpp>
#include "vector_tile_datasource_pbf.hpp"
#include "vector_tile_datasource.hpp"
#include "vector_tile_compression.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include "vector_tile.pb.h"
#pragma GCC diagnostic pop

#include <iostream>

#include <protozero/pbf_reader.hpp>

int main(int argc, char** argv)
{
    try
    {

        if (argc < 4)
        {
            std::clog << "usage: vtile-decode /path/to/tile.vector.mvt z x y [iterations]\n";
            return -1;
        }
        std::string vtile(argv[1]);
        mapnik::util::file input(vtile);
        if (!input.open())
        {
            std::clog << std::string("failed to open ") + vtile << "\n";
            return -1;
        }

        int z = std::stoi(argv[2]);
        int x = std::stoi(argv[3]);
        int y = std::stoi(argv[4]);

        std::size_t iterations = 100;
        if (argc > 5)
        {
            iterations = std::stoi(argv[5]);
        }

        std::clog << "z:" << z << " x:" << x << " y:" << y <<  " iterations:" << iterations << "\n";

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

        std::size_t feature_count = 0;
        std::size_t layer_count = 0;
        {
            mapnik::progress_timer __stats__(std::clog, std::string("decode as datasource_pbf: ") + vtile);
            for (std::size_t i=0;i<iterations;++i)
            {
                protozero::pbf_reader tile(message);
                while (tile.next(3)) {
                    ++layer_count;
                    protozero::pbf_reader layer = tile.get_message();
                    auto ds = std::make_shared<mapnik::vector_tile_impl::tile_datasource_pbf>(layer,x,y,z);
                    mapnik::query q(ds->get_tile_extent());
                    auto fs = ds->features(q);
                    while (fs->next()) {
                        ++feature_count;
                    }
                }
            }
        }


        std::size_t feature_count2 = 0;
        std::size_t layer_count2 = 0;
        {
            mapnik::progress_timer __stats__(std::clog, std::string("decode as datasource: ") + vtile);
            vector_tile::Tile tiledata;
            tiledata.ParseFromString(message);
            for (std::size_t i=0;i<iterations;++i)
            {
                for (int j=0;j<tiledata.layers_size(); ++j)
                {
                    ++layer_count2;
                    auto const& layer = tiledata.layers(j);
                    auto ds = std::make_shared<mapnik::vector_tile_impl::tile_datasource>(layer,x,y,z);
                    mapnik::query q(ds->get_tile_extent());
                    auto fs = ds->features(q);
                    while (fs->next()) {
                        ++feature_count2;
                    }                    
                }
            }
        }
        if (feature_count!= feature_count2) {
            std::clog << "error: tile datasource impl did not return same # of features " << feature_count << " vs " << feature_count2 << "\n";
            return -1;
        } else if (feature_count == 0) {
            std::clog << "error: no features processed\n";
            return -1;
        } else {
            std::clog << "processed " << feature_count << " features\n";
        }
    }
    catch (std::exception const& ex)
    {
        std::clog << "error: " << ex.what() << "\n";
        return -1;
    }
    return 0;
}
