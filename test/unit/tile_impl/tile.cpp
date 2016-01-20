#include "catch.hpp"
#include <iostream>

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

    SECTION("construction with zero tile_size")
    {
        mapnik::vector_tile_impl::tile zero_size_tile(global_extent, 0);

        CHECK(zero_size_tile.tile_size() == 0);
        CHECK(std::abs(zero_size_tile.scale() - 40075016.6855780035) < 0.00001);
        CHECK(zero_size_tile.get_buffered_extent() == global_extent);
    }

    SECTION("construction with negative tile_size")
    {
        mapnik::vector_tile_impl::tile negative_size_tile(global_extent, -1);

        CHECK(negative_size_tile.tile_size() == 4294967295);
        CHECK(std::abs(negative_size_tile.scale() - 0.0093306919) < 0.0000001);
        CHECK(negative_size_tile.get_buffered_extent() == global_extent);
    }

    SECTION("construction with positive buffer size")
    {
        mapnik::vector_tile_impl::tile positive_buffer_tile(global_extent, 4096, 10);

        mapnik::box2d<double> buffered_extent(-20135347.7389940246939659,-20135347.7389940246939659,20135347.7389940246939659,20135347.7389940246939659);
        CHECK(positive_buffer_tile.get_buffered_extent() == buffered_extent);
        CHECK(positive_buffer_tile.buffer_size() == 10);
    }

    SECTION("construction with very negative buffer size")
    {
        mapnik::vector_tile_impl::tile negative_buffer_tile(global_extent, 4096, -4000);
        mapnik::box2d<double> buffered_extent(0.0, 0.0, 0.0, 0.0);
        CHECK(negative_buffer_tile.get_buffered_extent() == buffered_extent);
        CHECK(negative_buffer_tile.buffer_size() == -4000);
    }

    SECTION("add bogus layer buffer")
    {
        mapnik::vector_tile_impl::tile tile(global_extent);
        const char *data = "blahblah";

        tile.append_layer_buffer(data, sizeof(data)/sizeof(char), "bogus");

        const std::set<std::string> expected_set{"bogus"};
        const std::set<std::string> empty_set;
        const std::vector<std::string> expected_vec{"bogus"};

        CHECK(tile.get_painted_layers() == expected_set);
        CHECK(tile.get_empty_layers() == empty_set);
        CHECK(tile.get_layers() == expected_vec);
        CHECK(tile.get_layers_set() == expected_set);
        CHECK(tile.has_layer("bogus") == true);
        CHECK(tile.is_painted() == true);
        CHECK(tile.is_empty() == false);

        CHECK(tile.size() == 10);

        std::string str;
        tile.serialize_to_string(str);
        CHECK(str == "\32\10blahblah");

        std::string buffer(tile.data());

        protozero::pbf_reader read_back(buffer);
        if (read_back.next(3))
        {
            std::string blah_blah = read_back.get_string();
            CHECK(blah_blah == "blahblah");
        }
    }
}
