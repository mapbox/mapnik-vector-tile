// https://github.com/philsquared/Catch/wiki/Supplying-your-own-main()
#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

// test utils
#include "test_utils.hpp"

#include "vector_tile_projection.hpp"

const unsigned x=0,y=0,z=0;
const unsigned tile_size = 256;
mapnik::box2d<double> bbox;

// vector output api
#include "vector_tile_processor.hpp"
#include "vector_tile_backend_pbf.hpp"

TEST_CASE( "vector tile projection 1", "should support z/x/y to bbox conversion at 0/0/0" ) {
    mapnik::vector::spherical_mercator merc(256);
    int x = 0;
    int y = 0;
    int z = 0;
    double minx,miny,maxx,maxy;
    merc.xyz(x,y,z,minx,miny,maxx,maxy);
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

TEST_CASE( "vector tile output", "should create vector tile with one point" ) {
    typedef mapnik::vector::backend_pbf backend_type;
    typedef mapnik::vector::processor<backend_type> renderer_type;
    typedef mapnik::vector::tile tile_type;
    tile_type tile;
    backend_type backend(tile,16);
    mapnik::Map map(tile_size,tile_size);
    mapnik::layer lyr("layer");
    lyr.set_datasource(build_ds());
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
    lyr.set_datasource(build_ds());
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
                                        layer2,x,y,z,map2.width());
    ds->set_envelope(bbox);
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
    lyr.set_datasource(build_ds());
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
    mapnik::vector::tile_datasource ds(layer,x,y,z,tile_size);
    mapnik::featureset_ptr fs;

    // ensure we can query single feature
    fs = ds.features(mapnik::query(bbox));
    CHECK(fs->next() != mapnik::feature_ptr());
    fs = ds.features(mapnik::query(mapnik::box2d<double>(-1,-1,1,1)));
    CHECK(fs->next() != mapnik::feature_ptr());

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

int main (int argc, char* const argv[])
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    // set up bbox
    double minx,miny,maxx,maxy;
    mapnik::vector::spherical_mercator merc(256);
    merc.xyz(x,y,z,minx,miny,maxx,maxy);
    bbox.init(minx,miny,maxx,maxy);
    int result = Catch::Main( argc, argv );
    if (!result) printf("\x1b[1;32m ✓ \x1b[0m\n");
    google::protobuf::ShutdownProtobufLibrary();
    return result;
}
