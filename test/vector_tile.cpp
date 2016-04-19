#include "catch.hpp"

// test utils
#include "test_utils.hpp"

// mapnik
#include <mapnik/agg_renderer.hpp>
#include <mapnik/datasource_cache.hpp>
#include <mapnik/feature_factory.hpp>
#include <mapnik/geometry.hpp>
#include <mapnik/geometry_is_empty.hpp>
#include <mapnik/geometry_reprojection.hpp>
#include <mapnik/image_util.hpp>
#include <mapnik/load_map.hpp>
#include <mapnik/memory_datasource.hpp>
#include <mapnik/projection.hpp>
#include <mapnik/proj_transform.hpp>
#include <mapnik/util/file_io.hpp>
#include <mapnik/util/fs.hpp>
#include <mapnik/util/geometry_to_geojson.hpp>
#include <mapnik/vertex_adapters.hpp>

// boost
#include <boost/optional/optional_io.hpp>

// mapnik-vector-tile
#include "vector_tile_processor.hpp"
#include "vector_tile_geometry_decoder.hpp"
#include "vector_tile_datasource_pbf.hpp"

// libprotobuf
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include "vector_tile.pb.h"
#pragma GCC diagnostic pop

// std
#include <fstream>

TEST_CASE("vector tile from simplified geojson")
{
    unsigned tile_size = 256 * 1000;
    mapnik::Map map(256,256,"+init=epsg:3857");
    mapnik::layer lyr("layer","+init=epsg:4326");
    std::shared_ptr<mapnik::memory_datasource> ds = testing::build_geojson_ds("./test/data/poly.geojson");
    ds->set_envelope(mapnik::box2d<double>(160.147311,11.047284,160.662858,11.423830));
    lyr.set_datasource(ds);
    map.add_layer(lyr);
    mapnik::vector_tile_impl::processor ren(map);
    mapnik::vector_tile_impl::tile out_tile = ren.create_tile(0,0,0,tile_size);
    CHECK(out_tile.is_painted() == true);
    CHECK(out_tile.is_empty() == false);
    
    vector_tile::Tile tile;
    tile.ParseFromString(out_tile.get_buffer());
    REQUIRE(1 == tile.layers_size());
    vector_tile::Tile_Layer const& layer = tile.layers(0);
    CHECK(std::string("layer") == layer.name());
    REQUIRE(1 == layer.features_size());
    vector_tile::Tile_Feature const& f = layer.features(0);
    unsigned z = 0;
    unsigned x = 0;
    unsigned y = 0;
    double resolution = mapnik::EARTH_CIRCUMFERENCE/(1 << z);
    double tile_x = -0.5 * mapnik::EARTH_CIRCUMFERENCE + x * resolution;
    double tile_y =  0.5 * mapnik::EARTH_CIRCUMFERENCE - y * resolution;
    double scale = static_cast<double>(layer.extent())/resolution;
    protozero::pbf_reader layer_reader;
    out_tile.layer_reader(0, layer_reader);
    REQUIRE(layer_reader.next(mapnik::vector_tile_impl::Layer_Encoding::FEATURES));
    protozero::pbf_reader feature_reader = layer_reader.get_message();
    int32_t geometry_type = mapnik::vector_tile_impl::Geometry_Type::UNKNOWN; 
    std::pair<protozero::pbf_reader::const_uint32_iterator, protozero::pbf_reader::const_uint32_iterator> geom_itr;
    while (feature_reader.next())
    {
        if (feature_reader.tag() == mapnik::vector_tile_impl::Feature_Encoding::GEOMETRY)
        {
            geom_itr = feature_reader.get_packed_uint32();
        }
        else if (feature_reader.tag() == mapnik::vector_tile_impl::Feature_Encoding::TYPE)
        {
            geometry_type = feature_reader.get_enum();
        }
        else
        {
            feature_reader.skip();
        }
    }
    mapnik::vector_tile_impl::GeometryPBF geoms(geom_itr);
    auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, geometry_type, 2, tile_x, tile_y, scale, -1.0 * scale);

    unsigned int n_err = 0;
    mapnik::projection wgs84("+init=epsg:4326",true);
    mapnik::projection merc("+init=epsg:3857",true);
    mapnik::proj_transform prj_trans(merc,wgs84);
    mapnik::geometry::geometry<double> projected_geom = mapnik::geometry::reproject_copy(geom,prj_trans,n_err);
    CHECK( n_err == 0 );
    std::string geojson_string;
    CHECK( mapnik::util::to_geojson(geojson_string,projected_geom) );
    std::string geojson_file = "./test/data/simplified_geometry.geojson";
    mapnik::util::file input(geojson_file);
    if (input.is_open())
    {
        std::string json_string(input.data().get(), input.size());
        CHECK (geojson_string == json_string);
    }
    if (!mapnik::util::exists(geojson_file) || (std::getenv("UPDATE") != nullptr))
    {
        std::ofstream out(geojson_file);
        out << geojson_string;
    }
}

TEST_CASE("vector tile transform -- should not throw on coords outside merc range")
{
    unsigned tile_size = 256 * 64;
    mapnik::Map map(256,256,"+init=epsg:3857");
    // Note: 4269 is key. 4326 will trigger custom mapnik reprojection code
    // that does not hit proj4 and clamps values
    mapnik::layer lyr("layer","+init=epsg:4269");
    mapnik::parameters params;
    params["type"] = "shape";
    params["file"] = "./test/data/poly-lat-invalid-4269.shp";
    std::shared_ptr<mapnik::datasource> ds =
        mapnik::datasource_cache::instance().create(params);
    lyr.set_datasource(ds);
    map.add_layer(lyr);
    mapnik::vector_tile_impl::processor ren(map);
    mapnik::vector_tile_impl::tile out_tile = ren.create_tile(0,0,0,tile_size);
    CHECK(out_tile.is_painted() == true);
    CHECK(out_tile.is_empty() == false);
    
    vector_tile::Tile tile;
    tile.ParseFromString(out_tile.get_buffer());

    // serialize to message
    std::string buffer;
    CHECK(tile.SerializeToString(&buffer));
    CHECK(70 == buffer.size());
    // now create new objects
    mapnik::Map map2(256,256,"+init=epsg:3857");
    vector_tile::Tile tile2;
    CHECK(tile2.ParseFromString(buffer));
    CHECK(1 == tile2.layers_size());
    vector_tile::Tile_Layer const& layer2 = tile2.layers(0);
    CHECK(std::string("layer") == layer2.name());
    CHECK(1 == layer2.features_size());
    CHECK(1 == layer2.features(0).id());
    CHECK(3 == layer2.features(0).type());

    mapnik::layer lyr2("layer",map.srs());

    protozero::pbf_reader layer_reader;
    out_tile.layer_reader(0, layer_reader);
    std::shared_ptr<mapnik::vector_tile_impl::tile_datasource_pbf> ds2 = std::make_shared<
                                    mapnik::vector_tile_impl::tile_datasource_pbf>(
                                        layer_reader,0,0,0);
    //ds2->set_envelope(bbox);
    CHECK( ds2->type() == mapnik::datasource::Vector );
    CHECK( ds2->get_geometry_type() == mapnik::datasource_geometry_t::Collection );
    mapnik::layer_descriptor lay_desc = ds2->get_descriptor();
    std::vector<std::string> expected_names;
    expected_names.push_back("FID");
    std::vector<std::string> names;
    for (auto const& desc : lay_desc.get_descriptors())
    {
        names.push_back(desc.get_name());
    }
    CHECK(names == expected_names);
    lyr2.set_datasource(ds2);
    lyr2.add_style("style");
    map2.add_layer(lyr2);
    mapnik::load_map(map2,"test/data/polygon-style.xml");
    //std::clog << mapnik::save_map_to_string(map2) << "\n";
    map2.zoom_all();
    mapnik::image_rgba8 im(map2.width(),map2.height());
    mapnik::agg_renderer<mapnik::image_rgba8> ren2(map2,im);
    ren2.apply();
    if (!mapnik::util::exists("test/fixtures/transform-expected-1.png"))
    {
        mapnik::save_to_file(im,"test/fixtures/transform-expected-1.png","png32");
    }
    unsigned diff = testing::compare_images(im,"test/fixtures/transform-expected-1.png");
    CHECK(0 == diff);
    if (diff > 0)
    {
        mapnik::save_to_file(im,"test/fixtures/transform-actual-1.png","png32");
    }
}

TEST_CASE("vector tile transform2 -- should not throw reprojected data from local NZ projection")
{
    unsigned tile_size = 256 * 64;
    mapnik::box2d<double> bbox(-20037508.342789,-20037508.342789,20037508.342789,20037508.342789);
    mapnik::Map map(256,256,"+init=epsg:3857");
    // Note: 4269 is key. 4326 will trigger custom mapnik reprojection code
    // that does not hit proj4 and clamps values
    mapnik::layer lyr("layer","+proj=nzmg +lat_0=-41 +lon_0=173 +x_0=2510000 +y_0=6023150 +ellps=intl +units=m +no_defs");
    mapnik::parameters params;
    params["type"] = "shape";
    params["file"] = "./test/data/NZ_Coastline_NZMG.shp";
    std::shared_ptr<mapnik::datasource> ds =
        mapnik::datasource_cache::instance().create(params);
    lyr.set_datasource(ds);
    map.add_layer(lyr);
    
    mapnik::vector_tile_impl::processor ren(map);
    mapnik::vector_tile_impl::tile out_tile = ren.create_tile(0,0,0,tile_size);
    CHECK(out_tile.is_painted() == true);
    CHECK(out_tile.is_empty() == false);
    
    vector_tile::Tile tile;
    tile.ParseFromString(out_tile.get_buffer());

    // serialize to message
    std::string buffer;
    CHECK(tile.SerializeToString(&buffer));
    CHECK(231 == buffer.size());
    // now create new objects
    mapnik::Map map2(256,256,"+init=epsg:3857");
    vector_tile::Tile tile2;
    CHECK(tile2.ParseFromString(buffer));
    CHECK(1 == tile2.layers_size());
    vector_tile::Tile_Layer const& layer2 = tile2.layers(0);
    CHECK(std::string("layer") == layer2.name());
    CHECK(2 == layer2.features_size());

    mapnik::layer lyr2("layer",map.srs());
    protozero::pbf_reader layer_reader;
    out_tile.layer_reader(0, layer_reader);
    std::shared_ptr<mapnik::vector_tile_impl::tile_datasource_pbf> ds2 = std::make_shared<
                                    mapnik::vector_tile_impl::tile_datasource_pbf>(
                                        layer_reader,0,0,0);
    ds2->set_envelope(bbox);
    CHECK( ds2->type() == mapnik::datasource::Vector );
    CHECK( ds2->get_geometry_type() == mapnik::datasource_geometry_t::Collection );
    mapnik::layer_descriptor lay_desc = ds2->get_descriptor();
    std::vector<std::string> expected_names;
    expected_names.push_back("featurecla");
    expected_names.push_back("scalerank");
    std::vector<std::string> names;
    for (auto const& desc : lay_desc.get_descriptors())
    {
        names.push_back(desc.get_name());
    }
    CHECK(names == expected_names);
    lyr2.set_datasource(ds2);
    lyr2.add_style("style");
    map2.add_layer(lyr2);
    mapnik::load_map(map2,"test/data/polygon-style.xml");
    //std::clog << mapnik::save_map_to_string(map2) << "\n";
    map2.zoom_to_box(bbox);
    mapnik::image_rgba8 im(map2.width(),map2.height());
    mapnik::agg_renderer<mapnik::image_rgba8> ren2(map2,im);
    ren2.apply();
    if (!mapnik::util::exists("test/fixtures/transform-expected-2.png")) {
        mapnik::save_to_file(im,"test/fixtures/transform-expected-2.png","png32");
    }
    unsigned diff = testing::compare_images(im,"test/fixtures/transform-expected-2.png");
    CHECK(0 == diff);
    if (diff > 0) {
        mapnik::save_to_file(im,"test/fixtures/transform-actual-2.png","png32");
    }
}
