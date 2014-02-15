// https://github.com/philsquared/Catch/wiki/Supplying-your-own-main()
#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

// test utils
#include "test_utils.hpp"

#include "vector_tile_projection.hpp"

const unsigned _x=0,_y=0,_z=0;
const unsigned tile_size = 256;
mapnik::box2d<double> bbox;

// vector output api
#include "vector_tile_processor.hpp"
#include "vector_tile_backend_pbf.hpp"
#include "vector_tile_util.hpp"

TEST_CASE( "vector tile projection 1", "should support z/x/y to bbox conversion at 0/0/0" ) {
    mapnik::vector::spherical_mercator merc(256);
    double minx,miny,maxx,maxy;
    merc.xyz(_x,_y,_z,minx,miny,maxx,maxy);
    mapnik::box2d<double> map_extent(minx,miny,maxx,maxy);
    mapnik::box2d<double> e(-20037508.342789,-20037508.342789,20037508.342789,20037508.342789);
    double epsilon = 0.000001;
    CHECK(std::fabs(map_extent.minx() - e.minx()) < epsilon);
    CHECK(std::fabs(map_extent.miny() - e.miny()) < epsilon);
    CHECK(std::fabs(map_extent.maxx() - e.maxx()) < epsilon);
    CHECK(std::fabs(map_extent.maxy() - e.maxy()) < epsilon);
}

TEST_CASE( "vector tile projection 2", "should support z/x/y to bbox conversion up to z33" ) {
    mapnik::vector::spherical_mercator merc(256);
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

TEST_CASE( "vector tile output 1", "should create vector tile with one point" ) {
    typedef mapnik::vector::backend_pbf backend_type;
    typedef mapnik::vector::processor<backend_type> renderer_type;
    typedef mapnik::vector::tile tile_type;
    tile_type tile;
    backend_type backend(tile,16);
    mapnik::Map map(tile_size,tile_size);
    mapnik::layer lyr("layer");
    lyr.set_datasource(build_ds(0,0));
    map.addLayer(lyr);
    mapnik::request m_req(tile_size,tile_size,bbox);
    renderer_type ren(backend,map,m_req);
    ren.apply();
    CHECK(1 == tile.layers_size());
    mapnik::vector::tile_layer const& layer = tile.layers(0);
    CHECK(std::string("layer") == layer.name());
    CHECK(1 == layer.features_size());
    mapnik::vector::tile_feature const& f = layer.features(0);
    CHECK(static_cast<mapnik::value_integer>(1) == static_cast<mapnik::value_integer>(f.id()));
    CHECK(3 == f.geometry_size());
    CHECK(9 == f.geometry(0));
    CHECK(4096 == f.geometry(1));
    CHECK(4096 == f.geometry(2));
    CHECK(52 == tile.ByteSize());
    std::string buffer;
    CHECK(tile.SerializeToString(&buffer));
    CHECK(52 == buffer.size());
}

TEST_CASE( "vector tile output 2", "adding empty layers should result in empty tile" ) {
    typedef mapnik::vector::backend_pbf backend_type;
    typedef mapnik::vector::processor<backend_type> renderer_type;
    typedef mapnik::vector::tile tile_type;
    tile_type tile;
    backend_type backend(tile,16);
    mapnik::Map map(tile_size,tile_size);
    map.addLayer(mapnik::layer("layer"));
    map.zoom_to_box(bbox);
    mapnik::request m_req(tile_size,tile_size,bbox);
    renderer_type ren(backend,map,m_req);
    ren.apply();
    CHECK(0 == tile.layers_size());
}

TEST_CASE( "vector tile output 3", "adding layers with geometries outside rendering extent should not add layer" ) {
    typedef mapnik::vector::backend_pbf backend_type;
    typedef mapnik::vector::processor<backend_type> renderer_type;
    typedef mapnik::vector::tile tile_type;
    tile_type tile;
    backend_type backend(tile,16);
    mapnik::Map map(tile_size,tile_size);
    mapnik::layer lyr("layer");
    mapnik::context_ptr ctx = boost::make_shared<mapnik::context_type>();
    mapnik::feature_ptr feature(mapnik::feature_factory::create(ctx,1));
    std::auto_ptr<mapnik::geometry_type> g(new mapnik::geometry_type(mapnik::LineString));
    g->move_to(0,0);
    g->line_to(1,1);
    feature->add_geometry(g.release());
    boost::shared_ptr<mapnik::memory_datasource> ds = boost::make_shared<mapnik::memory_datasource>();
    ds->push(feature);
    lyr.set_datasource(ds);
    map.addLayer(lyr);
    map.zoom_to_box(bbox);
    mapnik::request m_req(tile_size,tile_size,bbox);
    renderer_type ren(backend,map,m_req);
    ren.apply();
    CHECK(1 == tile.layers_size());
}

TEST_CASE( "vector tile output 4", "adding layers with degenerate geometries should not add layer" ) {
    typedef mapnik::vector::backend_pbf backend_type;
    typedef mapnik::vector::processor<backend_type> renderer_type;
    typedef mapnik::vector::tile tile_type;
    tile_type tile;
    backend_type backend(tile,16);
    mapnik::Map map(tile_size,tile_size);
    mapnik::layer lyr("layer");
    // create a datasource with a feature outside the map
    boost::shared_ptr<mapnik::memory_datasource> ds = build_ds(bbox.minx()-1,bbox.miny()-1);
    // but fake the overall envelope to ensure the layer is still processed
    // and then removed given no intersecting features will be added
    ds->set_envelope(bbox);
    lyr.set_datasource(ds);
    map.addLayer(lyr);
    map.zoom_to_box(bbox);
    mapnik::request m_req(tile_size,tile_size,bbox);
    renderer_type ren(backend,map,m_req);
    ren.apply();
    CHECK(0 == tile.layers_size());
}

// vector input api
#include "vector_tile_datasource.hpp"
#include "vector_tile_backend_pbf.hpp"

TEST_CASE( "vector tile input", "should be able to parse message and render point" ) {
    typedef mapnik::vector::backend_pbf backend_type;
    typedef mapnik::vector::processor<backend_type> renderer_type;
    typedef mapnik::vector::tile tile_type;
    tile_type tile;
    backend_type backend(tile,16);
    mapnik::Map map(tile_size,tile_size);
    mapnik::layer lyr("layer");
    lyr.set_datasource(build_ds(0,0));
    map.addLayer(lyr);
    map.zoom_to_box(bbox);
    mapnik::request m_req(map.width(),map.height(),map.get_current_extent());
    renderer_type ren(backend,map,m_req);
    ren.apply();
    // serialize to message
    std::string buffer;
    CHECK(tile.SerializeToString(&buffer));
    CHECK(52 == buffer.size());
    // now create new objects
    mapnik::Map map2(tile_size,tile_size);
    tile_type tile2;
    CHECK(tile2.ParseFromString(buffer));
    CHECK(1 == tile2.layers_size());
    mapnik::vector::tile_layer const& layer2 = tile2.layers(0);
    CHECK(std::string("layer") == layer2.name());
    mapnik::layer lyr2("layer");
    boost::shared_ptr<mapnik::vector::tile_datasource> ds = boost::make_shared<
                                    mapnik::vector::tile_datasource>(
                                        layer2,_x,_y,_z,map2.width());
    ds->set_envelope(bbox);
    mapnik::layer_descriptor lay_desc = ds->get_descriptor();
    std::vector<std::string> expected_names;
    expected_names.push_back("name");
    std::vector<std::string> names;
    BOOST_FOREACH(mapnik::attribute_descriptor const& desc, lay_desc.get_descriptors())
    {
        names.push_back(desc.get_name());
    }
    CHECK(names == expected_names);
    lyr2.set_datasource(ds);
    lyr2.add_style("style");
    map2.addLayer(lyr2);
    mapnik::feature_type_style s;
    mapnik::rule r;
    mapnik::color red(255,0,0);
    mapnik::markers_symbolizer sym;
    sym.set_fill(red);
    r.append(sym);
    s.add_rule(r);
    map2.insert_style("style",s);
    map2.zoom_to_box(bbox);
    mapnik::image_32 im(map2.width(),map2.height());
    mapnik::agg_renderer<mapnik::image_32> ren2(map2,im);
    ren2.apply();
    unsigned rgba = im.data()(128,128);
    CHECK(red.rgba() == rgba);
    //mapnik::save_to_file(im,"test.png");
}

TEST_CASE( "vector tile datasource", "should filter features outside extent" ) {
    typedef mapnik::vector::backend_pbf backend_type;
    typedef mapnik::vector::processor<backend_type> renderer_type;
    typedef mapnik::vector::tile tile_type;
    tile_type tile;
    backend_type backend(tile,16);
    mapnik::Map map(tile_size,tile_size);
    mapnik::layer lyr("layer");
    lyr.set_datasource(build_ds(0,0));
    map.addLayer(lyr);
    mapnik::request m_req(tile_size,tile_size,bbox);
    renderer_type ren(backend,map,m_req);
    ren.apply();
    CHECK(1 == tile.layers_size());
    mapnik::vector::tile_layer const& layer = tile.layers(0);
    CHECK(std::string("layer") == layer.name());
    CHECK(1 == layer.features_size());
    mapnik::vector::tile_feature const& f = layer.features(0);
    CHECK(static_cast<mapnik::value_integer>(1) == static_cast<mapnik::value_integer>(f.id()));
    CHECK(3 == f.geometry_size());
    CHECK(9 == f.geometry(0));
    CHECK(4096 == f.geometry(1));
    CHECK(4096 == f.geometry(2));
    // now actually start the meat of the test
    mapnik::vector::tile_datasource ds(layer,_x,_y,_z,tile_size);
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

// NOTE: encoding a multiple lines as one path is technically incorrect
// because in Mapnik the protocol is to split geometry parts into separate paths
// however this case should still be supported in error and its an optimization in the
// case where you know that lines do not need to be labeled in custom ways.
TEST_CASE( "encoding multi line as one path", "should maintain second move_to command" ) {
    // Options
    // here we use a multiplier of 1 to avoid rounding numbers
    // and stay in integer space for simplity
    unsigned path_multiplier = 1;
    // here we use an extreme tolerance to prove tht all vertices are maintained no matter
    // the tolerance because we never want to drop a move_to or the first line_to
    unsigned tolerance = 2000000;
    // now create the testing data
    mapnik::vector::tile tile;
    mapnik::vector::backend_pbf backend(tile,path_multiplier);
    backend.start_tile_layer("layer");
    mapnik::feature_ptr feature(mapnik::feature_factory::create(boost::make_shared<mapnik::context_type>(),1));
    backend.start_tile_feature(*feature);
    std::auto_ptr<mapnik::geometry_type> g(new mapnik::geometry_type(mapnik::LineString));
    g->move_to(0,0);        // takes 3 geoms: command length,x,y
    g->line_to(2,2);        // new command, so again takes 3 geoms: command length,x,y | total 6
    g->move_to(1,1);        // takes 3 geoms: command length,x,y
    g->line_to(2,2);        // new command, so again takes 3 geoms: command length,x,y | total 6
    backend.add_path(*g, tolerance, g->type());
    backend.stop_tile_feature();
    backend.stop_tile_layer();
    // done encoding single feature/geometry
    CHECK(1 == tile.layers_size());
    mapnik::vector::tile_layer const& layer = tile.layers(0);
    CHECK(1 == layer.features_size());
    mapnik::vector::tile_feature const& f = layer.features(0);
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
}

TEST_CASE( "encoding repeated move_to in degerate geometry", "should never drop move_to commands" ) {
    // Options
    // here we use a multiplier of 1 to avoid rounding numbers
    // and stay in integer space for simplity
    unsigned path_multiplier = 1;
    // here we use an extreme tolerance to prove tht all vertices are maintained no matter
    // the tolerance because we never want to drop a move_to or the first line_to
    unsigned tolerance = 2000000;
    // now create the testing data
    mapnik::vector::tile tile;
    mapnik::vector::backend_pbf backend(tile,path_multiplier);
    backend.start_tile_layer("layer");
    mapnik::feature_ptr feature(mapnik::feature_factory::create(boost::make_shared<mapnik::context_type>(),1));
    backend.start_tile_feature(*feature);
    std::auto_ptr<mapnik::geometry_type> g(new mapnik::geometry_type(mapnik::LineString));
    // simulate broken geometry, perhaps from clipping or simplification
    g->move_to(0,0);
    g->move_to(1,1); // we cannot drop this. Future: should we drop the previous bogus move_to?
    g->line_to(2,2);
    backend.add_path(*g, tolerance, g->type());
    backend.stop_tile_feature();
    backend.stop_tile_layer();
    // done encoding single feature/geometry
    CHECK(1 == tile.layers_size());
    mapnik::vector::tile_layer const& layer = tile.layers(0);
    CHECK(1 == layer.features_size());
    mapnik::vector::tile_feature const& f = layer.features(0);
    CHECK(8 == f.geometry_size());
    CHECK(17 == f.geometry(0)); // 2 move_to's (2 << 3) | (1 & ((1 << 3) -1))
    CHECK(0 == f.geometry(1)); // x:0
    CHECK(0 == f.geometry(2)); // y:0
    CHECK(2 == f.geometry(3)); // x:1
    CHECK(2 == f.geometry(4)); // y:1
    CHECK(10 == f.geometry(5)); // 1 line_to
    CHECK(2 == f.geometry(6)); // x:1
    CHECK(2 == f.geometry(7)); // y:1
}

TEST_CASE( "encoding single line 1", "should maintain start/end vertex" ) {
    // Options
    // here we use a multiplier of 1 to avoid rounding numbers
    // this works because we are staying in integer space for this test
    unsigned path_multiplier = 1;
    // here we use a tolerance of 2. Along with a multiplier of 1 this
    // says to discard any verticies that are not at least >= 2 different
    // in both the x and y from the previous vertex
    unsigned tolerance = 2;
    // now create the testing data
    mapnik::vector::tile tile;
    mapnik::vector::backend_pbf backend(tile,path_multiplier);
    backend.start_tile_layer("layer");
    mapnik::feature_ptr feature(mapnik::feature_factory::create(boost::make_shared<mapnik::context_type>(),1));
    backend.start_tile_feature(*feature);
    std::auto_ptr<mapnik::geometry_type> g(new mapnik::geometry_type(mapnik::LineString));
    g->move_to(0,0);        // takes 3 geoms: command length,x,y
    g->line_to(2,2);        // new command, so again takes 3 geoms: command length,x,y | total 6
    g->line_to(1000,1000);  // repeated line_to, so only takes 2 geoms: x,y | total 8
    g->line_to(1001,1001);  // should skip given tolerance of 2 | total 8
    g->line_to(1001,1001);  // should not skip given it is the endpoint, added 2 geoms | total 10
    backend.add_path(*g, tolerance, g->type());
    backend.stop_tile_feature();
    backend.stop_tile_layer();
    // done encoding single feature/geometry
    CHECK(1 == tile.layers_size());
    mapnik::vector::tile_layer const& layer = tile.layers(0);
    CHECK(1 == layer.features_size());
    mapnik::vector::tile_feature const& f = layer.features(0);
    // sequence of 10 geometries given tolerance of 2
    CHECK(10 == f.geometry_size());
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
    // in this case there should be 3 because one was skipped
    unsigned line_value = f.geometry(3);
    // (3      << 3) | (2       & ((1 << 3) -1)) == 26
    CHECK(26 == line_value);
    unsigned line_cmd = line_value & ((1 << 3) - 1);
    CHECK(2 == line_cmd);
    unsigned line_length = line_value >> 3;
    CHECK(3 == line_length);
    // 5th and 6th are the x,y of the first line_to command
    // due zigzag encoding the 2,2 should be 4,4
    // delta encoding has no impact since the previous coordinate was 0,0
    unsigned four = (2 << 1) ^ (2 >> 31);
    CHECK(4 == four);
    CHECK(4 == f.geometry(4));
    CHECK(4 == f.geometry(5));
    // 7th and 8th are x,y of the second line_to command
    // due to delta encoding 1000-2 becomes 998
    // zigzag encoded 998 becomes 1996
    CHECK(1996 == f.geometry(6));
    CHECK(1996 == f.geometry(7));
    // next 1001,1001 should have been skipped
    // so final line_to command of 1001,1001 after
    // delta encoding becomes 1 which zigzag encoded is 2
    CHECK(2 == f.geometry(8));
    CHECK(2 == f.geometry(9));
}

// testcase for avoiding error in is_solid_extent of
// "Unknown command type (is_solid_extent): 0"
// not yet clear if this test is correct
// ported from shapefile test in tilelive-bridge (a:should render a (1.0.1))
TEST_CASE( "encoding single line 2", "should maintain start/end vertex" ) {
    unsigned path_multiplier = 16;
    unsigned tolerance = 5;
    mapnik::vector::tile tile;
    mapnik::vector::backend_pbf backend(tile,path_multiplier);
    backend.start_tile_layer("layer");
    mapnik::feature_ptr feature(mapnik::feature_factory::create(boost::make_shared<mapnik::context_type>(),1));
    backend.start_tile_feature(*feature);
    std::auto_ptr<mapnik::geometry_type> g(new mapnik::geometry_type(mapnik::Polygon));
    g->move_to(168.267850,-24.576888);
    g->line_to(167.982618,-24.697145);
    g->line_to(168.114561,-24.783548);
    g->line_to(168.267850,-24.576888);
    g->line_to(168.267850,-24.576888);
    g->close_path();
    //g->push_vertex(256.000000,-0.00000, mapnik::SEG_CLOSE);
    // todo - why does shape_io result in on-zero close path x,y?
    backend.add_path(*g, tolerance, g->type());
    backend.stop_tile_feature();
    backend.stop_tile_layer();
    std::string key("test");
    is_solid_extent(tile,key); // should not throw!
    CHECK(1 == tile.layers_size());
    mapnik::vector::tile_layer const& layer = tile.layers(0);
    CHECK(1 == layer.features_size());
    mapnik::vector::tile_feature const& f = layer.features(0);
    CHECK(7 == f.geometry_size());
}

int main (int argc, char* const argv[])
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    // set up bbox
    double minx,miny,maxx,maxy;
    mapnik::vector::spherical_mercator merc(256);
    merc.xyz(_x,_y,_z,minx,miny,maxx,maxy);
    bbox.init(minx,miny,maxx,maxy);
    int result = Catch::Session().run( argc, argv );
    if (!result) printf("\x1b[1;32m ✓ \x1b[0m\n");
    google::protobuf::ShutdownProtobufLibrary();
    return result;
}
