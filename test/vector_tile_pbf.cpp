#include "catch.hpp"

// test utils
#include "test_utils.hpp"
#include <mapnik/memory_datasource.hpp>
#include <mapnik/util/fs.hpp>
#include <mapnik/agg_renderer.hpp>
#include <mapnik/feature_factory.hpp>
#include <mapnik/load_map.hpp>
#include <mapnik/image_util.hpp>
#include <mapnik/vertex_adapters.hpp>
#include <mapnik/projection.hpp>
#include <mapnik/proj_transform.hpp>
#include <mapnik/geometry_is_empty.hpp>
#include <mapnik/util/geometry_to_geojson.hpp>
#include <mapnik/util/geometry_to_wkt.hpp>
#include <mapnik/geometry_reprojection.hpp>
#include <mapnik/geometry_transform.hpp>
#include <mapnik/geometry_strategy.hpp>
#include <mapnik/proj_strategy.hpp>
#include <mapnik/geometry.hpp>

#include <boost/optional/optional_io.hpp>

// vector output api
#include "vector_tile_compression.hpp"
#include "vector_tile_processor.hpp"
#include "vector_tile_backend_pbf.hpp"
#include "vector_tile_util.hpp"
#include "vector_tile_projection.hpp"
#include "vector_tile_geometry_decoder.hpp"

// vector input api
#include "vector_tile_datasource.hpp"
#include "vector_tile_datasource_pbf.hpp"
#include "pbf_reader.hpp"

TEST_CASE( "pbf vector tile input", "should be able to parse message and render point" ) {
    typedef mapnik::vector_tile_impl::backend_pbf backend_type;
    typedef mapnik::vector_tile_impl::processor<backend_type> renderer_type;
    typedef vector_tile::Tile tile_type;
    tile_type tile;
    backend_type backend(tile,16);
    unsigned tile_size = 256;
    mapnik::box2d<double> bbox(-20037508.342789,-20037508.342789,20037508.342789,20037508.342789);
    mapnik::Map map(tile_size,tile_size,"+init=epsg:3857");
    mapnik::layer lyr("layer",map.srs());
    lyr.set_datasource(testing::build_ds(0,0));
    map.add_layer(lyr);
    map.zoom_to_box(bbox);
    mapnik::request m_req(map.width(),map.height(),map.get_current_extent());
    renderer_type ren(backend,map,m_req);
    ren.apply();
    // serialize to message
    std::string buffer;
    CHECK(tile.SerializeToString(&buffer));
    CHECK(52 == buffer.size());
    // now create new objects
    mapnik::Map map2(tile_size,tile_size,"+init=epsg:3857");
    tile_type tile2;
    CHECK(tile2.ParseFromString(buffer));
    std::string key("");
    CHECK(false == mapnik::vector_tile_impl::is_solid_extent(tile2,key));
    CHECK("" == key);
    CHECK(1 == tile2.layers_size());
    vector_tile::Tile_Layer const& layer2 = tile2.layers(0);
    CHECK(std::string("layer") == layer2.name());
    CHECK(1 == layer2.features_size());

    mapnik::layer lyr2("layer",map.srs());
    
    mapbox::util::pbf pbf_tile(buffer.c_str(), buffer.size());
    pbf_tile.next();
    mapbox::util::pbf layer3 = pbf_tile.get_message();
    
    std::shared_ptr<mapnik::vector_tile_impl::tile_datasource_pbf> ds = std::make_shared<
                                    mapnik::vector_tile_impl::tile_datasource_pbf>(
                                        layer3,0,0,0,map2.width());
    ds->set_envelope(bbox);
    CHECK( ds->type() == mapnik::datasource::Vector );
    CHECK( ds->get_geometry_type() == mapnik::datasource_geometry_t::Collection );
    mapnik::layer_descriptor lay_desc = ds->get_descriptor();
    std::vector<std::string> expected_names;
    expected_names.push_back("name");
    std::vector<std::string> names;
    for (auto const& desc : lay_desc.get_descriptors())
    {
        names.push_back(desc.get_name());
    }
    
    CHECK(names == expected_names);
    lyr2.set_datasource(ds);
    lyr2.add_style("style");
    map2.add_layer(lyr2);
    mapnik::load_map(map2,"test/data/style.xml");
    //std::clog << mapnik::save_map_to_string(map2) << "\n";
    map2.zoom_to_box(bbox);
    mapnik::image_rgba8 im(map2.width(),map2.height());
    mapnik::agg_renderer<mapnik::image_rgba8> ren2(map2,im);
    ren2.apply();
    if (!mapnik::util::exists("test/fixtures/expected-1.png")) {
        mapnik::save_to_file(im,"test/fixtures/expected-1.png","png32");
    }
    unsigned diff = testing::compare_images(im,"test/fixtures/expected-1.png");
    CHECK(0 == diff);
    if (diff > 0) {
        mapnik::save_to_file(im,"test/fixtures/actual-1.png","png32");
    }
}


TEST_CASE( "pbf vector tile datasource", "should filter features outside extent" ) {
    typedef mapnik::vector_tile_impl::backend_pbf backend_type;
    typedef mapnik::vector_tile_impl::processor<backend_type> renderer_type;
    typedef vector_tile::Tile tile_type;
    tile_type tile;
    backend_type backend(tile,16);
    unsigned tile_size = 256;
    mapnik::box2d<double> bbox(-20037508.342789,-20037508.342789,20037508.342789,20037508.342789);
    mapnik::Map map(tile_size,tile_size,"+init=epsg:3857");
    mapnik::layer lyr("layer",map.srs());
    lyr.set_datasource(testing::build_ds(0,0));
    map.add_layer(lyr);
    mapnik::request m_req(tile_size,tile_size,bbox);
    renderer_type ren(backend,map,m_req);
    ren.apply();
    std::string key("");
    CHECK(false == mapnik::vector_tile_impl::is_solid_extent(tile,key));
    CHECK("" == key);
    CHECK(1 == tile.layers_size());
    vector_tile::Tile_Layer const& layer = tile.layers(0);
    CHECK(std::string("layer") == layer.name());
    CHECK(1 == layer.features_size());
    vector_tile::Tile_Feature const& f = layer.features(0);
    CHECK(static_cast<mapnik::value_integer>(1) == static_cast<mapnik::value_integer>(f.id()));
    CHECK(3 == f.geometry_size());
    CHECK(9 == f.geometry(0));
    CHECK(4096 == f.geometry(1));
    CHECK(4096 == f.geometry(2));
    
    std::string buffer;
    tile.SerializeToString(&buffer);
    mapbox::util::pbf pbf_tile(buffer.c_str(), buffer.size());
    pbf_tile.next();
    mapbox::util::pbf layer2 = pbf_tile.get_message();
    
    // now actually start the meat of the test
    mapnik::vector_tile_impl::tile_datasource_pbf ds(layer2,0,0,0,tile_size);
    mapnik::featureset_ptr fs;

    // ensure we can query single feature
    fs = ds.features(mapnik::query(bbox));
    mapnik::feature_ptr feat = fs->next();
    CHECK(feat != mapnik::feature_ptr());
    CHECK(feat->size() == 0);
    CHECK(fs->next() == mapnik::feature_ptr());
    mapnik::query qq = mapnik::query(mapnik::box2d<double>(-1,-1,1,1));
    qq.add_property_name("name");
    fs = ds.features(qq);
    feat = fs->next();
    CHECK(feat != mapnik::feature_ptr());
    CHECK(feat->size() == 1);
//    CHECK(feat->get("name") == "null island");

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

// NOTE: encoding multiple lines as one path is technically incorrect
// because in Mapnik the protocol is to split geometry parts into separate paths.
// However this case should still be supported because keeping a single flat array is an
// important optimization in the case that lines do not need to be labeled in custom ways
// or represented as GeoJSON
TEST_CASE( "pbf encoding multi line as one path", "should maintain second move_to command" ) {
    // Options
    // here we use a multiplier of 1 to avoid rounding numbers
    // and stay in integer space for simplity
    unsigned path_multiplier = 1;
    // here we use an extreme tolerance to prove that all vertices are maintained no matter
    // the tolerance because we never want to drop a move_to or the first line_to
    //unsigned tolerance = 2000000;
    // now create the testing data
    vector_tile::Tile tile;
    unsigned tile_size = 256;
    mapnik::box2d<double> bbox(-20037508.342789,-20037508.342789,20037508.342789,20037508.342789);
    mapnik::vector_tile_impl::backend_pbf backend(tile,path_multiplier);
    backend.start_tile_layer("layer");
    mapnik::feature_ptr feature(mapnik::feature_factory::create(std::make_shared<mapnik::context_type>(),1));
    backend.start_tile_feature(*feature);
    mapnik::geometry::multi_line_string<std::int64_t> geom;
    {
        mapnik::geometry::linear_ring<std::int64_t> ring;
        ring.add_coord(0,0);
        ring.add_coord(2,2);
        geom.emplace_back(std::move(ring));
    }
    {
        mapnik::geometry::linear_ring<std::int64_t> ring;
        ring.add_coord(1,1);
        ring.add_coord(2,2);
        geom.emplace_back(std::move(ring));
    }
    /*
    g->move_to(0,0);        // takes 3 geoms: command length,x,y
    g->line_to(2,2);        // new command, so again takes 3 geoms: command length,x,y | total 6
    g->move_to(1,1);        // takes 3 geoms: command length,x,y
    g->line_to(2,2);        // new command, so again takes 3 geoms: command length,x,y | total 6
    */
    backend.current_feature_->set_type(vector_tile::Tile_GeomType_LINESTRING);
    for (auto const& line : geom)
    {
        backend.add_path(line);
    }
    backend.stop_tile_feature();
    backend.stop_tile_layer();
    // done encoding single feature/geometry
    std::string key("");
    CHECK(false == mapnik::vector_tile_impl::is_solid_extent(tile,key));
    CHECK("" == key);
    CHECK(1 == tile.layers_size());
    vector_tile::Tile_Layer const& layer = tile.layers(0);
    CHECK(1 == layer.features_size());
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
    
    
    std::string buffer;
    tile.SerializeToString(&buffer);
    mapbox::util::pbf pbf_tile(buffer.c_str(), buffer.size());
    pbf_tile.next();
    mapbox::util::pbf layer2 = pbf_tile.get_message();
    

    mapnik::vector_tile_impl::tile_datasource_pbf ds(layer2,0,0,0,tile_size);
    fs = ds.features(mapnik::query(bbox));
    f_ptr = fs->next();
    CHECK(f_ptr != mapnik::feature_ptr());
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
    mapbox::util::pbf pbf_tile(buffer.c_str(), buffer.size());
    pbf_tile.next();
    mapbox::util::pbf layer2;
    REQUIRE_THROWS(layer2 = pbf_tile.get_message());
}

TEST_CASE( "pbf decoding garbage buffer", "should throw exception" ) {
    std::string buffer("daufyglwi3h7fseuhfas8w3h,dksufasdf");
    mapbox::util::pbf pbf_tile(buffer.c_str(), buffer.size());
    pbf_tile.next();
    mapbox::util::pbf layer2;
    REQUIRE_THROWS(layer2 = pbf_tile.get_message());
}


TEST_CASE( "pbf decoding some truncated buffers", "should throw exception" ) {
  
    typedef mapnik::vector_tile_impl::backend_pbf backend_type;
    typedef mapnik::vector_tile_impl::processor<backend_type> renderer_type;
    typedef vector_tile::Tile tile_type;
    tile_type tile;
    backend_type backend(tile,16);
    unsigned tile_size = 256;
    mapnik::box2d<double> bbox(-20037508.342789,-20037508.342789,20037508.342789,20037508.342789);
    mapnik::Map map(tile_size,tile_size,"+init=epsg:3857");
    mapnik::layer lyr("layer",map.srs());
    lyr.set_datasource(testing::build_ds(0,0));
    map.add_layer(lyr);
    mapnik::request m_req(tile_size,tile_size,bbox);
    renderer_type ren(backend,map,m_req);
    ren.apply();
    std::string key("");
    CHECK(false == mapnik::vector_tile_impl::is_solid_extent(tile,key));
    CHECK("" == key);
    CHECK(1 == tile.layers_size());
    vector_tile::Tile_Layer const& layer = tile.layers(0);
    CHECK(std::string("layer") == layer.name());
    CHECK(1 == layer.features_size());
    vector_tile::Tile_Feature const& f = layer.features(0);
    CHECK(static_cast<mapnik::value_integer>(1) == static_cast<mapnik::value_integer>(f.id()));
    CHECK(3 == f.geometry_size());
    CHECK(9 == f.geometry(0));
    CHECK(4096 == f.geometry(1));
    CHECK(4096 == f.geometry(2));
  

    // We will test truncating the generated protobuf at every increment.
    //  Most cases should fail, except for the lucky bites where we chop
    // it off at a point that would be valid anyway.
    std::string buffer;
    tile.SerializeToString(&buffer);
    for (int i=1; i< buffer.size(); i++) 
    {
      CHECK_THROWS({
          mapbox::util::pbf pbf_tile(buffer.c_str(), i);
          pbf_tile.next();
          mapbox::util::pbf layer2 = pbf_tile.get_message();
          mapnik::vector_tile_impl::tile_datasource_pbf ds(layer2,0,0,0,tile_size);
          mapnik::featureset_ptr fs;
          mapnik::feature_ptr f_ptr;
          fs = ds.features(mapnik::query(bbox));
          while (f_ptr != mapnik::feature_ptr()) {
              f_ptr = fs->next();
          }
      });
    }
}
