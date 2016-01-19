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
#include <mapnik/util/fs.hpp>
#include <mapnik/util/geometry_to_geojson.hpp>
#include <mapnik/vertex_adapters.hpp>

// boost
#include <boost/optional/optional_io.hpp>

// mapnik-vector-tile
#include "vector_tile_processor.hpp"
#include "vector_tile_geometry_decoder.hpp"
#include "vector_tile_datasource.hpp"

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
    
    vector_tile::Tile tile = out_tile.get_tile();
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
    mapnik::vector_tile_impl::Geometry<double> geoms(f,tile_x, tile_y,scale,-1*scale);
    auto geom = mapnik::vector_tile_impl::decode_geometry(geoms,f.type(),2);

    unsigned int n_err = 0;
    mapnik::projection wgs84("+init=epsg:4326",true);
    mapnik::projection merc("+init=epsg:3857",true);
    mapnik::proj_transform prj_trans(merc,wgs84);
    mapnik::geometry::geometry<double> projected_geom = mapnik::geometry::reproject_copy(geom,prj_trans,n_err);
    CHECK( n_err == 0 );
    std::string geojson_string;
    CHECK( mapnik::util::to_geojson(geojson_string,projected_geom) );
    //std::clog << geojson_string << std::endl;
    CHECK( geojson_string == "{\"type\":\"Polygon\",\"coordinates\":[[[160.36171875,11.2997786224589],[160.3828125,11.3011576095711],[160.408125,11.3039155638972],[160.408125,11.2997786224589],[160.425,11.3094313929343],[160.41234375,11.3411453475587],[160.3996875,11.3301148056307],[160.40953125,11.3700984927314],[160.39265625,11.3618264654176],[160.396875,11.3797488877286],[160.4053125,11.3893989555911],[160.40953125,11.3866418267411],[160.419375,11.4004272036667],[160.41515625,11.4059411672242],[160.419375,11.4114550237293],[160.425,11.412833471123],[160.42359375,11.422482415387],[160.40671875,11.3976701817587],[160.396875,11.3935345987524],[160.39828125,11.4018057045896],[160.39265625,11.4004272036667],[160.38984375,11.3811274888866],[160.3940625,11.3838846711709],[160.3771875,11.3521754635814],[160.33921875,11.3590690696413],[160.35046875,11.3645838345287],[160.3575,11.3645838345287],[160.3575,11.3756130442004],[160.29421875,11.3507967223837],[160.2928125,11.3480392200086],[160.28859375,11.3480392200086],[160.295625,11.3287359579628],[160.26328125,11.3080524456288],[160.295625,11.1866791818427],[160.31671875,11.1811610026871],[160.318125,11.1770222993774],[160.31390625,11.1687447155658],[160.3125,11.1494294353899],[160.2703125,11.1107950268865],[160.2421875,11.1149346728405],[160.23796875,11.0997556838987],[160.25625,11.095615822671],[160.21828125,11.0735355725517],[160.21546875,11.0652550492086],[160.2084375,11.0762956949617],[160.20140625,11.0638749392263],[160.19015625,11.0528338254202],[160.18453125,11.0528338254202],[160.183125,11.0486933005675],[160.24640625,11.0583544343014],[160.26890625,11.0555941428523],[160.250625,11.0804358297701],[160.28015625,11.0942358558913],[160.295625,11.0845759059922],[160.2928125,11.0721555015877],[160.318125,11.0790557913426],[160.31953125,11.0942358558913],[160.33359375,11.1038954864431],[160.34484375,11.0900959164515],[160.35609375,11.1038954864431],[160.363125,11.0969957829326],[160.36453125,11.1052754075802],[160.36171875,11.1121749153987],[160.37578125,11.1149346728405],[160.39828125,11.1080352302834],[160.36734375,11.1756427184796],[160.48125,11.1852996469051],[160.48546875,11.1825405573266],[160.5121875,11.1852996469051],[160.5459375,11.1342522433585],[160.56421875,11.1301128717933],[160.55578125,11.1204541093718],[160.56140625,11.1135547973836],[160.588125,11.1314926688534],[160.62328125,11.1121749153987],[160.633125,11.1135547973836],[160.6471875,11.1025155587833],[160.64296875,11.1176944041669],[160.63734375,11.1190742600349],[160.62328125,11.1342522433585],[160.62046875,11.128733068196],[160.6078125,11.1480497233847],[160.61203125,11.1480497233847],[160.6134375,11.1563278971795],[160.5909375,11.1425308098987],[160.576875,11.1480497233847],[160.57828125,11.1452902797332],[160.57265625,11.1425308098987],[160.57125,11.1494294353899],[160.54875,11.1577075698847],[160.554375,11.179781441482],[160.54875,11.1770222993774],[160.5628125,11.2087508469621],[160.5234375,11.2059919808933],[160.52203125,11.2032330885061],[160.50515625,11.2184066708578],[160.49390625,11.2032330885061],[160.46296875,11.2046125379891],[160.46296875,11.201853632445],[160.4165625,11.2115096867066],[160.41796875,11.2211654184183],[160.39546875,11.2266828344767],[160.35609375,11.2225447823168],[160.35328125,11.2363380587922],[160.3659375,11.2473722050633],[160.351875,11.2915045605453],[160.32375,11.2721974885629],[160.32234375,11.2846093266964],[160.35328125,11.3080524456288],[160.351875,11.3149471157772],[160.3659375,11.3204627323768],[160.36171875,11.2997786224589]],[[160.38140625,11.3094313929343],[160.3828125,11.3011576095711],[160.37859375,11.3080524456288],[160.38140625,11.3094313929343]],[[160.3603125,11.1383915560672],[160.3603125,11.1439105480884],[160.363125,11.1425308098987],[160.3603125,11.1383915560672]],[[160.34625,11.1383915560672],[160.34203125,11.1480497233847],[160.35046875,11.1397713138873],[160.34625,11.1383915560672]]]}");
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
    
    vector_tile::Tile tile = out_tile.get_tile();

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
    std::shared_ptr<mapnik::vector_tile_impl::tile_datasource> ds2 = std::make_shared<
                                    mapnik::vector_tile_impl::tile_datasource>(
                                        layer2,0,0,0);
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
    
    vector_tile::Tile tile = out_tile.get_tile();

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
    std::shared_ptr<mapnik::vector_tile_impl::tile_datasource> ds2 = std::make_shared<
                                    mapnik::vector_tile_impl::tile_datasource>(
                                        layer2,0,0,0);
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
