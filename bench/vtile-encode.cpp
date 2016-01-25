#include <mapnik/timer.hpp>
#include <mapnik/util/file_io.hpp>
#include "vector_tile_backend_pbf.hpp"
#include "vector_tile_processor.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include "vector_tile.pb.h"
#pragma GCC diagnostic pop

#include <iostream>
#include <fstream>

// mapnik
#include <mapnik/box2d.hpp>
#include <mapnik/datasource_cache.hpp>
#include <mapnik/global.hpp>
#include <mapnik/well_known_srs.hpp>
#include <mapnik/json/geometry_parser.hpp>
#include <mapnik/memory_datasource.hpp>
#include <mapnik/feature_factory.hpp>
#include <mapnik/map.hpp>
#include <mapnik/layer.hpp>
#include <mapnik/request.hpp>

#include <protozero/pbf_reader.hpp>

typedef mapnik::vector_tile_impl::backend_pbf backend_type;
typedef mapnik::vector_tile_impl::processor<backend_type> renderer_type;

void from_pixels(double shift, double & x, double & y)
{
    double b = shift/2.0;
    x = (x - b)/(shift/360.0);
    double g = (y - b)/-(shift/(2 * M_PI));
    y = mapnik::R2D * (2.0 * std::atan(std::exp(g)) - mapnik::M_PI_by2);
}

void xyz(int tile_size,
    int x,
    int y,
    int z,
    double & minx,
    double & miny,
    double & maxx,
    double & maxy)
{
    minx = x * tile_size;
    miny = (y + 1.0) * tile_size;
    maxx = (x + 1.0) * tile_size;
    maxy = y * tile_size;
    double shift = std::pow(2.0,z) * tile_size;
    from_pixels(shift,minx,miny);
    from_pixels(shift,maxx,maxy);
    mapnik::lonlat2merc(&minx,&miny,1);
    mapnik::lonlat2merc(&maxx,&maxy,1);
}


int main(int argc, char** argv)
{
    try
    {

        if (argc < 4)
        {
            std::clog << "usage: vtile-encode /path/to/geometry.geojson z x y [iterations]\n";
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

        std::clog << "z:" << z << " x:" << x << " y:" << y <<  " iterations:" << iterations << std::endl;

        mapnik::datasource_cache::instance().register_datasources(MAPNIK_PLUGINDIR);
        mapnik::parameters params_1;
        params_1["type"] = "geojson";
        params_1["file"] = geojson_file;
        params_1["cache_features"] = "false";
        auto ds_1 = mapnik::datasource_cache::instance().create(params_1);

        // Query
        mapnik::box2d<double> query_ext(std::numeric_limits<double>::min(),
                                        std::numeric_limits<double>::min(),
                                        std::numeric_limits<double>::max(),
                                        std::numeric_limits<double>::max());
        mapnik::query q(query_ext);
        auto json_fs = ds_1->features(q);

        // Create memory datasource from geojson
        mapnik::parameters params;
        params["type"] = "memory";
        auto ds = std::make_shared<mapnik::memory_datasource>(params);

        if (!json_fs)
        {
            // No features in geojson so lets try to process it differently as
            // it might be a partial geojson and we can use from_geojson
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
            mapnik::context_ptr ctx = std::make_shared<mapnik::context_type>();
            mapnik::feature_ptr feature(mapnik::feature_factory::create(ctx,1));
            feature->set_geometry(std::move(geom));
            ds->push(feature);
        }
        else
        {
            mapnik::feature_ptr feature = json_fs->next();
        
            while (feature)
            {
                ds->push(feature);
                feature = json_fs->next();
            }
        }

        // Create tile 
        unsigned tile_size = 256;

        double minx,miny,maxx,maxy;
        xyz(tile_size, x, y, z, minx, miny, maxx, maxy);
        mapnik::box2d<double> bbox(minx,miny,maxx,maxy);

        // Create a fresh map to render into a tile
        mapnik::Map map(tile_size,tile_size,"+init=epsg:3857");
        mapnik::layer lyr("layer","+init=epsg:4326");
        lyr.set_datasource(ds);
        map.add_layer(lyr);
        map.zoom_to_box(bbox);

        // Output buffer
        std::string output_buffer;
        std::size_t expected_size;
        {
            vector_tile::Tile tile;
            mapnik::vector_tile_impl::backend_pbf backend(tile, 16);
            mapnik::request m_req(tile_size,tile_size,bbox);
            renderer_type ren(backend,map,m_req);
            ren.apply();
            tile.SerializeToString(&output_buffer);
            expected_size = output_buffer.size();
        }

        {
            mapnik::progress_timer __stats__(std::clog, std::string("encode tile: ") + geojson_file);
            for (std::size_t i = 0; i < iterations; ++i)
            {
                vector_tile::Tile tile;
                mapnik::vector_tile_impl::backend_pbf backend(tile, 16);
                mapnik::request m_req(tile_size,tile_size,bbox);
                renderer_type ren(backend,map,m_req,1.0,0.0,0.0,0.1,true);
                ren.set_simplify_distance(5.0);
                ren.set_fill_type(mapnik::vector_tile_impl::positive_fill);
                ren.apply();
                std::string buffer;
                tile.SerializeToString(&buffer);
                assert(buffer.size() == expected_size);
            }
        }

        if (output_path != "")
        {
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
