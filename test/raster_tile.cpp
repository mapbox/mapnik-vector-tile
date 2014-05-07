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
#include "vector_tile_datasource.hpp"


#include <mapnik/graphics.hpp>
#include <mapnik/datasource_cache.hpp>

TEST_CASE( "vector tile output 1", "should create vector tile with one point" ) {
    mapnik::datasource_cache::instance().register_datasources("/opt/mapnik-3.x/lib/mapnik/input/");
    typedef mapnik::vector::backend_pbf backend_type;
    typedef mapnik::vector::processor<backend_type> renderer_type;
    typedef mapnik::vector::tile tile_type;
    tile_type tile;
    backend_type backend(tile,16);
    mapnik::Map map(tile_size,tile_size,"+init=epsg:3857");
    mapnik::layer lyr("layer",map.srs());
    mapnik::parameters params;
    params["type"] = "gdal";
    // created with:
    // wget http://www.nacis.org/naturalearth/50m/raster/NE2_50m_SR_W.zip
    // gdalwarp -t_srs EPSG:3857 -ts 1048 1048 -r bilinear NE2_50M_SR_W.tif natural_earth.tif
    params["file"] = "test/natural_earth.tif";
    MAPNIK_SHARED_PTR<mapnik::datasource> ds =
        mapnik::datasource_cache::instance().create(params);
    lyr.set_datasource(ds);
    map.MAPNIK_ADD_LAYER(lyr);
    mapnik::request m_req(tile_size,tile_size,bbox);
    renderer_type ren(backend,map,m_req);
    ren.apply();
    CHECK(1 == tile.layers_size());
    mapnik::vector::tile_layer const& layer = tile.layers(0);
    CHECK(std::string("layer") == layer.name());
    CHECK(1 == layer.features_size());
    mapnik::vector::tile_feature const& f = layer.features(0);
    CHECK(static_cast<mapnik::value_integer>(1) == static_cast<mapnik::value_integer>(f.id()));
    CHECK(0 == f.geometry_size());
    CHECK(f.has_raster());
    std::string const& ras_buffer = f.raster();
    CHECK(!ras_buffer.empty());
    CHECK(44359 == ras_buffer.size());
    /*
    std::ofstream file("out.png", std::ios::out|std::ios::trunc|std::ios::binary);
    file << ras_buffer;
    file.close();
    */
    CHECK(44385 == tile.ByteSize());
    std::string buffer;
    CHECK(tile.SerializeToString(&buffer));
    CHECK(44385 == buffer.size());
    // now read back and render image
    mapnik::Map map2(tile_size,tile_size,"+init=epsg:3857");
    tile_type tile2;
    CHECK(tile2.ParseFromString(buffer));
    CHECK(1 == tile2.layers_size());
    mapnik::vector::tile_layer const& layer2 = tile2.layers(0);
    CHECK(std::string("layer") == layer2.name());
    CHECK(1 == layer2.features_size());
    mapnik::vector::tile_feature const& f2 = layer2.features(0);
    CHECK(static_cast<mapnik::value_integer>(1) == static_cast<mapnik::value_integer>(f2.id()));
    CHECK(0 == f2.geometry_size());
    CHECK(f2.has_raster());
    CHECK(!f2.raster().empty());
    CHECK(44359 == f2.raster().size());

    mapnik::layer lyr2("layer",map2.srs());
    MAPNIK_SHARED_PTR<mapnik::vector::tile_datasource> ds2 = MAPNIK_MAKE_SHARED<
                                    mapnik::vector::tile_datasource>(
                                        layer2,_x,_y,_z,map2.width());
    ds2->set_envelope(bbox);
    lyr2.set_datasource(ds2);
    lyr2.add_style("style");
    map2.MAPNIK_ADD_LAYER(lyr2);
    mapnik::load_map(map2,"test/raster_style.xml");
    map2.zoom_to_box(bbox);
    mapnik::image_32 im(map2.width(),map2.height());
    mapnik::agg_renderer<mapnik::image_32> ren2(map2,im);
    ren2.apply();
    //mapnik::save_to_file(im,"image.png");
    unsigned rgba = im.data()(128,128);
    CHECK(0xffc29c66 == rgba);
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
