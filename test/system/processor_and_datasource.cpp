#include "catch.hpp"

// mapnik-vector-tile
#include "vector_tile_datasource_pbf.hpp"
#include "vector_tile_processor.hpp"

// mapnik
#include <mapnik/agg_renderer.hpp>
#include <mapnik/feature_factory.hpp>
#include <mapnik/load_map.hpp>
#include <mapnik/image_util.hpp>
#include <mapnik/util/fs.hpp>

// test utils
#include "test_utils.hpp"

// libprotobuf
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include "vector_tile.pb.h"
#pragma GCC diagnostic pop

// boost
#include <boost/optional/optional_io.hpp>

// std
#include <set>

TEST_CASE("vector tile output -- simple two points")
{
    // This test should create vector tile with two points

    // Build Map
    mapnik::Map map(256, 256, "+init=epsg:3857");
    mapnik::layer lyr("layer",map.srs());
    lyr.set_datasource(testing::build_ds(0,0,true));
    map.add_layer(lyr);
    
    // Create processor
    mapnik::vector_tile_impl::processor ren(map);
    
    // Request Tile
    mapnik::vector_tile_impl::merc_tile out_tile = ren.create_tile(0,0,0);
    CHECK(out_tile.is_painted() == true);
    CHECK(out_tile.is_empty() == false);

    // Now check that the tile is correct.
    vector_tile::Tile tile;
    tile.ParseFromString(out_tile.get_buffer());
    REQUIRE(1 == tile.layers_size());
    vector_tile::Tile_Layer const& layer = tile.layers(0);
    CHECK(std::string("layer") == layer.name());
    REQUIRE(2 == layer.features_size());
    vector_tile::Tile_Feature const& f = layer.features(0);
    CHECK(static_cast<mapnik::value_integer>(1) == static_cast<mapnik::value_integer>(f.id()));
    REQUIRE(3 == f.geometry_size());
    CHECK(9 == f.geometry(0));
    CHECK(4096 == f.geometry(1));
    CHECK(4096 == f.geometry(2));
    CHECK(190 == tile.ByteSize());
    std::string buffer;
    CHECK(tile.SerializeToString(&buffer));
    CHECK(190 == buffer.size());
}

TEST_CASE("processor -- can deal with (optional) variables")
{
    // Build Map
    mapnik::Map map(256, 256, "+init=epsg:3857");
    mapnik::layer lyr("layer",map.srs());
    lyr.set_datasource(testing::build_ds(0,0,true));
    map.add_layer(lyr);

    // Create processor
    const mapnik::attributes vars { {"zoom_level", 20} };
    mapnik::vector_tile_impl::processor ren(map, vars);

    // Request Tile
    mapnik::vector_tile_impl::merc_tile out_tile = ren.create_tile(0,0,0);
    CHECK(out_tile.is_painted() == true);
    CHECK(out_tile.is_empty() == false);

    // Now check that the tile is correct.
    vector_tile::Tile tile;
    tile.ParseFromString(out_tile.get_buffer());
    REQUIRE(1 == tile.layers_size());
    vector_tile::Tile_Layer const& layer = tile.layers(0);
    CHECK(std::string("layer") == layer.name());
    REQUIRE(2 == layer.features_size());
    vector_tile::Tile_Feature const& f = layer.features(0);
    CHECK(static_cast<mapnik::value_integer>(1) == static_cast<mapnik::value_integer>(f.id()));
    REQUIRE(3 == f.geometry_size());
    CHECK(9 == f.geometry(0));
    CHECK(4096 == f.geometry(1));
    CHECK(4096 == f.geometry(2));
    CHECK(190 == tile.ByteSize());
    std::string buffer;
    CHECK(tile.SerializeToString(&buffer));
    CHECK(190 == buffer.size());
}

TEST_CASE("vector tile output -- empty tile")
{
    // test adding empty layers should result in empty tile
    mapnik::Map map(256,256,"+init=epsg:3857");
    map.add_layer(mapnik::layer("layer",map.srs()));
    mapnik::vector_tile_impl::processor ren(map);
    mapnik::vector_tile_impl::tile out_tile = ren.create_tile(0,0,0);
    CHECK(out_tile.is_painted() == false);
    CHECK(out_tile.is_empty() == true);
    vector_tile::Tile tile;
    tile.ParseFromString(out_tile.get_buffer());
    CHECK(0 == tile.layers_size());
    std::string buffer;
    out_tile.serialize_to_string(buffer);
    CHECK(buffer.empty());
}

TEST_CASE("vector tile output -- layers outside extent")
{
    // adding layers with geometries outside rendering extent should not add layer
    unsigned tile_size = 4096;
    // Create map
    mapnik::Map map(256,256,"+init=epsg:3857");
    mapnik::layer lyr("layer",map.srs());
    mapnik::context_ptr ctx = std::make_shared<mapnik::context_type>();
    mapnik::feature_ptr feature(mapnik::feature_factory::create(ctx,1));
    mapnik::geometry::line_string<double> g;
    g.add_coord(-10,-10);
    g.add_coord(-11,-11);
    feature->set_geometry(std::move(g));
    mapnik::parameters params;
    params["type"] = "memory";
    std::shared_ptr<mapnik::memory_datasource> ds = std::make_shared<mapnik::memory_datasource>(params);
    ds->push(feature);
    lyr.set_datasource(ds);
    map.add_layer(lyr);

    mapnik::box2d<double> custom_bbox(0,0,10,10);
    
    // Build processor and create tile
    mapnik::vector_tile_impl::processor ren(map);
    mapnik::vector_tile_impl::tile out_tile = ren.create_tile(custom_bbox, tile_size);
    CHECK(out_tile.is_painted() == false);
    CHECK(out_tile.is_empty() == true);
    vector_tile::Tile tile;
    tile.ParseFromString(out_tile.get_buffer());
    CHECK(0 == tile.layers_size());
    std::string buffer;
    out_tile.serialize_to_string(buffer);
    CHECK(buffer.empty());
}

TEST_CASE("vector tile output is empty -- degenerate geometries")
{
    // adding layers with degenerate geometries should not add layer
    unsigned tile_size = 4096;
    mapnik::box2d<double> bbox(-20037508.342789,-20037508.342789,20037508.342789,20037508.342789);
    mapnik::Map map(256,256,"+init=epsg:3857");
    mapnik::layer lyr("layer",map.srs());
    // create a datasource with a feature outside the map
    std::shared_ptr<mapnik::memory_datasource> ds = testing::build_ds(bbox.minx()-1,bbox.miny()-1);
    // but fake the overall envelope to ensure the layer is still processed
    // and then removed given no intersecting features will be added
    ds->set_envelope(bbox);
    lyr.set_datasource(ds);
    map.add_layer(lyr);
    
    // Build processor and create tile
    mapnik::vector_tile_impl::processor ren(map);
    
    // Request Tile
    mapnik::vector_tile_impl::tile out_tile = ren.create_tile(bbox, tile_size);
    
    // Check output
    CHECK(out_tile.is_painted() == false);
    CHECK(out_tile.is_empty() == true);
    vector_tile::Tile tile;
    tile.ParseFromString(out_tile.get_buffer());
    CHECK(0 == tile.layers_size());
    std::string buffer;
    out_tile.serialize_to_string(buffer);
    CHECK(buffer.empty());
}

TEST_CASE("vector tile render simple point")
{
    // should be able to parse message and render point
    unsigned tile_size = 4096;
    mapnik::Map map(256, 256, "+init=epsg:3857");
    mapnik::layer lyr("layer", map.srs());
    lyr.set_datasource(testing::build_ds(0,0));
    map.add_layer(lyr);
    
    // create processor
    mapnik::vector_tile_impl::processor ren(map);
    mapnik::vector_tile_impl::tile out_tile = ren.create_tile(0,0,0);
    // serialize to message
    std::string buffer;
    out_tile.serialize_to_string(buffer);
    CHECK(out_tile.is_painted() == true);
    CHECK(out_tile.is_empty() == false);
    CHECK(147 == buffer.size());

    // create a new vector tile from the buffer
    vector_tile::Tile tile2;
    CHECK(tile2.ParseFromString(buffer));

    // Validate the new tile.
    CHECK(1 == tile2.layers_size());
    vector_tile::Tile_Layer const& layer2 = tile2.layers(0);
    CHECK(std::string("layer") == layer2.name());
    CHECK(1 == layer2.features_size());
    
    // Create another map
    mapnik::Map map2(256, 256, "+init=epsg:3857");
    mapnik::layer lyr2("layer",map.srs());
    
    // Create datasource from tile.
    protozero::pbf_reader layer_reader;
    out_tile.layer_reader(0, layer_reader);
    std::shared_ptr<mapnik::vector_tile_impl::tile_datasource_pbf> ds = std::make_shared<
                                    mapnik::vector_tile_impl::tile_datasource_pbf>(
                                        layer_reader,0,0,0);
    CHECK( ds->type() == mapnik::datasource::Vector );
    CHECK( ds->get_geometry_type() == mapnik::datasource_geometry_t::Collection );
    
    // Check that all the names are in the list
    mapnik::layer_descriptor lay_desc = ds->get_descriptor();
    std::set<std::string> expected_names;
    expected_names.insert("bool");
    expected_names.insert("boolf");
    expected_names.insert("double");
    expected_names.insert("float");
    expected_names.insert("int");
    expected_names.insert("name");
    expected_names.insert("uint");
    std::size_t desc_count = 0;
    for (auto const& desc : lay_desc.get_descriptors())
    {
        ++desc_count;
        CHECK(expected_names.count(desc.get_name()) == 1);
    }
    CHECK(desc_count == expected_names.size());
    
    // Add datasource to layer and map
    lyr2.set_datasource(ds);
    lyr2.add_style("style");
    map2.add_layer(lyr2);
    // Load map style
    mapnik::load_map(map2,"test/data/style.xml");
    map2.zoom_all();
    mapnik::image_rgba8 im(map2.width(),map2.height());
    mapnik::agg_renderer<mapnik::image_rgba8> ren2(map2,im);
    ren2.apply();

    if (!mapnik::util::exists("test/fixtures/expected-1.png"))
    {
        mapnik::save_to_file(im,"test/fixtures/expected-1.png","png32");
    }
    unsigned diff = testing::compare_images(im,"test/fixtures/expected-1.png");
    CHECK(0 == diff);
    if (diff > 0)
    {
        mapnik::save_to_file(im,"test/fixtures/actual-1.png","png32");
    }
}

TEST_CASE("vector tile datasource -- should filter features outside extent")
{
    mapnik::Map map(256,256,"+init=epsg:3857");
    mapnik::layer lyr("layer",map.srs());
    lyr.set_datasource(testing::build_ds(0,0));
    map.add_layer(lyr);
    
    mapnik::vector_tile_impl::processor ren(map);
    mapnik::vector_tile_impl::tile out_tile = ren.create_tile(0,0,0);

    // serialize to message
    std::string buffer;
    out_tile.serialize_to_string(buffer);
    CHECK(out_tile.is_painted() == true);
    CHECK(out_tile.is_empty() == false);
    
    // check that vector tile contains proper information
    vector_tile::Tile tile;
    tile.ParseFromString(out_tile.get_buffer());
    REQUIRE(1 == tile.layers_size());
    vector_tile::Tile_Layer const& layer = tile.layers(0);
    CHECK(std::string("layer") == layer.name());
    REQUIRE(1 == layer.features_size());
    vector_tile::Tile_Feature const& f = layer.features(0);
    CHECK(static_cast<mapnik::value_integer>(1) == static_cast<mapnik::value_integer>(f.id()));
    CHECK(3 == f.geometry_size());
    CHECK(9 == f.geometry(0));
    CHECK(4096 == f.geometry(1));
    CHECK(4096 == f.geometry(2));
    
    // now actually start the meat of the test
    // create a datasource from the vector tile
    protozero::pbf_reader layer_reader;
    out_tile.layer_reader(0, layer_reader);
    mapnik::vector_tile_impl::tile_datasource_pbf ds(layer_reader,0,0,0);

    // ensure we can query single feature
    mapnik::box2d<double> bbox(-20037508.342789,-20037508.342789,20037508.342789,20037508.342789);
    mapnik::featureset_ptr fs;
    fs = ds.features(mapnik::query(bbox));
    mapnik::feature_ptr feat = fs->next();
    // Check that feature is not empty.
    REQUIRE(feat != mapnik::feature_ptr());
    CHECK(feat->size() == 0);
    // Check that this was the only feature so next should be empty
    CHECK(fs->next() == mapnik::feature_ptr());

    // Now query for another feature.
    mapnik::query qq = mapnik::query(mapnik::box2d<double>(-1,-1,1,1));
    qq.add_property_name("name");
    fs = ds.features(qq);
    feat = fs->next();
    // Check that feature is not empty
    CHECK(feat != mapnik::feature_ptr());
    CHECK(feat->size() == 1);
    CHECK(feat->get("name") == mapnik::value_unicode_string("null island"));
    CHECK(fs->next() == mapnik::feature_ptr());

    // now check that datasource api throws out feature which is outside extent
    fs = ds.features(mapnik::query(mapnik::box2d<double>(-10,-10,-10,-10)));
    CHECK(fs->next() == mapnik::feature_ptr());

    // ensure same behavior for feature_at_point
    fs = ds.features_at_point(mapnik::coord2d(0.0,0.0),0.0001);
    CHECK(fs->next() != mapnik::feature_ptr());

    fs = ds.features_at_point(mapnik::coord2d(1.0,1.0),1.0001);
    CHECK(fs->next() != mapnik::feature_ptr());

    fs = ds.features_at_point(mapnik::coord2d(-10,-10),0);
    CHECK(fs->next() == mapnik::feature_ptr());

    // finally, make sure attributes are also filtered
    mapnik::feature_ptr f_ptr;
    fs = ds.features(mapnik::query(bbox));
    f_ptr = fs->next();
    CHECK(f_ptr != mapnik::feature_ptr());
    // no attributes
    CHECK(f_ptr->context()->size() == 0);

    mapnik::query q(bbox);
    q.add_property_name("name");
    fs = ds.features(q);
    f_ptr = fs->next();
    CHECK(f_ptr != mapnik::feature_ptr());
    // one attribute
    CHECK(f_ptr->context()->size() == 1);
}
