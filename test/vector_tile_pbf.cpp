#include "catch.hpp"

// test utils
#include "test_utils.hpp"

// mapnik
#include <mapnik/memory_datasource.hpp>
#include <mapnik/util/fs.hpp>
#include <mapnik/agg_renderer.hpp>
#include <mapnik/feature_factory.hpp>
#include <mapnik/load_map.hpp>
#include <mapnik/image_util.hpp>
#include <mapnik/vertex_adapters.hpp>
#include <mapnik/projection.hpp>
#include <mapnik/proj_transform.hpp>
#include <mapnik/util/file_io.hpp>
#include <mapnik/util/geometry_to_geojson.hpp>
#include <mapnik/version.hpp>
#if MAPNIK_VERSION >= 300100
#include <mapnik/geometry/is_empty.hpp>
#include <mapnik/geometry/reprojection.hpp>
#include <mapnik/geometry/transform.hpp>
#include <mapnik/geometry/strategy.hpp>
#else
#include <mapnik/geometry_is_empty.hpp>
#include <mapnik/geometry_reprojection.hpp>
#include <mapnik/geometry_transform.hpp>
#include <mapnik/geometry_strategy.hpp>
#endif
#include <mapnik/proj_strategy.hpp>
#include <mapnik/geometry.hpp>
#include <mapnik/datasource_cache.hpp>

// mapbox
#include <mapbox/geometry/geometry.hpp>

// boost
#include <boost/optional/optional_io.hpp>

// mapnik-vector-tile
#include "vector_tile_compression.hpp"
#include "vector_tile_processor.hpp"
#include "vector_tile_projection.hpp"
#include "vector_tile_geometry_decoder.hpp"
#include "vector_tile_geometry_encoder_pbf.hpp"
#include "vector_tile_datasource_pbf.hpp"
#include "vector_tile_layer.hpp"

//protozero
#include "protozero/pbf_reader.hpp"

//std
#include <string>
#include <fstream>
#include <streambuf>

// libprotobuf
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include "vector_tile.pb.h"
#pragma GCC diagnostic pop

TEST_CASE("pbf vector tile input")
{
    unsigned tile_size = 4096;
    mapnik::Map map(256,256,"+init=epsg:3857");
    mapnik::layer lyr("layer",map.srs());
    lyr.set_datasource(testing::build_ds(0,0));
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
    CHECK(147 == buffer.size());
    // now create new objects
    mapnik::Map map2(256,256,"+init=epsg:3857");
    vector_tile::Tile tile2;
    CHECK(tile2.ParseFromString(buffer));
    CHECK(1 == tile2.layers_size());
    vector_tile::Tile_Layer const& layer2 = tile2.layers(0);
    CHECK(std::string("layer") == layer2.name());
    CHECK(1 == layer2.features_size());

    mapnik::layer lyr2("layer",map.srs());

    protozero::pbf_reader pbf_tile(buffer);
    pbf_tile.next();
    protozero::pbf_reader layer3 = pbf_tile.get_message();

    std::shared_ptr<mapnik::vector_tile_impl::tile_datasource_pbf> ds = std::make_shared<
                                    mapnik::vector_tile_impl::tile_datasource_pbf>(
                                        layer3,0,0,0);
    CHECK(ds->get_name() == "layer");
    mapnik::box2d<double> bbox(-20037508.342789,-20037508.342789,20037508.342789,20037508.342789);
    ds->set_envelope(bbox);
    CHECK( ds->type() == mapnik::datasource::Vector );
    CHECK( ds->get_geometry_type() == mapnik::datasource_geometry_t::Collection );
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
    lyr2.set_datasource(ds);
    lyr2.add_style("style");
    map2.add_layer(lyr2);
    mapnik::load_map(map2,"test/data/style.xml");
    //std::clog << mapnik::save_map_to_string(map2) << "\n";
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

TEST_CASE("pbf vector tile datasource")
{
    unsigned tile_size = 4096;
    mapnik::Map map(tile_size,tile_size,"+init=epsg:3857");
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

    protozero::pbf_reader pbf_tile(buffer);
    pbf_tile.next();
    protozero::pbf_reader layer2 = pbf_tile.get_message();

    // now actually start the meat of the test
    mapnik::vector_tile_impl::tile_datasource_pbf ds(layer2,0,0,0);
    mapnik::featureset_ptr fs;

    // ensure we can query single feature
    mapnik::box2d<double> bbox(-20037508.342789,-20037508.342789,20037508.342789,20037508.342789);
    fs = ds.features(mapnik::query(bbox));
    mapnik::feature_ptr feat = fs->next();
    REQUIRE(feat != mapnik::feature_ptr());
    CHECK(feat->size() == 0);
    CHECK(fs->next() == mapnik::feature_ptr());
    mapnik::query qq = mapnik::query(mapnik::box2d<double>(-1,-1,1,1));
    qq.add_property_name("name");
    fs = ds.features(qq);
    feat = fs->next();
    REQUIRE(feat != mapnik::feature_ptr());
    CHECK(feat->size() == 1);
    CHECK(feat->get("name") == mapnik::value_unicode_string("null island"));

    // now check that datasource api throws out feature which is outside extent
    fs = ds.features(mapnik::query(mapnik::box2d<double>(-10,-10,-10,-10)));
    CHECK(fs->next() == mapnik::feature_ptr());

    // ensure same behavior for feature_at_point
    fs = ds.features_at_point(mapnik::coord2d(0.0,0.0),0.0001);
    REQUIRE(fs->next() != mapnik::feature_ptr());

    fs = ds.features_at_point(mapnik::coord2d(1.0,1.0),1.0001);
    REQUIRE(fs->next() != mapnik::feature_ptr());

    fs = ds.features_at_point(mapnik::coord2d(-10,-10),0);
    CHECK(fs->next() == mapnik::feature_ptr());

    // finally, make sure attributes are also filtered
    mapnik::feature_ptr f_ptr;
    fs = ds.features(mapnik::query(bbox));
    f_ptr = fs->next();
    REQUIRE(f_ptr != mapnik::feature_ptr());
    // no attributes
    CHECK(f_ptr->context()->size() == 0);

    mapnik::query q(bbox);
    q.add_property_name("name");
    fs = ds.features(q);
    f_ptr = fs->next();
    REQUIRE(f_ptr != mapnik::feature_ptr());
    // one attribute
    CHECK(f_ptr->context()->size() == 1);
}

TEST_CASE("pbf encoding multi line")
{
    mapbox::geometry::multi_line_string<std::int64_t> geom;
    {
        mapbox::geometry::line_string<std::int64_t> ring;
        ring.emplace_back(0,0);
        ring.emplace_back(2,2);
        geom.emplace_back(std::move(ring));
    }
    {
        mapbox::geometry::line_string<std::int64_t> ring;
        ring.emplace_back(1,1);
        ring.emplace_back(2,2);
        geom.emplace_back(std::move(ring));
    }
    vector_tile::Tile tile;
    vector_tile::Tile_Layer * t_layer = tile.add_layers();
    t_layer->set_name("layer");
    t_layer->set_version(2);
    t_layer->set_extent(4096);
    vector_tile::Tile_Feature * t_feature = t_layer->add_features();
    std::int32_t x = 0;
    std::int32_t y = 0;

    std::string feature_str;
    protozero::pbf_writer feature_writer(feature_str);
    CHECK(mapnik::vector_tile_impl::encode_geometry_pbf(geom, feature_writer, x, y));
    t_feature->ParseFromString(feature_str);

    std::string buffer;
    tile.SerializeToString(&buffer);
    REQUIRE(1 == tile.layers_size());
    vector_tile::Tile_Layer const& layer = tile.layers(0);
    REQUIRE(1 == layer.features_size());
    vector_tile::Tile_Feature const& f = layer.features(0);
    CHECK(12 == f.geometry_size());
    CHECK(9 == f.geometry(0)); // 1 move_to
    CHECK(0 == f.geometry(1)); // x:0
    CHECK(0 == f.geometry(2)); // y:0
    CHECK(10 == f.geometry(3)); // 1 line_to
    CHECK(4 == f.geometry(4)); // x:2
    CHECK(4 == f.geometry(5)); // y:2
    CHECK(9 == f.geometry(6)); // 1 move_to
    CHECK(1 == f.geometry(7)); // x:1
    CHECK(1 == f.geometry(8)); // y:1
    CHECK(10 == f.geometry(9)); // 1 line_to
    CHECK(2 == f.geometry(10)); // x:2
    CHECK(2 == f.geometry(11)); // y:2

    mapnik::featureset_ptr fs;
    mapnik::feature_ptr f_ptr;

    protozero::pbf_reader pbf_tile(buffer);
    pbf_tile.next();
    protozero::pbf_reader layer2 = pbf_tile.get_message();

    unsigned tile_size = 4096;
    mapnik::vector_tile_impl::tile_datasource_pbf ds(layer2,0,0,0);
    mapnik::box2d<double> bbox(-20037508.342789,-20037508.342789,20037508.342789,20037508.342789);
    fs = ds.features(mapnik::query(bbox));
    f_ptr = fs->next();
    REQUIRE(f_ptr != mapnik::feature_ptr());
    // no attributes
    CHECK(f_ptr->context()->size() == 0);

    CHECK(f_ptr->get_geometry().is<mapnik::geometry::multi_line_string<double> >());
}


// NOTE: encoding multiple lines as one path is technically incorrect
// because in Mapnik the protocol is to split geometry parts into separate paths.
// However this case should still be supported because keeping a single flat array is an
// important optimization in the case that lines do not need to be labeled in custom ways
// or represented as GeoJSON

TEST_CASE( "pbf decoding empty buffer", "should throw exception" ) {
    std::string buffer;
    protozero::pbf_reader pbf_tile(buffer);
    pbf_tile.next();
    protozero::pbf_reader layer2;
#if defined(DNDEBUG)
    // throws in release mode
    REQUIRE_THROWS(layer2 = pbf_tile.get_message());
#else
    // aborts in protozero in debug mode
#endif
}

TEST_CASE( "pbf decoding garbage buffer", "should throw exception" ) {
    std::string buffer("daufyglwi3h7fseuhfas8w3h,dksufasdf");
    protozero::pbf_reader pbf_tile(buffer);
    REQUIRE_THROWS_AS(pbf_tile.next(), protozero::unknown_pbf_wire_type_exception);
    protozero::pbf_reader layer2;
#if defined(DNDEBUG)
    // throws in release mode
    REQUIRE_THROWS(layer2 = pbf_tile.get_message());
#else
    // aborts in protozero in debug mode
#endif
}


TEST_CASE("pbf decoding some truncated buffers")
{
    unsigned tile_size = 4096;
    mapnik::box2d<double> bbox(-20037508.342789,-20037508.342789,20037508.342789,20037508.342789);
    mapnik::Map map(tile_size,tile_size,"+init=epsg:3857");
    mapnik::layer lyr("layer",map.srs());
    lyr.set_datasource(testing::build_ds(0,0));
    map.add_layer(lyr);

    // Create processor
    mapnik::vector_tile_impl::processor ren(map);
    
    // Request Tile
    mapnik::vector_tile_impl::tile out_tile = ren.create_tile(0,0,0);
    CHECK(out_tile.is_painted() == true);
    CHECK(out_tile.is_empty() == false);
    
    // Now check that the tile is correct.
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

    std::string buffer;
    out_tile.serialize_to_string(buffer);
    // We will test truncating the generated protobuf at every increment.
    //  Most cases should fail, except for the lucky bites where we chop
    // it off at a point that would be valid anyway.
    for (std::size_t i=1; i< buffer.size(); ++i)
    {
      CHECK_THROWS(
      {
          protozero::pbf_reader pbf_tile(buffer.c_str(), i);
          pbf_tile.next();
          protozero::pbf_reader layer2 = pbf_tile.get_message();
          mapnik::vector_tile_impl::tile_datasource_pbf ds(layer2,0,0,0);
          mapnik::featureset_ptr fs;
          mapnik::feature_ptr f_ptr;
          fs = ds.features(mapnik::query(bbox));
          f_ptr = fs->next();
          while (f_ptr != mapnik::feature_ptr())
          {
              f_ptr = fs->next();
          }
      }
      );
    }
}

TEST_CASE("pbf vector tile from simplified geojson")
{
    unsigned tile_size = 256 * 100;
    mapnik::Map map(tile_size,tile_size,"+init=epsg:3857");
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
    double scale = (static_cast<double>(layer.extent()) / tile_size) * tile_size/resolution;

    std::string buffer;
    tile.SerializeToString(&buffer);
    protozero::pbf_reader pbf_tile(buffer);
    pbf_tile.next();
    protozero::pbf_reader pbf_layer = pbf_tile.get_message();
    // Need to loop because they could be encoded in any order
    bool found = false;
    while (!found && pbf_layer.next(2))
    {
      // But there will be only one in our tile, so we'll break out of loop once we find it
      protozero::pbf_reader pbf_feature = pbf_layer.get_message();
      while (!found && pbf_feature.next(4))
      {
          found = true;
          mapnik::vector_tile_impl::GeometryPBF::pbf_itr geom_itr = pbf_feature.get_packed_uint32();
          mapnik::vector_tile_impl::GeometryPBF geoms(geom_itr);
          auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, f.type(), 2, tile_x, tile_y, scale, -1.0 * scale);
          unsigned int n_err = 0;
          mapnik::projection wgs84("+init=epsg:4326",true);
          mapnik::projection merc("+init=epsg:3857",true);
          mapnik::proj_transform prj_trans(merc,wgs84);
          mapnik::geometry::geometry<double> projected_geom = mapnik::geometry::reproject_copy(geom,prj_trans,n_err);
          CHECK( n_err == 0 );
          std::string geojson_string;
          CHECK( mapnik::util::to_geojson(geojson_string,projected_geom) );
          std::string geojson_file = "./test/data/simplified_geometry_pbf.geojson";
          mapnik::util::file input(geojson_file);
          if (!mapnik::util::exists(geojson_file) || (std::getenv("UPDATE") != nullptr))
          {
              std::ofstream out(geojson_file);
              out << geojson_string;
          } else if (input.is_open())
          {
              std::string json_string(input.data().get(), input.size());
              CHECK (geojson_string == json_string);
          }
          break;
      }
    }
    REQUIRE( found );
}

TEST_CASE("pbf raster tile output -- should be able to overzoom raster")
{
    unsigned tile_size = 4096;
    int buffer_size = 1024;
    mapnik::vector_tile_impl::merc_tile out_tile(0, 0, 0, tile_size, buffer_size);
    {
        mapnik::box2d<double> const& bbox = out_tile.extent();
        std::ostringstream s;
        s << std::fixed << std::setprecision(16)
          << bbox.minx() << ',' << bbox.miny() << ','
          << bbox.maxx() << ',' << bbox.maxy();
        
        // build map
        mapnik::Map map(tile_size,tile_size,"+init=epsg:3857");
        map.set_buffer_size(buffer_size);
        mapnik::layer lyr("layer",map.srs());
        mapnik::parameters params;
        params["type"] = "gdal";
        params["extent"] = s.str();
        params["file"] = "test/data/256x256.png";
        std::shared_ptr<mapnik::datasource> ds = mapnik::datasource_cache::instance().create(params);
        lyr.set_datasource(ds);
        map.add_layer(lyr);
        
        // build processor
        mapnik::vector_tile_impl::processor ren(map);
        ren.set_image_format("jpeg");
        ren.set_scaling_method(mapnik::SCALING_BILINEAR);
        
        // Update the tile
        ren.update_tile(out_tile);
    }
    vector_tile::Tile tile;
    tile.ParseFromString(out_tile.get_buffer());
    // Done creating test data, now test created tile
    REQUIRE(1 == tile.layers_size());
    vector_tile::Tile_Layer const& layer = tile.layers(0);
    CHECK(std::string("layer") == layer.name());
    REQUIRE(1 == layer.features_size());
    vector_tile::Tile_Feature const& f = layer.features(0);
    CHECK(static_cast<mapnik::value_integer>(1) == static_cast<mapnik::value_integer>(f.id()));
    CHECK(0 == f.geometry_size());
    CHECK(f.has_raster());
    std::string const& ras_buffer = f.raster();
    CHECK(!ras_buffer.empty());

    // confirm tile looks correct as encoded
    std::string buffer;
    CHECK(tile.SerializeToString(&buffer));

    protozero::pbf_reader pbf_tile(buffer);
    pbf_tile.next();
    protozero::pbf_reader layer2 = pbf_tile.get_message();

    // now read back and render image at larger size
    // and zoomed in
    // 2/0/1.png
    mapnik::box2d<double> bbox = mapnik::vector_tile_impl::tile_mercator_bbox(0,1,2);
    mapnik::Map map2(256,256,"+init=epsg:3857");
    map2.set_buffer_size(1024);
    mapnik::layer lyr2("layer",map2.srs());
    std::shared_ptr<mapnik::vector_tile_impl::tile_datasource_pbf> ds2 = std::make_shared<
                                    mapnik::vector_tile_impl::tile_datasource_pbf>(
                                        layer2,0,0,0);
    lyr2.set_datasource(ds2);
    lyr2.add_style("style");
    map2.add_layer(lyr2);
    mapnik::load_map(map2,"test/data/raster_style.xml");
    map2.zoom_to_box(bbox);
    mapnik::image_rgba8 im(map2.width(),map2.height());
    mapnik::agg_renderer<mapnik::image_rgba8> ren2(map2,im);
    ren2.apply();
    if (!mapnik::util::exists("test/fixtures/expected-3.png"))
    {
        mapnik::save_to_file(im,"test/fixtures/expected-3.png","png32");
    }
    unsigned diff = testing::compare_images(im,"test/fixtures/expected-3.png");
    CHECK(0 == diff);
    if (diff > 0)
    {
        mapnik::save_to_file(im,"test/fixtures/actual-3.png","png32");
    }
}

TEST_CASE("Check that we throw on various valid-but-we-don't-handle PBF encoded files")
{
    std::vector<std::string> filenames = {"test/data/tile_with_extra_feature_field.mvt",
                                          "test/data/tile_with_extra_layer_fields.mvt",
                                          "test/data/tile_with_invalid_layer_value_type.mvt",
                                          "test/data/tile_with_unexpected_geomtype.mvt"};

    for (auto const& f : filenames)
    {
        std::ifstream t(f);
        REQUIRE(t.is_open());
        std::string buffer((std::istreambuf_iterator<char>(t)),
                        std::istreambuf_iterator<char>());
        CHECK_THROWS({
            mapnik::box2d<double> bbox(-20037508.342789,-20037508.342789,20037508.342789,20037508.342789);
            unsigned tile_size = 4096;
              protozero::pbf_reader pbf_tile(buffer);
              pbf_tile.next();
              protozero::pbf_reader layer2 = pbf_tile.get_message();
              mapnik::vector_tile_impl::tile_datasource_pbf ds(layer2,0,0,0);
              mapnik::featureset_ptr fs;
              mapnik::feature_ptr f_ptr;
              fs = ds.features(mapnik::query(bbox));
              f_ptr = fs->next();
              while (f_ptr != mapnik::feature_ptr()) {
                  f_ptr = fs->next();
              }
        });
   }

}

TEST_CASE("pbf vector tile from linestring geojson")
{
    unsigned tile_size = 4096;
    mapnik::Map map(tile_size,tile_size,"+init=epsg:3857");
    mapnik::layer lyr("layer","+init=epsg:4326");
    auto ds = testing::build_geojson_fs_ds("./test/data/linestrings_and_point.geojson");
    lyr.set_datasource(ds);
    map.add_layer(lyr);
    mapnik::vector_tile_impl::processor ren(map);
    // Request Tile
    mapnik::vector_tile_impl::tile out_tile = ren.create_tile(0,0,0);
    CHECK(out_tile.is_painted() == true);
    CHECK(out_tile.is_empty() == false);

    // Now check that the tile is correct.
    vector_tile::Tile tile;
    tile.ParseFromString(out_tile.get_buffer());
    REQUIRE(1 == tile.layers_size());
    vector_tile::Tile_Layer const& layer = tile.layers(0);
    CHECK(std::string("layer") == layer.name());
    REQUIRE(3 == layer.features_size());
    std::string buffer;
    tile.SerializeToString(&buffer);
    protozero::pbf_reader pbf_tile(buffer);
    pbf_tile.next();
    protozero::pbf_reader layer3 = pbf_tile.get_message();
    std::shared_ptr<mapnik::vector_tile_impl::tile_datasource_pbf> ds2 = std::make_shared<
                                    mapnik::vector_tile_impl::tile_datasource_pbf>(
                                        layer3,0,0,0);
    CHECK(ds2->get_name() == "layer");
    
    mapnik::box2d<double> bbox(-20037508.342789,-20037508.342789,20037508.342789,20037508.342789);
    mapnik::query q(bbox);
    q.add_property_name("x");
    q.add_property_name("y");
    q.add_property_name("pbool");
    
    std::size_t expected_num_attr_returned = q.property_names().size();
    auto fs = ds2->features(q);
    auto f_ptr = fs->next();
    REQUIRE(f_ptr != mapnik::feature_ptr());
    // no attributes
    CHECK(f_ptr->context()->size() == expected_num_attr_returned);
    CHECK(f_ptr->get_geometry().is<mapnik::geometry::line_string<double> >());
    
    // second feature
    f_ptr = fs->next();
    REQUIRE(f_ptr != mapnik::feature_ptr());
    CHECK(f_ptr->context()->size() == expected_num_attr_returned);
    CHECK(f_ptr->get_geometry().is<mapnik::geometry::line_string<double> >());

    // third feature (x : 0, y : 10, pbool : false)
    f_ptr = fs->next();
    REQUIRE(f_ptr != mapnik::feature_ptr());
    CHECK(f_ptr->context()->size() == expected_num_attr_returned);
    CHECK(f_ptr->get_geometry().is<mapnik::geometry::point<double> >());

    CHECK(f_ptr->has_key("x") == true);
    CHECK_NOTHROW(f_ptr->get("x").get<long int>());
    CHECK(f_ptr->get("x") == 0);

    CHECK(f_ptr->has_key("y") == true);
    CHECK_NOTHROW(f_ptr->get("y").get<long int>());
    CHECK(f_ptr->get("y") == 10);

    CHECK(f_ptr->has_key("pbool") == true);
    CHECK_NOTHROW(f_ptr->get("pbool").get<bool>());
    CHECK(f_ptr->get("pbool").get<bool>() == false);

    // only three features
    f_ptr = fs->next();
    CHECK(f_ptr == mapnik::feature_ptr());
}
