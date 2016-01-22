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

// mapnik
#include <mapnik/box2d.hpp>
#include <mapnik/well_known_srs.hpp>
#include <mapnik/json/geometry_parser.hpp>
#include <mapnik/memory_datasource.hpp>
#include <mapnik/feature_factory.hpp>
#include <mapnik/map.hpp>
#include <mapnik/layer.hpp>
#include <mapnik/request.hpp>

#include <protozero/pbf_reader.hpp>

#ifndef M_PI
#define M_PI 3.141592653589793238462643
#endif

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
        std::string geojson(argv[1]);
        mapnik::util::file input(geojson);
        if (!input.open())
        {
            std::clog << std::string("failed to open ") + geojson << "\n";
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

        // Create memory datasource from geojson
        mapnik::geometry::geometry<double> geom;
        std::string json_string(input.data().get(), input.size());
        if (!mapnik::json::from_geojson(json_string, geom))
        {
            throw std::runtime_error("failed to parse geojson");
        }
        mapnik::geometry::correct(geom);
        mapnik::parameters params;
        params["type"] = "memory";
        auto ds = std::make_shared<mapnik::memory_datasource>(params);
        mapnik::context_ptr ctx = std::make_shared<mapnik::context_type>();
        ctx->push("name");
        mapnik::feature_ptr feature(mapnik::feature_factory::create(ctx,1));
        feature->set_geometry(std::move(geom));
        ds->push(feature);

        // Create tile 
        unsigned tile_size = 256;
        vector_tile::Tile tile;
        mapnik::vector_tile_impl::backend_pbf backend(tile, 16);;

        double minx,miny,maxx,maxy;
        xyz(tile_size, x, y, z, minx, miny, maxx, maxy);
        mapnik::box2d<double> bbox(minx,miny,maxx,maxy);

        // Create map to render into tile
        mapnik::Map map(tile_size,tile_size,"+init=epsg:3857");
        mapnik::layer lyr("layer","+init=epsg:4326");

        {
            mapnik::progress_timer __stats__(std::clog, std::string("encode tile: ") + geojson);
            for (std::size_t i=0;i<iterations;++i)
            {
                lyr.set_datasource(ds);
                map.add_layer(lyr);
                map.zoom_to_box(bbox);
                mapnik::request m_req(tile_size,tile_size,bbox);
                renderer_type ren(backend,map,m_req);
                ren.apply();

                std::string buffer;
                tile.SerializeToString(&buffer);
            }
        }
    }
    catch (std::exception const& ex)
    {
        std::clog << "error: " << ex.what() << "\n";
        return -1;
    }
    return 0;
}
