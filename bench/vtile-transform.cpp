#include <mapnik/projection.hpp>
#include <mapnik/geometry_transform.hpp>
#include <mapnik/util/file_io.hpp>
#include <mapnik/json/geometry_parser.hpp>
#include <mapnik/timer.hpp>

#include "vector_tile_strategy.hpp"
#include "vector_tile_projection.hpp"

/*

# 10000 times
    no reserve + transform_visitor
     - 2.77

    with reserve
     - 1.69

    boost::geometry::transform
     - 2.3

*/

int main() {
    mapnik::projection merc("+init=epsg:3857",true);
    mapnik::proj_transform prj_trans(merc,merc); // no-op
    unsigned tile_size = 256;
    mapnik::vector_tile_impl::spherical_mercator merc_tiler(tile_size);
    double minx,miny,maxx,maxy;
    merc_tiler.xyz(9664,20435,15,minx,miny,maxx,maxy);
    mapnik::box2d<double> z15_extent(minx,miny,maxx,maxy);
    mapnik::view_transform tr(tile_size,tile_size,z15_extent,0,0);
    std::string geojson_file("./test/data/poly.geojson");
    mapnik::util::file input(geojson_file);
    if (!input.open())
    {
        throw std::runtime_error("failed to open geojson");
    }
    mapnik::geometry::geometry<double> geom;
    std::string json_string(input.data().get(), input.size());
    if (!mapnik::json::from_geojson(json_string, geom))
    {
        throw std::runtime_error("failed to parse geojson");
    }
    mapnik::geometry::correct(geom);

    unsigned count = 0;
    unsigned count2 = 0;
    {
        mapnik::vector_tile_impl::vector_tile_strategy vs(prj_trans, tr, 16);
        mapnik::progress_timer __stats__(std::clog, "boost::geometry::transform");
        for (unsigned i=0;i<10000;++i)
        {
            mapnik::geometry::geometry<std::int64_t> new_geom = mapnik::geometry::transform<std::int64_t>(geom, vs);
            auto const& poly = mapnik::util::get<mapnik::geometry::multi_polygon<std::int64_t>>(new_geom);
            count += poly.size();
        }
    }
    {
        mapnik::vector_tile_impl::vector_tile_strategy vs(prj_trans, tr, 16);
        mapnik::progress_timer __stats__(std::clog, "transform_visitor with reserve");
        mapnik::vector_tile_impl::transform_visitor transit(vs);
        for (unsigned i=0;i<10000;++i)
        {
            mapnik::geometry::geometry<std::int64_t> new_geom = mapnik::util::apply_visitor(transit,geom);        
            auto const& poly = mapnik::util::get<mapnik::geometry::multi_polygon<std::int64_t>>(new_geom);
            count2 += poly.size();
        }
        if (count != count2)
        {
            std::clog << "tests did not run as expected!\n";
            return -1;
        }
    }
    return 0;
}