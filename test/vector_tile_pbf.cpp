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
#include <mapnik/datasource_cache.hpp>

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

#include <string>
#include <fstream>
#include <streambuf>

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
    CHECK(151 == buffer.size());
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
    CHECK(ds->get_name() == "layer");
    ds->set_envelope(bbox);
    CHECK( ds->type() == mapnik::datasource::Vector );
    CHECK( ds->get_geometry_type() == mapnik::datasource_geometry_t::Collection );
    mapnik::layer_descriptor lay_desc = ds->get_descriptor();
    std::vector<std::string> expected_names;
    expected_names.push_back("bool");
    expected_names.push_back("boolf");
    expected_names.push_back("double");
    expected_names.push_back("float");
    expected_names.push_back("int");
    expected_names.push_back("name");
    expected_names.push_back("uint");
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
          f_ptr = fs->next();
          while (f_ptr != mapnik::feature_ptr()) {
              f_ptr = fs->next();
          }
      });
    }
}

TEST_CASE( "pbf vector tile from simplified geojson", "should create vector tile with data" ) {
    typedef mapnik::vector_tile_impl::backend_pbf backend_type;
    typedef mapnik::vector_tile_impl::processor<backend_type> renderer_type;
    typedef vector_tile::Tile tile_type;
    tile_type tile;
    backend_type backend(tile,1000);
    unsigned tile_size = 256;
    mapnik::box2d<double> bbox(-20037508.342789,-20037508.342789,20037508.342789,20037508.342789);
    mapnik::Map map(tile_size,tile_size,"+init=epsg:3857");
    mapnik::layer lyr("layer","+init=epsg:4326");
    std::shared_ptr<mapnik::memory_datasource> ds = testing::build_geojson_ds("./test/data/poly.geojson");
    ds->set_envelope(mapnik::box2d<double>(160.147311,11.047284,160.662858,11.423830));
    lyr.set_datasource(ds);
    map.add_layer(lyr);
    map.zoom_to_box(bbox);
    mapnik::request m_req(tile_size,tile_size,bbox);
    renderer_type ren(backend,map,m_req);
    ren.apply();
    CHECK( ren.painted() == true );
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

    std::string buffer;
    tile.SerializeToString(&buffer);
    mapbox::util::pbf pbf_tile(buffer.c_str(), buffer.size());
    pbf_tile.next();
    mapbox::util::pbf pbf_layer = pbf_tile.get_message();
    // Need to loop because they could be encoded in any order
    bool found = false;
    while (!found && pbf_layer.next()) {
      // But there will be only one in our tile, so we'll break out of loop once we find it
      if (pbf_layer.tag() == 2) {
          mapbox::util::pbf pbf_feature = pbf_layer.get_message();
          while (!found && pbf_feature.next()) {
              if (pbf_feature.tag() == 4) {
                  found = true;
                  std::pair< mapbox::util::pbf::const_uint32_iterator, mapbox::util::pbf::const_uint32_iterator > geom_itr = pbf_feature.packed_uint32();
                  mapnik::vector_tile_impl::GeometryPBF geoms(geom_itr, tile_x,tile_y,scale,-1*scale);
                  auto geom = mapnik::vector_tile_impl::decode_geometry(geoms, f.type());

                  unsigned int n_err = 0;
                  mapnik::projection wgs84("+init=epsg:4326",true);
                  mapnik::projection merc("+init=epsg:3857",true);
                  mapnik::proj_transform prj_trans(merc,wgs84);
                  mapnik::geometry::geometry<double> projected_geom = mapnik::geometry::reproject_copy(geom,prj_trans,n_err);
                  CHECK( n_err == 0 );
                  std::string geojson_string;
                  CHECK( mapnik::util::to_geojson(geojson_string,projected_geom) );
                  CHECK( geojson_string == "{\"type\":\"Polygon\",\"coordinates\":[[[160.42359375,11.422482415387],[160.40671875,11.3976701817587],[160.396875,11.3935345987523],[160.39828125,11.4018057045895],[160.39265625,11.4004272036667],[160.38984375,11.3811274888866],[160.3940625,11.3838846711709],[160.3771875,11.3521754635814],[160.33921875,11.3590690696413],[160.35046875,11.3645838345287],[160.3575,11.3645838345287],[160.3575,11.3756130442004],[160.28859375,11.3480392200085],[160.295625,11.3287359579627],[160.26328125,11.3080524456288],[160.295625,11.1866791818427],[160.31671875,11.1811610026871],[160.318125,11.1770222993774],[160.31390625,11.1687447155658],[160.3125,11.1494294353899],[160.2703125,11.1107950268865],[160.2421875,11.1149346728405],[160.23796875,11.0997556838987],[160.25625,11.095615822671],[160.21828125,11.0735355725517],[160.21546875,11.0652550492086],[160.2084375,11.0762956949617],[160.20140625,11.0638749392263],[160.19015625,11.0528338254201],[160.18453125,11.0528338254201],[160.183125,11.0486933005675],[160.24640625,11.0583544343014],[160.26890625,11.0555941428522],[160.250625,11.0804358297701],[160.28015625,11.0942358558912],[160.295625,11.0845759059922],[160.2928125,11.0721555015877],[160.318125,11.0790557913425],[160.31953125,11.0942358558912],[160.33359375,11.103895486443],[160.34484375,11.0900959164514],[160.35609375,11.103895486443],[160.363125,11.0969957829326],[160.36453125,11.1052754075802],[160.36171875,11.1121749153987],[160.37578125,11.1149346728405],[160.39828125,11.1080352302833],[160.36734375,11.1756427184796],[160.48125,11.1852996469051],[160.48546875,11.1825405573265],[160.5121875,11.1852996469051],[160.5459375,11.1342522433584],[160.56421875,11.1301128717933],[160.55578125,11.1204541093718],[160.56140625,11.1135547973836],[160.588125,11.1314926688533],[160.62328125,11.1121749153987],[160.633125,11.1135547973836],[160.6471875,11.1025155587832],[160.64296875,11.1176944041668],[160.63734375,11.1190742600349],[160.62328125,11.1342522433584],[160.62046875,11.1287330681959],[160.6078125,11.1480497233847],[160.61203125,11.1480497233847],[160.6134375,11.1563278971794],[160.5909375,11.1425308098987],[160.576875,11.1480497233847],[160.57125,11.1549482179223],[160.57125,11.1494294353899],[160.57828125,11.1452902797332],[160.57265625,11.1425308098987],[160.57125,11.1494294353899],[160.54875,11.1577075698847],[160.554375,11.1797814414819],[160.54875,11.1770222993774],[160.5628125,11.2087508469621],[160.5234375,11.2059919808932],[160.52203125,11.203233088506],[160.50515625,11.2184066708578],[160.49390625,11.203233088506],[160.46296875,11.2046125379891],[160.46296875,11.201853632445],[160.4165625,11.2115096867065],[160.41796875,11.2211654184182],[160.39546875,11.2266828344767],[160.35609375,11.2225447823168],[160.35328125,11.2363380587921],[160.3659375,11.2473722050632],[160.351875,11.2915045605452],[160.32375,11.2721974885629],[160.32234375,11.2846093266964],[160.35328125,11.3080524456288],[160.351875,11.3149471157772],[160.3659375,11.3204627323768],[160.36171875,11.2997786224589],[160.3828125,11.3011576095711],[160.37859375,11.3080524456288],[160.38140625,11.3094313929343],[160.3828125,11.3011576095711],[160.408125,11.3039155638971],[160.408125,11.2997786224589],[160.425,11.3094313929343],[160.41234375,11.3411453475586],[160.3996875,11.3301148056307],[160.40953125,11.3700984927314],[160.39265625,11.3618264654175],[160.396875,11.3797488877286],[160.4053125,11.3893989555911],[160.40953125,11.3866418267411],[160.419375,11.4004272036667],[160.41515625,11.4059411672241],[160.419375,11.4114550237293],[160.425,11.412833471123],[160.42359375,11.422482415387]],[[160.3603125,11.1439105480884],[160.363125,11.1425308098987],[160.3603125,11.1383915560672],[160.3603125,11.1439105480884]],[[160.34203125,11.1480497233847],[160.35046875,11.1397713138873],[160.34625,11.1383915560672],[160.34203125,11.1480497233847]]]}");
                  break;
              }
          }
      }
    }
    REQUIRE( found );
}


TEST_CASE( "pbf raster tile output", "should be able to overzoom raster" ) {
    mapnik::datasource_cache::instance().register_datasources(MAPNIK_PLUGINDIR);
    typedef vector_tile::Tile tile_type;
    tile_type tile;
    {
        typedef mapnik::vector_tile_impl::backend_pbf backend_type;
        typedef mapnik::vector_tile_impl::processor<backend_type> renderer_type;
        double minx,miny,maxx,maxy;
        mapnik::vector_tile_impl::spherical_mercator merc(256);
        merc.xyz(0,0,0,minx,miny,maxx,maxy);
        mapnik::box2d<double> bbox(minx,miny,maxx,maxy);
        backend_type backend(tile,16);
        mapnik::Map map(256,256,"+init=epsg:3857");
        map.set_buffer_size(1024);
        mapnik::layer lyr("layer",map.srs());
        mapnik::parameters params;
        params["type"] = "gdal";
        std::ostringstream s;
        s << std::fixed << std::setprecision(16)
          << bbox.minx() << ',' << bbox.miny() << ','
          << bbox.maxx() << ',' << bbox.maxy();
        params["extent"] = s.str();
        params["file"] = "test/data/256x256.png";
        std::shared_ptr<mapnik::datasource> ds =
            mapnik::datasource_cache::instance().create(params);
        lyr.set_datasource(ds);
        map.add_layer(lyr);
        mapnik::request m_req(256,256,bbox);
        m_req.set_buffer_size(map.buffer_size());
        renderer_type ren(backend,map,m_req,1.0,0,0,1,"jpeg",mapnik::SCALING_BILINEAR);
        ren.apply();
    }
    // Done creating test data, now test created tile
    CHECK(1 == tile.layers_size());
    vector_tile::Tile_Layer const& layer = tile.layers(0);
    CHECK(std::string("layer") == layer.name());
    CHECK(1 == layer.features_size());
    vector_tile::Tile_Feature const& f = layer.features(0);
    CHECK(static_cast<mapnik::value_integer>(1) == static_cast<mapnik::value_integer>(f.id()));
    CHECK(0 == f.geometry_size());
    CHECK(f.has_raster());
    std::string const& ras_buffer = f.raster();
    CHECK(!ras_buffer.empty());

    // confirm tile looks correct as encoded
    std::string buffer;
    CHECK(tile.SerializeToString(&buffer));

    mapbox::util::pbf pbf_tile(buffer.c_str(), buffer.size());
    pbf_tile.next();
    mapbox::util::pbf layer2 = pbf_tile.get_message();

    // now read back and render image at larger size
    // and zoomed in
    double minx,miny,maxx,maxy;
    mapnik::vector_tile_impl::spherical_mercator merc(256);
    // 2/0/1.png
    merc.xyz(0,1,2,minx,miny,maxx,maxy);
    mapnik::box2d<double> bbox(minx,miny,maxx,maxy);
    mapnik::Map map2(256,256,"+init=epsg:3857");
    map2.set_buffer_size(1024);
    mapnik::layer lyr2("layer",map2.srs());
    std::shared_ptr<mapnik::vector_tile_impl::tile_datasource_pbf> ds2 = std::make_shared<
                                    mapnik::vector_tile_impl::tile_datasource_pbf>(
                                        layer2,0,0,0,256);
    lyr2.set_datasource(ds2);
    lyr2.add_style("style");
    map2.add_layer(lyr2);
    mapnik::load_map(map2,"test/data/raster_style.xml");
    map2.zoom_to_box(bbox);
    mapnik::image_rgba8 im(map2.width(),map2.height());
    mapnik::agg_renderer<mapnik::image_rgba8> ren2(map2,im);
    ren2.apply();
    if (!mapnik::util::exists("test/fixtures/expected-3.png")) {
        mapnik::save_to_file(im,"test/fixtures/expected-3.png","png32");
    }
    unsigned diff = testing::compare_images(im,"test/fixtures/expected-3.png");
    CHECK(0 == diff);
    if (diff > 0) {
        mapnik::save_to_file(im,"test/fixtures/actual-3.png","png32");
    }
}

TEST_CASE("Check that we throw on various valid-but-we-don't-handle PBF encoded files","Should be throwing exceptions")
{
    std::vector<std::string> filenames = {"test/data/tile_with_extra_feature_field.pbf",
                                          "test/data/tile_with_extra_layer_fields.pbf",
                                          "test/data/tile_with_invalid_layer_value_type.pbf",
                                          "test/data/tile_with_unexpected_geomtype.pbf"};

    for (auto const& f : filenames) {

      CHECK_THROWS({
        std::ifstream t(f);
        std::string buffer((std::istreambuf_iterator<char>(t)),
                            std::istreambuf_iterator<char>());

        mapnik::box2d<double> bbox(-20037508.342789,-20037508.342789,20037508.342789,20037508.342789);
        unsigned tile_size = 256;
          mapbox::util::pbf pbf_tile(buffer.c_str(), buffer.size());
          pbf_tile.next();
          mapbox::util::pbf layer2 = pbf_tile.get_message();
          mapnik::vector_tile_impl::tile_datasource_pbf ds(layer2,0,0,0,tile_size);
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
