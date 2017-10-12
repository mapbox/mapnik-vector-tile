#include "catch.hpp"

// mapnik
#include <mapnik/geometry.hpp>
#include <mapnik/version.hpp>
#if MAPNIK_VERSION >= 300100
#include <mapnik/geometry/strategy.hpp>
#include <mapnik/geometry/transform.hpp>
#else
#include <mapnik/geometry_strategy.hpp>
#include <mapnik/geometry_transform.hpp>
#endif

// mapnik-vector-tile
#include "vector_tile_geometry_encoder_pbf.hpp"
#include "vector_tile_datasource_pbf.hpp"

// vector tile
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include "vector_tile.pb.h"
#pragma GCC diagnostic pop

TEST_CASE("encoding multi line string and check output datasource")
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

    std::string buffer;
    tile.SerializeToString(&buffer);
    protozero::pbf_reader pbf_tile(buffer);
    pbf_tile.next();
    protozero::pbf_reader layer2 = pbf_tile.get_message();
    mapnik::vector_tile_impl::tile_datasource_pbf ds(layer2,0,0,0);
    mapnik::box2d<double> bbox(-20037508.342789,-20037508.342789,20037508.342789,20037508.342789);
    fs = ds.features(mapnik::query(bbox));
    f_ptr = fs->next();
    REQUIRE(f_ptr != mapnik::feature_ptr());
    // no attributes
    CHECK(f_ptr->context()->size() == 0);

    CHECK(f_ptr->get_geometry().is<mapnik::geometry::multi_line_string<double> >());
}

TEST_CASE("encoding and decoding with datasource simple polygon")
{
    mapnik::geometry::polygon<double> geom;
    {
        mapnik::geometry::linear_ring<double> ring;
        ring.emplace_back(168.267850,-24.576888);
        ring.emplace_back(167.982618,-24.697145);
        ring.emplace_back(168.114561,-24.783548);
        ring.emplace_back(168.267850,-24.576888);
        ring.emplace_back(168.267850,-24.576888);
        geom.push_back(std::move(ring));
    }
    
    unsigned path_multiplier = 16;
    mapnik::geometry::scale_rounding_strategy scale_strat(path_multiplier);
    mapnik::geometry::polygon<std::int64_t> geom2 = mapnik::geometry::transform<std::int64_t>(geom, scale_strat);

    // encode geometry
    vector_tile::Tile tile;
    vector_tile::Tile_Layer * t_layer = tile.add_layers();
    vector_tile::Tile_Feature * t_feature = t_layer->add_features();
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::string feature_str;
    protozero::pbf_writer feature_writer(feature_str);
    CHECK(mapnik::vector_tile_impl::encode_geometry_pbf(geom2, feature_writer, x, y));
    t_feature->ParseFromString(feature_str);
    
    // test results
    REQUIRE(1 == tile.layers_size());
    vector_tile::Tile_Layer const& layer = tile.layers(0);
    CHECK(1 == layer.features_size());
    vector_tile::Tile_Feature const& f = layer.features(0);
    CHECK(11 == f.geometry_size());
}
