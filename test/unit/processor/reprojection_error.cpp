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


TEST_CASE("feature processor - handle strange projection issues")
{
    mapnik::Map map(256, 256);
    mapnik::load_map(map, "test/data/singapore.xml");
    mapnik::vector_tile_impl::processor ren(map);

    {
        mapnik::vector_tile_impl::tile out_tile = ren.create_tile(1, 0, 1);
        vector_tile::Tile tile;
        REQUIRE(tile.ParseFromString(out_tile.get_buffer()));
        // Should be empty due to simplification
        REQUIRE(0 == tile.layers_size());
    }
    {
        mapnik::vector_tile_impl::tile out_tile = ren.create_tile(1, 0, 1, 4096 * 1024);
        vector_tile::Tile tile;
        REQUIRE(tile.ParseFromString(out_tile.get_buffer()));
        // Expanding the extent here we get some data as simplification doesn't occur
        REQUIRE(1 == tile.layers_size());
        CHECK(1 == tile.layers(0).features_size());
    }
    {
        mapnik::vector_tile_impl::tile out_tile = ren.create_tile(50, 31, 6);
        vector_tile::Tile tile;
        REQUIRE(tile.ParseFromString(out_tile.get_buffer()));
        // Slightly more zoomed in we get data.
        REQUIRE(1 == tile.layers_size());
        CHECK(1 == tile.layers(0).features_size());
    }
}
