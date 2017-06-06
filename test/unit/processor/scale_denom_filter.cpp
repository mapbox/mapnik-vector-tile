#include "catch.hpp"

// mapnik
#include <mapnik/load_map.hpp>

// mapnik-vector-tile
#include "vector_tile_processor.hpp"

// libprotobuf
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include "vector_tile.pb.h"
#pragma GCC diagnostic pop


TEST_CASE("feature processor - filtering by scale denominator on layer level")
{
    mapnik::Map map(256, 256);
    mapnik::load_map(map, "test/data/layer_scale_denom_style.xml");
    mapnik::vector_tile_impl::processor ren(map);
    const double scale_denom = 200000;

    {
        mapnik::vector_tile_impl::tile out_tile = ren.create_tile(
            2048, 2047, 12, 4096, 0, scale_denom);
        vector_tile::Tile tile;
        REQUIRE(tile.ParseFromString(out_tile.get_buffer()));
        REQUIRE(2 == tile.layers_size());
        CHECK(1 == tile.layers(0).features_size());
        CHECK(2 == tile.layers(1).features_size());
    }
    {
        // One layer is filtered out after change of scale denominator.
        mapnik::vector_tile_impl::tile out_tile = ren.create_tile(
            2048, 2047, 12, 4096, 0, scale_denom * 2.0);
        vector_tile::Tile tile;
        REQUIRE(tile.ParseFromString(out_tile.get_buffer()));
        REQUIRE(1 == tile.layers_size());
        CHECK(1 == tile.layers(0).features_size());
    }
}
