#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <mapnik/util/fs.hpp>
#include <mapnik/projection.hpp>
#include <mapnik/geometry_transform.hpp>
#include <mapnik/util/geometry_to_geojson.hpp>
#include <mapnik/geometry_correct.hpp>
#include <mapnik/util/file_io.hpp>
#include <mapnik/save_map.hpp>
#include <mapnik/geometry_reprojection.hpp>
#include <mapnik/geometry_strategy.hpp>
#include <mapnik/proj_strategy.hpp>
#include <mapnik/view_strategy.hpp>
#include <mapnik/geometry_is_valid.hpp>
#include <mapnik/geometry_is_simple.hpp>

#include "catch.hpp"
#include "encoding_util.hpp"
#include "test_utils.hpp"
#include "vector_tile_strategy.hpp"
#include "vector_tile_processor.hpp"
#include "vector_tile_backend_pbf.hpp"
#include "vector_tile_projection.hpp"
#include "vector_tile_geometry_decoder.hpp"
#include <boost/filesystem/operations.hpp>

void clip_geometry(std::string const& file,
                   mapnik::box2d<double> const& bbox,
                   int simplify_distance,
                   bool strictly_simple,
                   mapnik::vector_tile_impl::polygon_fill_type fill_type)
{
    typedef mapnik::vector_tile_impl::backend_pbf backend_type;
    typedef mapnik::vector_tile_impl::processor<backend_type> renderer_type;
    typedef vector_tile::Tile tile_type;
    std::string geojson_string;
    std::shared_ptr<mapnik::memory_datasource> ds = testing::build_geojson_ds(file);
    tile_type tile;
    backend_type backend(tile,1000);

    mapnik::Map map(256,256,"+init=epsg:4326");
    mapnik::layer lyr("layer","+init=epsg:4326");
    lyr.set_datasource(ds);
    map.add_layer(lyr);
    map.zoom_to_box(bbox);
    mapnik::request m_req(map.width(),map.height(),map.get_current_extent());
    renderer_type ren(
        backend,
        map,
        m_req,
        1.0,
        0,
        0,
        0.1,
        strictly_simple
    );
    ren.set_simplify_distance(simplify_distance);
    ren.set_fill_type(fill_type);
    ren.apply();
    std::string buffer;
    tile.SerializeToString(&buffer);
    if (buffer.size() > 0 && tile.layers_size() > 0 && tile.layers_size() != false)
    {
        vector_tile::Tile_Layer const& layer = tile.layers(0);
        if (layer.features_size() != false)
        {
            vector_tile::Tile_Feature const& f = layer.features(0);
            mapnik::vector_tile_impl::Geometry<double> geoms(f,0.0,0.0,32.0,32.0);
            mapnik::geometry::geometry<double> geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms,f.type());
            mapnik::geometry::scale_strategy ss(1.0/32.0);
            mapnik::view_transform vt(256, 256, bbox);
            mapnik::unview_strategy uvs(vt);
            using sg_type = strategy_group_first<mapnik::geometry::scale_strategy, mapnik::unview_strategy >;
            sg_type sg(ss, uvs);
            mapnik::geometry::geometry<double> geom4326 = transform<double>(geom, sg);
            std::string reason;
            std::string is_valid = "false";
            std::string is_simple = "false";
            if (mapnik::geometry::is_valid(geom4326, reason))
                is_valid = "true";
            if (mapnik::geometry::is_simple(geom4326))
                is_simple = "true";

            unsigned int n_err = 0;
            mapnik::util::to_geojson(geojson_string,geom4326);

            geojson_string = geojson_string.substr(0, geojson_string.size()-1);
            geojson_string += ",\"properties\":{\"is_valid\":"+is_valid+", \"is_simple\":"+is_simple+", \"message\":\""+reason+"\"}}";
        }
        else
        {
            geojson_string = "{\"type\": \"Feature\", \"coordinates\":null, \"properties\":{\"message\":\"Tile layer had no features\"}}";
        }
    }
    else
    {
        geojson_string = "{\"type\": \"Feature\", \"coordinates\":null, \"properties\":{\"message\":\"Tile had no layers\"}}";
    }

    std::string fixture_name = mapnik::util::basename(file);
    fixture_name = fixture_name.substr(0, fixture_name.size()-5);
    if (!mapnik::util::exists("./test/geometry-test-data/output"))
    {
        boost::filesystem::create_directory(("./test/geometry-test-data/output"));
    }
    if (!mapnik::util::exists("./test/geometry-test-data/output/"+fixture_name))
    {
        boost::filesystem::create_directory(("./test/geometry-test-data/output/"+fixture_name));
    }

    std::stringstream file_stream;
    file_stream << "./test/geometry-test-data/output/" << fixture_name << "/"
        << bbox.minx() << ","
        << bbox.miny() << ","
        << bbox.maxx()<< ","
        << bbox.maxy() << ","
        << "simplify_distance=" << simplify_distance << ","
        << "strictly_simple=" << strictly_simple << ","
        << "fill_type=" << fill_type << ".geojson";
    std::string file_path = file_stream.str();
    if (!mapnik::util::exists(file_path) || (std::getenv("UPDATE") != nullptr))
    {
        std::ofstream out(file_path);
        out << geojson_string;
    }
    else
    {
        mapnik::util::file input(file_path);
        if (!input.open())
        {
            throw std::runtime_error("failed to open geojson");
        }
        mapnik::geometry::geometry<double> geom;
        std::string expected_string(input.data().get(), input.size());
        REQUIRE(expected_string == geojson_string);
    }
}

mapnik::box2d<double> middle_fifty(mapnik::box2d<double> const& bbox)
{
    double width = std::fabs(bbox.maxx() - bbox.minx());
    double height = std::fabs(bbox.maxy() - bbox.miny());
    mapnik::box2d<double> new_bbox(
        bbox.minx() + width*0.25,
        bbox.miny() + height*0.25,
        bbox.maxx() - width*0.25,
        bbox.maxy() - height*0.25
    );
    return new_bbox;
}

mapnik::box2d<double> top_left(mapnik::box2d<double> const& bbox)
{
    double width = std::fabs(bbox.maxx() - bbox.minx());
    double height = std::fabs(bbox.maxy() - bbox.miny());
    mapnik::box2d<double> new_bbox(
        bbox.minx(),
        bbox.miny(),
        bbox.maxx() - width*0.5,
        bbox.maxy() - height*0.5
    );
    return new_bbox;
}

mapnik::box2d<double> top_right(mapnik::box2d<double> const& bbox)
{
    double width = std::fabs(bbox.maxx() - bbox.minx());
    double height = std::fabs(bbox.maxy() - bbox.miny());
    mapnik::box2d<double> new_bbox(
        bbox.minx() + width*0.5,
        bbox.miny(),
        bbox.maxx(),
        bbox.maxy() - height*0.5
    );
    return new_bbox;
}

mapnik::box2d<double> bottom_left(mapnik::box2d<double> const& bbox)
{
    double width = std::fabs(bbox.maxx() - bbox.minx());
    double height = std::fabs(bbox.maxy() - bbox.miny());
    mapnik::box2d<double> new_bbox(
        bbox.minx(),
        bbox.miny() + height*0.5,
        bbox.maxx() + width*0.5,
        bbox.maxy()
    );
    return new_bbox;
}

mapnik::box2d<double> bottom_right(mapnik::box2d<double> const& bbox)
{
    double width = std::fabs(bbox.maxx() - bbox.minx());
    double height = std::fabs(bbox.maxy() - bbox.miny());
    mapnik::box2d<double> new_bbox(
        bbox.minx() + width*0.5,
        bbox.miny() + height*0.5,
        bbox.maxx(),
        bbox.maxy()
    );
    return new_bbox;
}

mapnik::box2d<double> zoomed_out(mapnik::box2d<double> const& bbox)
{
    double width = std::fabs(bbox.maxx() - bbox.minx());
    double height = std::fabs(bbox.maxy() - bbox.miny());
    mapnik::box2d<double> new_bbox(
        bbox.minx() - width,
        bbox.miny() - height,
        bbox.maxx() + width,
        bbox.maxy() + height
    );
    return new_bbox;
}

TEST_CASE( "geometries", "should reproject, clip, and simplify")
{

    std::vector<std::string> geometries = mapnik::util::list_directory("./test/geometry-test-data/input");
    std::vector<std::string> benchmarks = mapnik::util::list_directory("./test/geometry-test-data/benchmark");
    if (std::getenv("BENCHMARK") != nullptr)
    {
        geometries.insert(geometries.end(), benchmarks.begin(), benchmarks.end());
    }
    for (std::string const& file: geometries)
    {
        std::shared_ptr<mapnik::memory_datasource> ds = testing::build_geojson_ds(file);
        mapnik::box2d<double> bbox = ds->envelope();
        for (int simplification_distance : std::vector<int>({0, 4, 8}))
        {
            for (bool strictly_simple : std::vector<bool>({false, true}))
            {
                std::vector<mapnik::vector_tile_impl::polygon_fill_type> types;
                types.emplace_back(mapnik::vector_tile_impl::even_odd_fill); 
                types.emplace_back(mapnik::vector_tile_impl::non_zero_fill);
                //types.emplace_back(mapnik::vector_tile_impl::positive_fill); 
                //types.emplace_back(mapnik::vector_tile_impl::negative_fill);
                for (auto type : types)
                {
                    clip_geometry(file, bbox, simplification_distance, strictly_simple, type);
                    clip_geometry(file, middle_fifty(bbox), simplification_distance, strictly_simple, type);
                    clip_geometry(file, top_left(bbox), simplification_distance, strictly_simple, type);
                    clip_geometry(file, top_right(bbox), simplification_distance, strictly_simple, type);
                    clip_geometry(file, bottom_left(bbox), simplification_distance, strictly_simple, type);
                    clip_geometry(file, bottom_right(bbox), simplification_distance, strictly_simple, type);
                    clip_geometry(file, zoomed_out(bbox), simplification_distance, strictly_simple, type);
                }
            }
        }
    }
}
