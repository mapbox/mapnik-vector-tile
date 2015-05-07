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
#include <mapnik/geometry_reprojection.hpp>
#include <mapnik/geometry_is_empty.hpp>
#include <mapnik/util/geometry_to_geojson.hpp>
#include <mapnik/util/geometry_to_wkt.hpp>
#include <mapnik/geometry_reprojection.hpp>
#include <mapnik/geometry_transform.hpp>
#include <mapnik/geometry_strategy.hpp>
#include <mapnik/proj_strategy.hpp>


// vector output api
#include "vector_tile_compression.hpp"
#include "vector_tile_processor.hpp"
#include "vector_tile_backend_pbf.hpp"
#include "vector_tile_util.hpp"
#include "vector_tile_projection.hpp"
#include "vector_tile_geometry_decoder.hpp"

// vector input api
#include "vector_tile_datasource.hpp"

/*
TEST_CASE( "vector tile negative id", "hmm" ) {
    vector_tile::Tile tile;
    vector_tile::Tile_Layer * layer = tile.add_layers();
    vector_tile::Tile_Feature * feat = layer->add_features();
    feat->set_id(-1);
    std::clog << feat->id() << "\n";
    //CHECK(std::fabs(map_extent.maxy() - e.maxy()) < epsilon);
}
*/

TEST_CASE( "vector tile compression", "should be able to round trip gzip and zlib data" ) {
    std::string data("amazing data");
    CHECK(!mapnik::vector_tile_impl::is_zlib_compressed(data));
    CHECK(!mapnik::vector_tile_impl::is_gzip_compressed(data));
    std::string zlibbed;
    mapnik::vector_tile_impl::zlib_compress(data,zlibbed,false);
    // failing - why?
    //CHECK(mapnik::vector_tile_impl::is_zlib_compressed(zlibbed));
    CHECK(!mapnik::vector_tile_impl::is_gzip_compressed(zlibbed));

    std::string unzlibbed;
    mapnik::vector_tile_impl::zlib_decompress(zlibbed,unzlibbed);
    CHECK(data == unzlibbed);

    std::string gzipped;
    mapnik::vector_tile_impl::zlib_compress(data,gzipped,true);
    CHECK(!mapnik::vector_tile_impl::is_zlib_compressed(gzipped));
    CHECK(mapnik::vector_tile_impl::is_gzip_compressed(gzipped));

    std::string ungzipped;
    mapnik::vector_tile_impl::zlib_decompress(gzipped,ungzipped);
    CHECK(data == ungzipped);
}

TEST_CASE( "vector tile projection 1", "should support z/x/y to bbox conversion at 0/0/0" ) {
    mapnik::vector_tile_impl::spherical_mercator merc(256);
    double minx,miny,maxx,maxy;
    merc.xyz(0,0,0,minx,miny,maxx,maxy);
    mapnik::box2d<double> map_extent(minx,miny,maxx,maxy);
    mapnik::box2d<double> e(-20037508.342789,-20037508.342789,20037508.342789,20037508.342789);
    double epsilon = 0.000001;
    CHECK(std::fabs(map_extent.minx() - e.minx()) < epsilon);
    CHECK(std::fabs(map_extent.miny() - e.miny()) < epsilon);
    CHECK(std::fabs(map_extent.maxx() - e.maxx()) < epsilon);
    CHECK(std::fabs(map_extent.maxy() - e.maxy()) < epsilon);
}

TEST_CASE( "vector tile projection 2", "should support z/x/y to bbox conversion up to z33" ) {
    mapnik::vector_tile_impl::spherical_mercator merc(256);
    int x = 2145960701;
    int y = 1428172928;
    int z = 32;
    double minx,miny,maxx,maxy;
    merc.xyz(x,y,z,minx,miny,maxx,maxy);
    mapnik::box2d<double> map_extent(minx,miny,maxx,maxy);
    mapnik::box2d<double> e(-14210.1492817168364127,6711666.7204630710184574,-14210.1399510249066225,6711666.7297937674447894);
    double epsilon = 0.00000001;
    CHECK(std::fabs(map_extent.minx() - e.minx()) < epsilon);
    CHECK(std::fabs(map_extent.miny() - e.miny()) < epsilon);
    CHECK(std::fabs(map_extent.maxx() - e.maxx()) < epsilon);
    CHECK(std::fabs(map_extent.maxy() - e.maxy()) < epsilon);
}

TEST_CASE( "vector tile output 1", "should create vector tile with two points" ) {
    typedef mapnik::vector_tile_impl::backend_pbf backend_type;
    typedef mapnik::vector_tile_impl::processor<backend_type> renderer_type;
    typedef vector_tile::Tile tile_type;
    tile_type tile;
    backend_type backend(tile,16);
    unsigned tile_size = 256;
    mapnik::box2d<double> bbox(-20037508.342789,-20037508.342789,20037508.342789,20037508.342789);
    mapnik::Map map(tile_size,tile_size,"+init=epsg:3857");
    mapnik::layer lyr("layer",map.srs());
    lyr.set_datasource(testing::build_ds(0,0,true));
    map.add_layer(lyr);
    mapnik::request m_req(tile_size,tile_size,bbox);
    renderer_type ren(backend,map,m_req);
    ren.apply();
    std::string key("");
    CHECK(false == mapnik::vector_tile_impl::is_solid_extent(tile,key));
    CHECK("" == key);
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
    CHECK(95 == tile.ByteSize());
    std::string buffer;
    CHECK(tile.SerializeToString(&buffer));
    CHECK(95 == buffer.size());
}

TEST_CASE( "vector tile output 2", "adding empty layers should result in empty tile" ) {
    typedef mapnik::vector_tile_impl::backend_pbf backend_type;
    typedef mapnik::vector_tile_impl::processor<backend_type> renderer_type;
    typedef vector_tile::Tile tile_type;
    tile_type tile;
    backend_type backend(tile,16);
    unsigned tile_size = 256;
    mapnik::box2d<double> bbox(-20037508.342789,-20037508.342789,20037508.342789,20037508.342789);
    mapnik::Map map(tile_size,tile_size,"+init=epsg:3857");
    map.add_layer(mapnik::layer("layer",map.srs()));
    map.zoom_to_box(bbox);
    mapnik::request m_req(tile_size,tile_size,bbox);
    renderer_type ren(backend,map,m_req);
    ren.apply();
    std::string key("");
    CHECK(true == mapnik::vector_tile_impl::is_solid_extent(tile,key));
    CHECK("" == key);
    CHECK(0 == tile.layers_size());
}

TEST_CASE( "vector tile output 3", "adding layers with geometries outside rendering extent should not add layer" ) {
    typedef mapnik::vector_tile_impl::backend_pbf backend_type;
    typedef mapnik::vector_tile_impl::processor<backend_type> renderer_type;
    typedef vector_tile::Tile tile_type;
    tile_type tile;
    backend_type backend(tile,16);
    unsigned tile_size = 256;
    mapnik::box2d<double> bbox(-20037508.342789,-20037508.342789,20037508.342789,20037508.342789);
    mapnik::Map map(tile_size,tile_size,"+init=epsg:3857");
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
    map.zoom_to_box(custom_bbox);
    mapnik::request m_req(tile_size,tile_size,custom_bbox);
    renderer_type ren(backend,map,m_req);
    ren.apply();
    std::string key("");
    CHECK(true == mapnik::vector_tile_impl::is_solid_extent(tile,key));
    CHECK("" == key);
    CHECK(0 == tile.layers_size());
}

TEST_CASE( "vector tile output 4", "adding layers with degenerate geometries should not add layer" ) {
    typedef mapnik::vector_tile_impl::backend_pbf backend_type;
    typedef mapnik::vector_tile_impl::processor<backend_type> renderer_type;
    typedef vector_tile::Tile tile_type;
    tile_type tile;
    backend_type backend(tile,16);
    unsigned tile_size = 256;
    mapnik::box2d<double> bbox(-20037508.342789,-20037508.342789,20037508.342789,20037508.342789);
    mapnik::Map map(tile_size,tile_size,"+init=epsg:3857");
    mapnik::layer lyr("layer",map.srs());
    // create a datasource with a feature outside the map
    std::shared_ptr<mapnik::memory_datasource> ds = testing::build_ds(bbox.minx()-1,bbox.miny()-1);
    // but fake the overall envelope to ensure the layer is still processed
    // and then removed given no intersecting features will be added
    ds->set_envelope(bbox);
    lyr.set_datasource(ds);
    map.add_layer(lyr);
    map.zoom_to_box(bbox);
    mapnik::request m_req(tile_size,tile_size,bbox);
    renderer_type ren(backend,map,m_req);
    ren.apply();
    std::string key("");
    CHECK(true == mapnik::vector_tile_impl::is_solid_extent(tile,key));
    CHECK("" == key);
    CHECK(0 == tile.layers_size());
}

TEST_CASE( "vector tile input", "should be able to parse message and render point" ) {
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
    std::shared_ptr<mapnik::vector_tile_impl::tile_datasource> ds = std::make_shared<
                                    mapnik::vector_tile_impl::tile_datasource>(
                                        layer2,0,0,0,map2.width());
    ds->set_envelope(bbox);
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


TEST_CASE( "vector tile datasource", "should filter features outside extent" ) {
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
    // now actually start the meat of the test
    mapnik::vector_tile_impl::tile_datasource ds(layer,0,0,0,tile_size);
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
TEST_CASE( "encoding multi line as one path", "should maintain second move_to command" ) {
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

    mapnik::vector_tile_impl::tile_datasource ds(layer,0,0,0,tile_size);
    fs = ds.features(mapnik::query(bbox));
    f_ptr = fs->next();
    CHECK(f_ptr != mapnik::feature_ptr());
    // no attributes
    CHECK(f_ptr->context()->size() == 0);

    CHECK(f_ptr->get_geometry().is<mapnik::geometry::multi_line_string<double> >());
}


TEST_CASE( "encoding single line 1", "should maintain start/end vertex" ) {
    // Options
    // here we use a multiplier of 1 to avoid rounding numbers
    // this works because we are staying in integer space for this test
    unsigned path_multiplier = 1;
    // here we use a tolerance of 2. Along with a multiplier of 1 this
    // says to discard any verticies that are not at least >= 2 different
    // in both the x and y from the previous vertex
    // unsigned tolerance = 2;
    // now create the testing data
    vector_tile::Tile tile;
    mapnik::vector_tile_impl::backend_pbf backend(tile,path_multiplier);
    backend.start_tile_layer("layer");
    mapnik::feature_ptr feature(mapnik::feature_factory::create(std::make_shared<mapnik::context_type>(),1));
    backend.start_tile_feature(*feature);
    /*
    std::unique_ptr<mapnik::geometry_type> g(new mapnik::geometry_type(mapnik::geometry_type::types::LineString));
    g->move_to(0,0);        // takes 3 geoms: command length,x,y
    g->line_to(2,2);        // new command, so again takes 3 geoms: command length,x,y | total 6
    g->line_to(1000,1000);  // repeated line_to, so only takes 2 geoms: x,y | total 8
    g->line_to(1001,1001);  // should skip given tolerance of 2 | total 8
    g->line_to(1001,1001);  // should not skip given it is the endpoint, added 2 geoms | total 10
    */
    mapnik::geometry::line_string<std::int64_t> geom;
    geom.add_coord(0,0);
    geom.add_coord(2,2);
    geom.add_coord(1000,1000);
    geom.add_coord(1001,1001);
    geom.add_coord(1001,1001);
    backend.current_feature_->set_type(vector_tile::Tile_GeomType_LINESTRING);
    backend.add_path(geom);
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
    // sequence of 10 geometries given tolerance of 2
    CHECK(12 == f.geometry_size());
    // first geometry is 9, which packs both the command and how many verticies are encoded with that same command
    // It is 9 because it is a move_to (which is an enum of 1) and there is one command (length == 1)
    unsigned move_value = f.geometry(0);
    // (1      << 3) | (1       & ((1 << 3) -1)) == 9
    // (length << 3) | (MOVE_TO & ((1 << 3) -1))
    CHECK(9 == move_value);
    unsigned move_cmd = move_value & ((1 << 3) - 1);
    CHECK(1 == move_cmd);
    unsigned move_length = move_value >> 3;
    CHECK(1 == move_length);
    // 2nd and 3rd are the x,y of the one move_to command
    CHECK(0 == f.geometry(1));
    CHECK(0 == f.geometry(2));
    // 4th is the line_to (which is an enum of 2) and a number indicating how many line_to commands are repeated
    // in this case there should be 2 because two were skipped
    unsigned line_value = f.geometry(3);
    // (2      << 3) | (2       & ((1 << 3) -1)) == 18
    CHECK(34 == line_value);
    unsigned line_cmd = line_value & ((1 << 3) - 1);
    CHECK(2 == line_cmd);
    unsigned line_length = line_value >> 3;
    CHECK(4 == line_length);
    // 5th and 6th are the x,y of the first line_to command
    // due zigzag encoding the 2,2 should be 4,4
    // delta encoding has no impact since the previous coordinate was 0,0
    unsigned four = (2 << 1) ^ (2 >> 31);
    CHECK(4 == four);
    CHECK(4 == f.geometry(4));
    CHECK(4 == f.geometry(5));
    // 7th and 8th are x,y of the second line_to command
    // due to delta encoding 1001-2 becomes 999
    // zigzag encoded 999 becomes 1998 == (999 << 1) ^ (999 >> 31)
    CHECK(1996 == f.geometry(6));
    CHECK(1996 == f.geometry(7));
}

// testcase for avoiding error in mapnik::vector_tile_impl::is_solid_extent of
// "Unknown command type (is_solid_extent): 0"
// not yet clear if this test is correct
// ported from shapefile test in tilelive-bridge (a:should render a (1.0.1))
TEST_CASE( "encoding single line 2", "should maintain start/end vertex" ) {
    unsigned path_multiplier = 16;
    //unsigned tolerance = 5;
    vector_tile::Tile tile;
    mapnik::vector_tile_impl::backend_pbf backend(tile,path_multiplier);
    backend.start_tile_layer("layer");
    mapnik::feature_ptr feature(mapnik::feature_factory::create(std::make_shared<mapnik::context_type>(),1));
    backend.start_tile_feature(*feature);
    /*
    std::unique_ptr<mapnik::geometry_type> g(new mapnik::geometry_type(mapnik::geometry_type::types::Polygon));
    g->move_to(168.267850,-24.576888);
    g->line_to(167.982618,-24.697145);
    g->line_to(168.114561,-24.783548);
    g->line_to(168.267850,-24.576888);
    g->line_to(168.267850,-24.576888);
    g->close_path();
    //g->push_vertex(256.000000,-0.00000, mapnik::SEG_CLOSE);
    // todo - why does shape_io result in on-zero close path x,y?
    mapnik::vertex_adapter va(*g);
    backend.add_path(va, tolerance, g->type());
    */
    mapnik::geometry::polygon<double> geom;
    {
        mapnik::geometry::linear_ring<double> ring;
        ring.add_coord(168.267850,-24.576888);
        ring.add_coord(167.982618,-24.697145);
        ring.add_coord(168.114561,-24.783548);
        ring.add_coord(168.267850,-24.576888);
        ring.add_coord(168.267850,-24.576888);
        geom.set_exterior_ring(std::move(ring));
    }
    mapnik::geometry::scale_strategy scale_strat(backend.get_path_multiplier(), 0.5);
    mapnik::geometry::polygon<std::int64_t> poly = mapnik::geometry::transform<std::int64_t>(geom, scale_strat);
    std::string foo;
    mapnik::util::to_wkt(foo, poly);
    INFO(foo);
    backend.current_feature_->set_type(vector_tile::Tile_GeomType_POLYGON);
    backend.add_path(poly);
    backend.stop_tile_feature();
    backend.stop_tile_layer();
    std::string key("");
    CHECK(false == mapnik::vector_tile_impl::is_solid_extent(tile,key));
    CHECK("" == key);
    CHECK(1 == tile.layers_size());
    vector_tile::Tile_Layer const& layer = tile.layers(0);
    CHECK(1 == layer.features_size());
    vector_tile::Tile_Feature const& f = layer.features(0);
    CHECK(11 == f.geometry_size());
}

mapnik::geometry::geometry<double> round_trip(mapnik::geometry::geometry<double> const& geom,
                                      double simplify_distance=0.0)
{
    typedef mapnik::vector_tile_impl::backend_pbf backend_type;
    typedef mapnik::vector_tile_impl::processor<backend_type> renderer_type;
    typedef vector_tile::Tile tile_type;
    tile_type tile;
    unsigned path_multiplier = 1000;
    backend_type backend(tile,path_multiplier);
    unsigned tile_size = 256;
    mapnik::box2d<double> bbox(-180,-90,180,90);
    mapnik::Map map(tile_size,tile_size,"+init=epsg:4326");
    mapnik::request m_req(tile_size,tile_size,bbox);
    renderer_type ren(backend,map,m_req,1,0,0,0);
    // instead of calling apply, let's cheat and test `handle_geometry` directly by adding features
    backend.start_tile_layer("layer");
    mapnik::context_ptr ctx = std::make_shared<mapnik::context_type>();
    mapnik::feature_ptr feature(mapnik::feature_factory::create(ctx,1));
    mapnik::projection wgs84("+init=epsg:4326",true);
    mapnik::projection merc("+init=epsg:4326",true);
    mapnik::proj_transform prj_trans(merc,wgs84);
    ren.set_simplify_distance(simplify_distance);
    ren.handle_geometry(*feature,geom,prj_trans,bbox);
    backend.stop_tile_layer();
    if (tile.layers_size() != 1)
    {
        std::stringstream s;
        s << "expected 1 layer in `round_trip` found " << tile.layers_size();
        throw std::runtime_error(s.str());
    }
    vector_tile::Tile_Layer const& layer = tile.layers(0);
    if (layer.features_size() != 1)
    {
        std::stringstream s;
        s << "expected 1 feature in `round_trip` found " << layer.features_size();
        throw std::runtime_error(s.str());
    }
    vector_tile::Tile_Feature const& f = layer.features(0);
    double scale = (double)path_multiplier;
    return mapnik::vector_tile_impl::decode_geometry(f,0,0,scale,-1*scale);
}

/*
auto pt2 = mapnik::util::get<mapnik::geometry::point>(geom);
std::clog << "pt2 " << pt2.x << " " << pt2.y << "\n";
unsigned int n_err = 0;
mapnik::geometry::geometry projected_geom = mapnik::reproject(geom,prj_trans,n_err,true);
auto pt1 = mapnik::util::get<mapnik::geometry::point>(projected_geom);
CHECK( pt0.x == pt1.x );
CHECK( pt0.y == pt1.y );

mapnik::vector_tile_impl::tile_datasource ds(layer,0,0,0,tile_size);
mapnik::featureset_ptr fs;

// ensure we can query single feature
fs = ds.features(mapnik::query(bbox));
mapnik::feature_ptr feat = fs->next();
auto const& geom2 = feat->get_geometry();
auto pt3 = mapnik::util::get<mapnik::geometry::point>(geom2);
std::clog << "pt3 " << pt3.x << " " << pt3.y << "\n";
*/

TEST_CASE( "vector tile point encoding", "should create vector tile with data" ) {
    mapnik::geometry::point<double> geom(0,0);
    mapnik::geometry::geometry<double> new_geom = round_trip(geom);
    CHECK( !mapnik::geometry::is_empty(new_geom) );
    CHECK( new_geom.is<mapnik::geometry::point<double> >() );
}

TEST_CASE( "vector tile multi_point encoding of single point", "should create vector tile with data" ) {
    mapnik::geometry::multi_point<double> geom;
    geom.emplace_back(0,0);
    mapnik::geometry::geometry<double> new_geom = round_trip(geom);
    CHECK( !mapnik::geometry::is_empty(new_geom) );
    std::string wkt;
    CHECK( mapnik::util::to_wkt(wkt, new_geom) );
    CHECK( wkt == "POINT(128 -128)" );
    CHECK( new_geom.is<mapnik::geometry::point<double> >() );
}

TEST_CASE( "vector tile multi_point encoding of actual multi_point", "should create vector tile with data" ) {
    mapnik::geometry::multi_point<double> geom;
    geom.emplace_back(0,0);
    geom.emplace_back(1,1);
    mapnik::geometry::geometry<double> new_geom = round_trip(geom);
    CHECK( !mapnik::geometry::is_empty(new_geom) );
    std::string wkt;
    CHECK( mapnik::util::to_wkt(wkt, new_geom) );
    CHECK( wkt == "MULTIPOINT(128 -128,128.711 -126.578)" );
    CHECK( new_geom.is<mapnik::geometry::multi_point<double> >() );
}

TEST_CASE( "vector tile line_string encoding", "should create vector tile with data" ) {
    mapnik::geometry::line_string<double> geom;
    geom.add_coord(0,0);
    geom.add_coord(100,100);
    mapnik::geometry::geometry<double> new_geom = round_trip(geom);
    std::string wkt;
    CHECK( mapnik::util::to_wkt(wkt, new_geom) );
    CHECK( wkt == "LINESTRING(128 -128,192.001 0)" );
    CHECK( !mapnik::geometry::is_empty(new_geom) );
    CHECK( new_geom.is<mapnik::geometry::line_string<double> >() );
}

TEST_CASE( "vector tile multi_line_string encoding of single line_string", "should create vector tile with data" ) {
    mapnik::geometry::multi_line_string<double> geom;
    mapnik::geometry::line_string<double> line;
    line.add_coord(0,0);
    line.add_coord(100,100);
    geom.emplace_back(std::move(line));
    mapnik::geometry::geometry<double> new_geom = round_trip(geom);
    std::string wkt;
    CHECK( mapnik::util::to_wkt(wkt, new_geom) );
    CHECK( wkt == "LINESTRING(128 -128,192.001 0)" );
    CHECK( !mapnik::geometry::is_empty(new_geom) );
    CHECK( new_geom.is<mapnik::geometry::line_string<double> >() );
}

TEST_CASE( "vector tile multi_line_string encoding of actual multi_line_string", "should create vector tile with data" ) {
    mapnik::geometry::multi_line_string<double> geom;
    mapnik::geometry::line_string<double> line;
    line.add_coord(0,0);
    line.add_coord(100,100);
    geom.emplace_back(std::move(line));
    mapnik::geometry::line_string<double> line2;
    line2.add_coord(-10,-0);
    line2.add_coord(-100,-100);
    geom.emplace_back(std::move(line2));
    mapnik::geometry::geometry<double> new_geom = round_trip(geom);
    std::string wkt;
    CHECK( mapnik::util::to_wkt(wkt, new_geom) );
    CHECK( wkt == "MULTILINESTRING((128 -128,192.001 0),(120.889 -128,63.288 -256))" );
    CHECK( !mapnik::geometry::is_empty(new_geom) );
    CHECK( new_geom.is<mapnik::geometry::multi_line_string<double> >() );
}


TEST_CASE( "vector tile polygon encoding", "should create vector tile with data" ) {
    mapnik::geometry::polygon<double> geom;
    geom.exterior_ring.add_coord(0,0);
    geom.exterior_ring.add_coord(0,10);
    geom.exterior_ring.add_coord(-10,10);
    geom.exterior_ring.add_coord(-10,0);
    geom.exterior_ring.add_coord(0,0);
    mapnik::geometry::geometry<double> new_geom = round_trip(geom);
    CHECK( !mapnik::geometry::is_empty(new_geom) );
    CHECK( new_geom.is<mapnik::geometry::polygon<double> >() );
    std::string wkt;
    CHECK( mapnik::util::to_wkt(wkt, new_geom) );
    CHECK( wkt == "POLYGON((128 -113.778,120.889 -113.778,120.889 -128,128 -128,128 -113.778))" );
}


TEST_CASE( "vector tile multi_polygon encoding of single polygon", "should create vector tile with data" ) {
    mapnik::geometry::polygon<double> poly;
    poly.exterior_ring.add_coord(0,0);
    poly.exterior_ring.add_coord(0,10);
    poly.exterior_ring.add_coord(-10,10);
    poly.exterior_ring.add_coord(-10,0);
    poly.exterior_ring.add_coord(0,0);
    mapnik::geometry::multi_polygon<double> geom;
    geom.emplace_back(std::move(poly));
    mapnik::geometry::geometry<double> new_geom = round_trip(geom);
    std::string wkt;
    CHECK( mapnik::util::to_wkt(wkt, new_geom) );
    CHECK( wkt == "POLYGON((128 -113.778,120.889 -113.778,120.889 -128,128 -128,128 -113.778))" );
    CHECK( !mapnik::geometry::is_empty(new_geom) );
    CHECK( new_geom.is<mapnik::geometry::polygon<double> >() );
}

TEST_CASE( "vector tile multi_polygon encoding of actual multi_polygon", "should create vector tile with data a multi polygon" ) {
    mapnik::geometry::multi_polygon<double> geom;
    mapnik::geometry::polygon<double> poly;
    poly.exterior_ring.add_coord(0,0);
    poly.exterior_ring.add_coord(0,10);
    poly.exterior_ring.add_coord(-10,10);
    poly.exterior_ring.add_coord(-10,0);
    poly.exterior_ring.add_coord(0,0);
    /*
    // This is an interior ring that touches nothing.
    poly.interior_rings.emplace_back();
    poly.interior_rings.back().add_coord(-1,1);
    poly.interior_rings.back().add_coord(-1,2);
    poly.interior_rings.back().add_coord(-2,2);
    poly.interior_rings.back().add_coord(-2,1);
    poly.interior_rings.back().add_coord(-1,1);
    // This is an interior ring that touches exterior edge.
    poly.interior_rings.emplace_back();
    poly.interior_rings.back().add_coord(-10,7);
    poly.interior_rings.back().add_coord(-10,5);
    poly.interior_rings.back().add_coord(-8,5);
    poly.interior_rings.back().add_coord(-8,7);
    poly.interior_rings.back().add_coord(-10,7);
    */
    geom.emplace_back(std::move(poly));
    mapnik::geometry::polygon<double> poly2;
    poly2.exterior_ring.add_coord(11,11);
    poly2.exterior_ring.add_coord(11,21);
    poly2.exterior_ring.add_coord(1,21);
    poly2.exterior_ring.add_coord(1,11);
    poly2.exterior_ring.add_coord(11,11);
    geom.emplace_back(std::move(poly2));
    mapnik::geometry::geometry<double> new_geom = round_trip(geom);
    CHECK( !mapnik::geometry::is_empty(new_geom) );
    CHECK( new_geom.is<mapnik::geometry::multi_polygon<double> >() );
}

// simplification

TEST_CASE( "vector tile point correctly passed through simplification code path", "should create vector tile with data" ) {
    mapnik::geometry::point<double> geom(-122,48);
    mapnik::geometry::geometry<double> new_geom = round_trip(geom,500);
    std::string wkt;
    CHECK( mapnik::util::to_wkt(wkt, new_geom) );
    CHECK( wkt == "POINT(41.244 -59.733)" );
    CHECK( !mapnik::geometry::is_empty(new_geom) );
    CHECK( new_geom.is<mapnik::geometry::point<double> >() );
}

TEST_CASE( "vector tile line_string is simplified", "should create vector tile with data" ) {
    mapnik::geometry::multi_line_string<double> geom;
    mapnik::geometry::line_string<double> line;
    line.add_coord(0,0);
    line.add_coord(1,1);
    line.add_coord(2,2);
    line.add_coord(100,100);
    geom.emplace_back(std::move(line));
    mapnik::geometry::geometry<double> new_geom = round_trip(geom,500);
    std::string wkt;
    CHECK( mapnik::util::to_wkt(wkt, new_geom) );
    CHECK( wkt == "LINESTRING(128 -128,192.001 0)" );
    CHECK( !mapnik::geometry::is_empty(new_geom) );
    CHECK( new_geom.is<mapnik::geometry::line_string<double> >() );
    auto const& line2 = mapnik::util::get<mapnik::geometry::line_string<double> >(new_geom);
    CHECK( line2.size() == 2 );
}

TEST_CASE( "vector tile line_string is simplified when outside bounds", "should create vector tile with data" ) {
    mapnik::geometry::multi_line_string<double> geom;
    mapnik::geometry::line_string<double> line;
    line.add_coord(-10000,0);
    line.add_coord(-10000.1,0);
    line.add_coord(100000,0);
    geom.emplace_back(std::move(line));
    mapnik::geometry::geometry<double> new_geom = round_trip(geom,100);
    std::string wkt;
    CHECK( mapnik::util::to_wkt(wkt, new_geom) );
    // yep this test is weird - more of a fuzz than anything
    CHECK( wkt == "LINESTRING(-7369.526 -128,-7113.526 -128)" );
    CHECK( !mapnik::geometry::is_empty(new_geom) );
    CHECK( new_geom.is<mapnik::geometry::line_string<double> >() );
    auto const& line2 = mapnik::util::get<mapnik::geometry::line_string<double> >(new_geom);
    CHECK( line2.size() == 2 );
}

/*
TEST_CASE( "vector tile from simplified geojson", "should create vector tile with data" ) {
    typedef mapnik::vector_tile_impl::backend_pbf backend_type;
    typedef mapnik::vector_tile_impl::processor<backend_type> renderer_type;
    typedef vector_tile::Tile tile_type;
    tile_type tile;
    backend_type backend(tile,1000);
    unsigned tile_size = 256;
    mapnik::box2d<double> bbox(-20037508.342789,-20037508.342789,20037508.342789,20037508.342789);
    mapnik::Map map(tile_size,tile_size,"+init=epsg:3857");
    mapnik::layer lyr("layer","+init=epsg:4326");
    // create a datasource with a feature outside the map
    std::shared_ptr<mapnik::memory_datasource> ds = testing::build_geojson_ds("./test/data/poly.geojson");
    // but fake the overall envelope to ensure the layer is still processed
    // and then removed given no intersecting features will be added
    ds->set_envelope(mapnik::box2d<double>(160.147311,11.047284,160.662858,11.423830));
    lyr.set_datasource(ds);
    map.add_layer(lyr);
    map.zoom_to_box(bbox);
    mapnik::request m_req(tile_size,tile_size,bbox);
    renderer_type ren(backend,map,m_req);
    ren.apply();
    CHECK(1 == tile.layers_size());
    vector_tile::Tile_Layer const& layer = tile.layers(0);
    CHECK(std::string("layer") == layer.name());
    CHECK(1 == layer.features_size());
    vector_tile::Tile_Feature const& f = layer.features(0);
    unsigned z = 0;
    unsigned x = 0;
    unsigned y = 0;
    double resolution = mapnik::EARTH_CIRCUMFERENCE/(1 << z);
    double tile_x = -0.5 * mapnik::EARTH_CIRCUMFERENCE + x * resolution;
    double tile_y =  0.5 * mapnik::EARTH_CIRCUMFERENCE - y * resolution;
    double scale = (static_cast<double>(layer.extent()) / tile_size) * tile_size/resolution;
    auto geom = mapnik::vector_tile_impl::decode_geometry(f,tile_x,tile_y,scale,-1*scale);

    unsigned int n_err = 0;
    mapnik::projection wgs84("+init=epsg:4326",true);
    mapnik::projection merc("+init=epsg:3857",true);
    mapnik::proj_transform prj_trans(merc,wgs84);
    mapnik::geometry::geometry projected_geom = mapnik::reproject(geom,prj_trans,n_err);
    //if (n_err > 0) return false;
    std::string geojson_string;
    CHECK( mapnik::util::to_geojson(geojson_string,projected_geom) );
    //std::clog << geojson_string << "\n";
}
*/

mapnik::geometry::geometry<double> round_trip2(mapnik::geometry::geometry<double> const& geom,
                                      double simplify_distance=0.0)
{
    typedef mapnik::vector_tile_impl::backend_pbf backend_type;
    typedef mapnik::vector_tile_impl::processor<backend_type> renderer_type;
    typedef vector_tile::Tile tile_type;
    tile_type tile;
    backend_type backend(tile,160);
    unsigned tile_size = 256;
    mapnik::projection wgs84("+init=epsg:4326",true);
    mapnik::projection merc("+init=epsg:3857",true);
    mapnik::proj_transform prj_trans(merc,wgs84);
    mapnik::box2d<double> bbox(0,0,11.25,11.178401873711785);
    prj_trans.backward(bbox);
    mapnik::Map map(tile_size,tile_size,"+init=epsg:3857");
    mapnik::request m_req(tile_size,tile_size,bbox);
    renderer_type ren(backend,map,m_req,1,0,0,0);
    // instead of calling apply, let's cheat and test `handle_geometry` directly by adding features
    backend.start_tile_layer("layer");
    mapnik::context_ptr ctx = std::make_shared<mapnik::context_type>();
    mapnik::feature_ptr feature(mapnik::feature_factory::create(ctx,1));
    ren.set_simplify_distance(simplify_distance);
    ren.handle_geometry(*feature,geom,prj_trans,bbox);
    backend.stop_tile_layer();
    if (tile.layers_size() != 1)
    {
        throw std::runtime_error("expected 1 layer in `round_trip`");
    }
    vector_tile::Tile_Layer const& layer = tile.layers(0);
    if (layer.features_size() != 1)
    {
        throw std::runtime_error("expected 1 feature in `round_trip`");
    }
    vector_tile::Tile_Feature const& f = layer.features(0);
    unsigned z = 5;
    unsigned x = 16;
    unsigned y = 15;
    double resolution = mapnik::EARTH_CIRCUMFERENCE/(1 << z);
    double tile_x = -0.5 * mapnik::EARTH_CIRCUMFERENCE + x * resolution;
    double tile_y =  0.5 * mapnik::EARTH_CIRCUMFERENCE - y * resolution;
    double scale = (static_cast<double>(layer.extent()) / tile_size) * tile_size/resolution;
    return mapnik::vector_tile_impl::decode_geometry(f,tile_x,tile_y,scale,-1*scale);
}

TEST_CASE( "vector tile line_string is verify direction", "should line string with proper directions" ) {
    mapnik::geometry::line_string<double> line;
    line.add_coord(-20,2);
    line.add_coord(2,2);
    line.add_coord(2,-20);
    line.add_coord(8,-20);
    line.add_coord(8,2);
    line.add_coord(60,2);
    line.add_coord(60,8);
    line.add_coord(8,8);
    line.add_coord(8,60);
    line.add_coord(2,60);
    line.add_coord(2,8);
    line.add_coord(-20,8);

    mapnik::geometry::geometry<double> new_geom = round_trip2(line);
    CHECK( !mapnik::geometry::is_empty(new_geom) );
    CHECK( new_geom.is<mapnik::geometry::multi_line_string<double> >() );
    mapnik::projection wgs84("+init=epsg:4326",true);
    mapnik::projection merc("+init=epsg:3857",true);
    mapnik::proj_transform prj_trans(merc,wgs84);
    mapnik::proj_strategy proj_strat(prj_trans);
    mapnik::geometry::geometry<double> xgeom = mapnik::geometry::transform<double>(new_geom, proj_strat);
    std::string foo;
    mapnik::util::to_wkt(foo, xgeom);
    std::clog << foo << std::endl;
    //auto const& line2 = mapnik::util::get<mapnik::geometry::line_string<double> >(new_geom);
    //CHECK( line2.size() == 2 );
}
