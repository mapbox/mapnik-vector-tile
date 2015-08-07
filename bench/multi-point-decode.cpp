#include <mapnik/timer.hpp>
#include <mapnik/util/file_io.hpp>
#include "vector_tile_datasource_pbf.hpp"
#include "vector_tile_datasource.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include "vector_tile.pb.h"
#pragma GCC diagnostic pop

#include <iostream>

#include <protozero/pbf_reader.hpp>

void decode_geometry(protozero::pbf_reader tile)
{
    tile.next();
    protozero::pbf_reader layer = tile.get_message();
    while (layer.next())
    {
        switch(layer.tag())
        {
            case 2:
                {
                    protozero::pbf_reader feature = layer.get_message();
                    // TODO
                }
                break;
            default:
                {
                    layer.skip();
                }
                break;
        }
    }
}


int main() {
    std::string vtile("./bench/enf.t5yd5cdi_14_13089_8506.vector.pbf");
    mapnik::util::file input(vtile);
    if (!input.open())
    {
        throw std::runtime_error("failed to open vtile");
    }
    std::size_t iterations = 100;

    /*
    {
        mapnik::progress_timer __stats__(std::clog, std::string("decode: ") + vtile);
        for (std::size_t i=0;i<iterations;++i)
        {
            protozero::pbf_reader tile(input.data().get(), input.size());
            decode_geometry(tile);
        }
    }
    */
    std::size_t count = 0;
    {
        mapnik::progress_timer __stats__(std::clog, std::string("decode as datasource_pbf: ") + vtile);
        for (std::size_t i=0;i<iterations;++i)
        {
            protozero::pbf_reader tile(input.data().get(), input.size());
            tile.next();
            protozero::pbf_reader layer = tile.get_message();
            auto ds = std::make_shared<mapnik::vector_tile_impl::tile_datasource_pbf>(layer,13089,8506,14,256);
            mapnik::query q(ds->get_tile_extent());
            auto fs = ds->features(q);
            while (fs->next()) { 
                ++count;
            }
        }
    }
    std::size_t count2 = 0;
    {
        mapnik::progress_timer __stats__(std::clog, std::string("decode as datasource: ") + vtile);
        std::string buffer(input.data().get(), input.size());
        vector_tile::Tile tiledata;
        tiledata.ParseFromString(buffer);
        for (std::size_t i=0;i<iterations;++i)
        {
            protozero::pbf_reader tile(input.data().get(), input.size());
            tile.next();
            protozero::pbf_reader layer = tile.get_message();
            auto ds = std::make_shared<mapnik::vector_tile_impl::tile_datasource>(tiledata.layers(0),13089,8506,14,256);
            mapnik::query q(ds->get_tile_extent());
            auto fs = ds->features(q);
            while (fs->next()) { 
                ++count2;
            }
        }
    }
    if (count != count2) {
        std::clog << "error: tile datasource impl did not return same # of features " << count  << " vs " << count2 << "\n";
    }
    return 0;
}
