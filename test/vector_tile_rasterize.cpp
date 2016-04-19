#include "catch.hpp"

// test utils
#include "test_utils.hpp"
#include <mapnik/util/fs.hpp>
#include <mapnik/agg_renderer.hpp>
#include <mapnik/load_map.hpp>
#include <mapnik/image_util.hpp>
#include <mapnik/geometry.hpp>
#include <mapnik/datasource_cache.hpp>

// vector output api
#include "vector_tile_compression.hpp"
#include "vector_tile_processor.hpp"
#include "vector_tile_strategy.hpp"
#include "vector_tile_projection.hpp"
#include "vector_tile_geometry_decoder.hpp"
#include "vector_tile_datasource_pbf.hpp"
#include "protozero/pbf_reader.hpp"

// boost
#include <boost/optional/optional_io.hpp>

// std
#include <fstream>

// libprotobuf
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include "vector_tile.pb.h"
#pragma GCC diagnostic pop

TEST_CASE("vector tile rasterize -- should try to decode windfail tile")
{
    // open vtile
    std::ifstream stream("./test/data/0.0.0.vector.mvt",std::ios_base::in|std::ios_base::binary);
    REQUIRE(stream.is_open());
    std::string buffer(std::istreambuf_iterator<char>(stream.rdbuf()),(std::istreambuf_iterator<char>()));
    REQUIRE(buffer.size() == 3812);

    // uncompress gzip data
    std::string uncompressed;
    mapnik::vector_tile_impl::zlib_decompress(buffer,uncompressed);
    REQUIRE(uncompressed.size() == 4934);

    mapnik::box2d<double> bbox(-20037508.342789,-20037508.342789,20037508.342789,20037508.342789);

    // first we render the raw tile directly to an image
    {
        mapnik::Map map(256,256,"+init=epsg:3857");
        vector_tile::Tile tile2;
        CHECK(tile2.ParseFromString(uncompressed));

        CHECK(1 == tile2.layers_size());
        vector_tile::Tile_Layer const& layer2 = tile2.layers(0);
        CHECK(std::string("water") == layer2.name());
        CHECK(1 == layer2.version());
        CHECK(23 == layer2.features_size());

        protozero::pbf_reader tile_reader(uncompressed);
        REQUIRE(tile_reader.next(mapnik::vector_tile_impl::Tile_Encoding::LAYERS));
        protozero::pbf_reader layer_reader = tile_reader.get_message();
        mapnik::layer lyr2("water",map.srs());
        std::shared_ptr<mapnik::vector_tile_impl::tile_datasource_pbf> ds = std::make_shared<
                                        mapnik::vector_tile_impl::tile_datasource_pbf>(
                                            layer_reader,0,0,0);
        ds->set_envelope(bbox);
        CHECK( ds->type() == mapnik::datasource::Vector );
        CHECK( ds->get_geometry_type() == mapnik::datasource_geometry_t::Collection );
        mapnik::layer_descriptor lay_desc = ds->get_descriptor();
        std::vector<std::string> expected_names;
        expected_names.push_back("osm_id");
        std::vector<std::string> names;
        for (auto const& desc : lay_desc.get_descriptors())
        {
            names.push_back(desc.get_name());
        }
        CHECK(names == expected_names);
        lyr2.set_datasource(ds);
        lyr2.add_style("style");
        map.add_layer(lyr2);
        mapnik::load_map(map,"test/data/polygon-style.xml");
        //std::clog << mapnik::save_map_to_string(map) << "\n";
        map.zoom_to_box(bbox);
        mapnik::image_rgba8 im(map.width(),map.height());
        mapnik::agg_renderer<mapnik::image_rgba8> ren(map,im);
        ren.apply();
        if (!mapnik::util::exists("test/fixtures/rasterize-expected-1.png"))
        {
            mapnik::save_to_file(im,"test/fixtures/rasterize-expected-1.png","png32");
        }
    }

    // set up to "re-render" it
    // the goal here is to trigger the geometries to pass through
    // the decoder and encoder again
    mapnik::vector_tile_impl::tile out_tile(bbox);
    
    {
        std::string merc_srs("+init=epsg:3857");
        mapnik::Map map(256, 256, merc_srs);
        protozero::pbf_reader message(uncompressed.data(), uncompressed.size());
        while (message.next(3))
        {
            protozero::pbf_reader layer_msg = message.get_message();
            auto ds = std::make_shared<mapnik::vector_tile_impl::tile_datasource_pbf>(
                        layer_msg,
                        0,
                        0,
                        0);
            mapnik::layer lyr(ds->get_name(),merc_srs);
            ds->set_envelope(out_tile.get_buffered_extent());
            lyr.set_datasource(ds);
            map.add_layer(lyr);
        }
        mapnik::vector_tile_impl::processor ren(map);
        ren.set_process_all_rings(true);
        ren.set_fill_type(mapnik::vector_tile_impl::non_zero_fill);
        ren.update_tile(out_tile);
    }
    // now `tile` should contain all the data
    std::string buffer2;
    out_tile.serialize_to_string(buffer2);
    CHECK(2824 == buffer2.size());

    std::ofstream stream_out("./test/data/0.0.0.vector-b.mvt",std::ios_base::out|std::ios_base::binary);
    stream_out << buffer2;
    stream_out.close();

    // let's now render this to a image and make sure it looks right
    {
        mapnik::Map map(256,256,"+init=epsg:3857");
        vector_tile::Tile tile2;
        CHECK(tile2.ParseFromString(buffer2));
        REQUIRE(1 == tile2.layers_size());
        vector_tile::Tile_Layer const& layer2 = tile2.layers(0);
        CHECK(std::string("water") == layer2.name());
        CHECK(5 == layer2.features_size());
        CHECK(2 == layer2.version());

        mapnik::layer lyr2("water",map.srs());
        protozero::pbf_reader tile_reader(buffer2);
        REQUIRE(tile_reader.next(mapnik::vector_tile_impl::Tile_Encoding::LAYERS));
        protozero::pbf_reader layer_reader = tile_reader.get_message();
        std::shared_ptr<mapnik::vector_tile_impl::tile_datasource_pbf> ds = std::make_shared<
                                        mapnik::vector_tile_impl::tile_datasource_pbf>(
                                            layer_reader,0,0,0);
        ds->set_envelope(bbox);
        CHECK( ds->type() == mapnik::datasource::Vector );
        CHECK( ds->get_geometry_type() == mapnik::datasource_geometry_t::Collection );
        mapnik::layer_descriptor lay_desc = ds->get_descriptor();
        std::vector<std::string> expected_names;
        expected_names.push_back("osm_id");
        std::vector<std::string> names;
        for (auto const& desc : lay_desc.get_descriptors())
        {
            names.push_back(desc.get_name());
        }
        CHECK(names == expected_names);
        lyr2.set_datasource(ds);
        lyr2.add_style("style");
        map.add_layer(lyr2);
        mapnik::load_map(map,"test/data/polygon-style.xml");
        //std::clog << mapnik::save_map_to_string(map) << "\n";
        map.zoom_to_box(bbox);
        mapnik::image_rgba8 im(map.width(),map.height());
        mapnik::agg_renderer<mapnik::image_rgba8> ren(map,im);
        ren.apply();
        unsigned diff = testing::compare_images(im,"test/fixtures/rasterize-expected-1.png");
        // should be almost equal (111 is good enough since re-rendering filters a few small degenerates)
        CHECK(111 == diff);
        if (diff > 111)
        {
            mapnik::save_to_file(im,"test/fixtures/rasterize-actual-1.png","png32");
        }
    }
}

