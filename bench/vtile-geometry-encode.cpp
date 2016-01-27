#include <mapnik/util/file_io.hpp>
#include <mapnik/timer.hpp>
#include "vector_tile_geometry_encoder.hpp"
#include "vector_tile_projection.hpp"
#include "vector_tile_strategy.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include "vector_tile.pb.h"
#pragma GCC diagnostic pop

#include <iostream>
#include <fstream>

// mapnik
#include <mapnik/box2d.hpp>
#include <mapnik/projection.hpp>
#include <mapnik/geometry_transform.hpp>
#include <mapnik/json/geometry_parser.hpp>

int main(int argc, char** argv)
{
    try
    {

        // Note: This script only works for partial geojson that just
        // have a single feature, not FeatureCollections
        if (argc < 4)
        {
            std::clog << "usage: vtile-encode-geometry /path/to/geometry.geojson z x y [iterations]\n";
            return -1;
        }
        std::string geojson_file(argv[1]);

        int z = std::stoi(argv[2]);
        int x = std::stoi(argv[3]);
        int y = std::stoi(argv[4]);

        std::size_t iterations = 1000;
        std::string output_path = "";
        if (argc > 5)
        {
            for (int i = 5; i < argc; i++)
            {
                std::string flag = argv[i];
                if (flag == "-i")
                {
                    iterations = std::stoi(argv[i + 1]);
                }
                if (flag == "-o")
                {
                    output_path = argv[i + 1];
                }
            }
        }

        // Maybe might do this differently, depending on `encode_geometry` parameters
        std::clog << "z:" << z << " x:" << x << " y:" << y <<  " iterations:" << iterations << std::endl;

        // Change zxy to bbox 
        unsigned tile_size = 256;

        double minx,miny,maxx,maxy;
        mapnik::vector_tile_impl::spherical_mercator merc_tiler(tile_size);
        merc_tiler.xyz(x, y, z, minx, miny, maxx, maxy);
        mapnik::box2d<double> bbox(minx,miny,maxx,maxy);

        // Create geometry from geojson
        mapnik::util::file input(geojson_file);
        if (!input.open())
        {
            std::clog << "failed to open " << geojson_file << std::endl;
            return -1;
        }
        mapnik::geometry::geometry<double> geom;
        std::string json_string(input.data().get(), input.size());
        if (!mapnik::json::from_geojson(json_string, geom))
        {
            std::clog << "failed to parse geojson" << std::endl;
            return -1;
        }

        // Transform geojson from lng,lat coordinates to tile coords
        // 1. Correct
        // 2. Reprojection
        // 3. Clipping
        mapnik::geometry::correct(geom);

        mapnik::projection merc("+init=epsg:3857",true);
        mapnik::projection merc2("+init=epsg:4326",true);
        mapnik::proj_transform prj_trans2(merc2,merc); // op
        mapnik::view_transform tr(tile_size,tile_size,bbox,0,0);
        mapnik::vector_tile_impl::vector_tile_strategy_proj vs(prj_trans2,tr, 16);
        mapnik::box2d<double> clip_extent(std::numeric_limits<double>::min(),
                                       std::numeric_limits<double>::min(),
                                       std::numeric_limits<double>::max(),
                                       std::numeric_limits<double>::max());
        mapnik::vector_tile_impl::transform_visitor<mapnik::vector_tile_impl::vector_tile_strategy_proj> transit(vs, clip_extent);
        mapnik::geometry::geometry<std::int64_t> new_geom = mapnik::util::apply_visitor(transit,geom);        

        // Time how long encode_geometry takes
        {
            mapnik::progress_timer __stats__(std::clog, std::string("encode tile: ") + geojson_file);
            for (std::size_t i = 0; i < iterations; ++i)
            {
                vector_tile::Tile_Feature feature;
                int32_t x_=0, y_=0;
                mapnik::vector_tile_impl::encode_geometry(new_geom, feature, x_, y_);
            }
        }

        // Output buffer
        if (output_path != "")
        {
            // Encode the geometry into a tile, and serialize it.
            vector_tile::Tile tile;
            vector_tile::Tile_Layer* layer = tile.add_layers();
            layer->set_version(2);
            layer->set_name("test");
            vector_tile::Tile_Feature* feature = layer->add_features();
            int32_t x_=0, y_=0;
            mapnik::vector_tile_impl::encode_geometry(new_geom, *feature, x_, y_);

            std::string output_buffer;
            tile.SerializeToString(&output_buffer);
            std::ofstream out(output_path);
            out << output_buffer;
        }
    }
    catch (std::exception const& ex)
    {
        std::clog << "error: " << ex.what() << std::endl;
        return -1;
    }
    return 0;
}
