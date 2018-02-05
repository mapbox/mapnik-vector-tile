#include <mapnik/version.hpp>

#if MAPNIK_VERSION >= 300100
#include <mapnik/geometry/correct.hpp>
#include <mapnik/geometry/transform.hpp>
#else
#include <mapnik/geometry_correct.hpp>
#include <mapnik/geometry_transform.hpp>
#endif

#include <mapnik/projection.hpp>
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

int main() 
{
    mapnik::projection merc("+init=epsg:3857",true);
    mapnik::projection merc2("+init=epsg:4326",true);
    mapnik::proj_transform prj_trans(merc,merc); // no-op
    mapnik::proj_transform prj_trans2(merc2,merc); // op
    unsigned tile_size = 256;
    mapnik::box2d<double> z15_extent = mapnik::vector_tile_impl::tile_mercator_bbox(9664,20435,15);
    unsigned path_multiplier = 16;
    mapnik::view_transform tr(tile_size * path_multiplier,
                              tile_size * path_multiplier,
                              z15_extent,0,0);
    std::string geojson_file("./test/data/poly.geojson");
    mapnik::util::file input(geojson_file);
    if (!input.is_open())
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
    unsigned count3 = 0;
    unsigned count4 = 0;
    {
        mapnik::vector_tile_impl::vector_tile_strategy vs(tr);
        mapnik::progress_timer __stats__(std::clog, "boost::geometry::transform");
        for (unsigned i=0;i<10000;++i)
        {
            mapnik::geometry::geometry<std::int64_t> new_geom = mapnik::geometry::transform<std::int64_t>(geom, vs);
            auto const& poly = mapbox::util::get<mapnik::geometry::multi_polygon<std::int64_t>>(new_geom);
            count += poly.size();
        }
    }
    {
        mapnik::vector_tile_impl::vector_tile_strategy_proj vs(prj_trans,tr);
        mapnik::progress_timer __stats__(std::clog, "transform_visitor with reserve with proj no-op");
        mapnik::box2d<double> clip_extent(std::numeric_limits<double>::min(),
                                       std::numeric_limits<double>::min(),
                                       std::numeric_limits<double>::max(),
                                       std::numeric_limits<double>::max());
        mapnik::vector_tile_impl::geom_out_visitor<std::int64_t> out_geom;
        mapnik::vector_tile_impl::transform_visitor<
                    mapnik::vector_tile_impl::vector_tile_strategy_proj,
                    mapnik::vector_tile_impl::geom_out_visitor<std::int64_t>
                                    > transit(vs, clip_extent, out_geom);
        for (unsigned i=0;i<10000;++i)
        {
            mapbox::util::apply_visitor(transit,geom);
            auto const& poly = mapbox::util::get<mapbox::geometry::multi_polygon<std::int64_t>>(*out_geom.geom);
            count2 += poly.size();
        }
        if (count != count2)
        {
            std::clog << "tests did not run as expected!\n";
            return -1;
        }
    }
    {
        mapnik::vector_tile_impl::vector_tile_strategy_proj vs(prj_trans2,tr);
        mapnik::progress_timer __stats__(std::clog, "transform_visitor with reserve with proj op");
        mapnik::box2d<double> clip_extent(std::numeric_limits<double>::min(),
                                       std::numeric_limits<double>::min(),
                                       std::numeric_limits<double>::max(),
                                       std::numeric_limits<double>::max());
        mapnik::vector_tile_impl::geom_out_visitor<std::int64_t> out_geom;
        mapnik::vector_tile_impl::transform_visitor<
                    mapnik::vector_tile_impl::vector_tile_strategy_proj,
                    mapnik::vector_tile_impl::geom_out_visitor<std::int64_t>
                                    > transit(vs, clip_extent, out_geom);
        for (unsigned i=0;i<10000;++i)
        {
            mapbox::util::apply_visitor(transit,geom);        
            auto const& poly = mapbox::util::get<mapbox::geometry::multi_polygon<std::int64_t>>(*out_geom.geom);
            count4 += poly.size();
        }
        if (count != count4)
        {
            std::clog << "tests did not run as expected!\n";
            return -1;
        }
    }
    {
        mapnik::vector_tile_impl::vector_tile_strategy vs(tr);
        mapnik::progress_timer __stats__(std::clog, "transform_visitor with reserve with no proj function call overhead");
        mapnik::box2d<double> clip_extent(std::numeric_limits<double>::min(),
                                       std::numeric_limits<double>::min(),
                                       std::numeric_limits<double>::max(),
                                       std::numeric_limits<double>::max());
        mapnik::vector_tile_impl::geom_out_visitor<std::int64_t> out_geom;
        mapnik::vector_tile_impl::transform_visitor<
                    mapnik::vector_tile_impl::vector_tile_strategy,
                    mapnik::vector_tile_impl::geom_out_visitor<std::int64_t>
                                    > transit(vs, clip_extent, out_geom);
        for (unsigned i=0;i<10000;++i)
        {
            mapnik::util::apply_visitor(transit,geom);
            auto const& poly = mapbox::util::get<mapbox::geometry::multi_polygon<std::int64_t>>(*out_geom.geom);
            count3 += poly.size();
        }
        if (count != count3)
        {
            std::clog << "tests did not run as expected!\n";
            return -1;
        }
    }
    return 0;
}
