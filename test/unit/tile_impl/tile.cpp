#include "catch.hpp"

// mapnik vector tile tile class
#include "vector_tile_tile.hpp"

TEST_CASE("Vector tile base class")
{
    mapnik::box2d<double> global_extent(-20037508.342789,-20037508.342789,20037508.342789,20037508.342789);

    SECTION("default constructed")
    {
        mapnik::vector_tile_impl::tile default_tile(global_extent);

        CHECK(default_tile.size() == 0);
        CHECK(default_tile.data()[0] == '\0');
        CHECK(std::abs(default_tile.scale() - 9783.9396205024) < 0.00001);

        std::string str;
        default_tile.serialize_to_string(str);
        CHECK(str == "");
        CHECK(default_tile.is_painted() == false);
        CHECK(default_tile.is_empty() == true);

        CHECK(default_tile.extent() == global_extent);
        CHECK(default_tile.get_buffered_extent() == global_extent);
        CHECK(default_tile.tile_size() == 4096);

        CHECK(default_tile.get_painted_layers().empty() == true);
        CHECK(default_tile.get_empty_layers().empty() == true);
        CHECK(default_tile.get_layers().empty() == true);
        CHECK(default_tile.get_layers_set().empty() == true);

        CHECK(default_tile.has_layer("anything") == false);

        vector_tile::Tile t;
        t = default_tile.get_tile();
        CHECK(t.layers_size() == 0);
    }
}
