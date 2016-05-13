#include "catch.hpp"
#include <memory>

// mapnik vector tile tile class
#include "vector_tile_tile.hpp"
#include "vector_tile_layer.hpp"

// mapnik
#include <mapnik/feature.hpp>
#include <mapnik/util/file_io.hpp>
#include <mapnik/json/feature_parser.hpp>

// libprotobuf
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include "vector_tile.pb.h"
#pragma GCC diagnostic pop

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
        
        mapnik::box2d<double> global_buffered_extent(-21289852.6142133139073849,-21289852.6142133139073849,21289852.6142133139073849,21289852.6142133139073849);
        CHECK(default_tile.extent() == global_extent);
        CHECK(default_tile.get_buffered_extent() == global_buffered_extent);
        CHECK(default_tile.tile_size() == 4096);

        CHECK(default_tile.get_painted_layers().empty() == true);
        CHECK(default_tile.get_empty_layers().empty() == true);
        CHECK(default_tile.get_layers().empty() == true);
        CHECK(default_tile.get_layers_set().empty() == true);

        CHECK(default_tile.has_layer("anything") == false);

        vector_tile::Tile t;
        t.ParseFromString(default_tile.get_buffer());
        CHECK(t.layers_size() == 0);
    }

    SECTION("construction with zero tile_size")
    {
        mapnik::vector_tile_impl::tile zero_size_tile(global_extent, 0, 0);

        CHECK(zero_size_tile.tile_size() == 0);
        CHECK(std::abs(zero_size_tile.scale() - 40075016.6855780035) < 0.00001);
        CHECK(zero_size_tile.extent() == global_extent);
        CHECK(zero_size_tile.get_buffered_extent() == global_extent);
    }

    SECTION("construction with negative tile_size")
    {
        mapnik::vector_tile_impl::tile negative_size_tile(global_extent, -1, 0);

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
        std::string bogus_layer_buffer = "blahblah";

        tile.append_layer_buffer(bogus_layer_buffer.data(), bogus_layer_buffer.length(), "bogus");

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

        // Check the buffer itself
        protozero::pbf_reader read_back(buffer);
        CHECK(read_back.next(3) == true);
        std::string blah_blah = read_back.get_string();
        CHECK(blah_blah == "blahblah");

        // Check the provided reader
        protozero::pbf_reader tile_reader = tile.get_reader();
        CHECK(tile_reader.next(3) == true);
        blah_blah = tile_reader.get_string();
        CHECK(blah_blah == "blahblah");

        protozero::pbf_reader layer_reader;
        CHECK_THROWS_AS(tile.layer_reader("bogus", layer_reader), protozero::end_of_buffer_exception);

        protozero::pbf_reader layer_reader_by_index;
        bool status = tile.layer_reader(0, layer_reader_by_index);

        CHECK(status == true);
        CHECK_THROWS_AS(layer_reader_by_index.next(1), protozero::end_of_buffer_exception);

        vector_tile::Tile bogus_tile;
        bogus_tile.ParseFromString(tile.get_buffer());
        CHECK(bogus_tile.layers_size() == 1);
        vector_tile::Tile_Layer bogus_layer = bogus_tile.layers(0);
        CHECK(bogus_layer.version() == 1);
        CHECK(bogus_layer.name() == "");
        CHECK(bogus_layer.features_size() == 0);
        CHECK(bogus_layer.keys_size() == 0);
        CHECK(bogus_layer.values_size() == 0);
        CHECK(bogus_layer.extent() == 4096);
    }

    SECTION("Add valid layer with layer buffer")
    {
        mapnik::vector_tile_impl::tile tile(global_extent);

        // Create layer
        vector_tile::Tile_Layer layer;
        layer.set_version(2);
        layer.set_name("valid");

        std::string layer_buffer;
        layer.SerializePartialToString(&layer_buffer);
        tile.append_layer_buffer(layer_buffer.data(), layer_buffer.length(), "valid");

        const std::set<std::string> expected_set{"valid"};
        const std::set<std::string> empty_set;
        const std::vector<std::string> expected_vec{"valid"};

        CHECK(tile.get_painted_layers() == expected_set);
        CHECK(tile.get_empty_layers() == empty_set);
        CHECK(tile.get_layers() == expected_vec);
        CHECK(tile.get_layers_set() == expected_set);
        CHECK(tile.has_layer("valid") == true);
        CHECK(tile.is_painted() == true);
        CHECK(tile.is_empty() == false);

        protozero::pbf_reader layer_reader_by_name;
        bool status_by_name = tile.layer_reader("valid", layer_reader_by_name);
        CHECK(status_by_name == true);
        CHECK(layer_reader_by_name.next(1) == true);
        CHECK(layer_reader_by_name.get_string() == "valid");

        protozero::pbf_reader layer_reader_by_index;
        bool status_by_index = tile.layer_reader(0, layer_reader_by_index);
        CHECK(status_by_index == true);
        CHECK(layer_reader_by_index.next(1) == true);
        CHECK(layer_reader_by_index.get_string() == "valid");

        vector_tile::Tile parsed_tile;
        parsed_tile.ParseFromString(tile.get_buffer());
        CHECK(parsed_tile.layers_size() == 1);
        vector_tile::Tile_Layer parsed_layer = parsed_tile.layers(0);
        CHECK(parsed_layer.version() == 2);
        CHECK(parsed_layer.name() == "valid");
    }
    
    SECTION("layer_reader by name works by name in buffer")
    {
        // Note - if the names of the layer are different
        // between the layer in the buffer and in the
        // tile object, `has_layer` will use the one
        // in the tile object, but `layer_reader` will
        // use the name in the buffer
        mapnik::vector_tile_impl::tile tile(global_extent);

        // Create layer
        vector_tile::Tile_Layer layer;
        layer.set_version(2);
        layer.set_name("buffer name");

        // Add layer to tile
        std::string layer_buffer;
        layer.SerializePartialToString(&layer_buffer);
        tile.append_layer_buffer(layer_buffer.data(), layer_buffer.length(), "layer name");

        const std::set<std::string> expected_set{"layer name"};
        const std::vector<std::string> expected_vec{"layer name"};

        // Confirm the use of "layer name" in these methods
        CHECK(tile.get_painted_layers() == expected_set);
        CHECK(tile.get_layers() == expected_vec);
        CHECK(tile.get_layers_set() == expected_set);
        CHECK(tile.has_layer("layer name") == true);
        CHECK(tile.has_layer("buffer name") == false);

        // Confirm the use of "buffer name" in this method
        protozero::pbf_reader layer_reader_by_buffer_name;
        bool status_by_buffer_name = tile.layer_reader("buffer name", layer_reader_by_buffer_name);
        CHECK(status_by_buffer_name == true);
        CHECK(layer_reader_by_buffer_name.next(1) == true);
        CHECK(layer_reader_by_buffer_name.get_string() == "buffer name");

        protozero::pbf_reader layer_reader_by_name;
        bool status_by_layer_name = tile.layer_reader("layer name", layer_reader_by_name);
        CHECK(status_by_layer_name == false);
    }

    SECTION("layer ordering is deterministic")
    {
        // Newly added layers from buffers are added to the end of
        // the tile, and are read from the tile in the same order
        // as they are added
        mapnik::vector_tile_impl::tile tile(global_extent);

        // Create layers
        vector_tile::Tile_Layer layer1, layer2;
        layer1.set_version(2);
        layer1.set_name("layer1");

        layer2.set_version(2);
        layer2.set_name("layer2");

        std::string layer1_buffer, layer2_buffer;
        layer1.SerializePartialToString(&layer1_buffer);
        tile.append_layer_buffer(layer1_buffer.data(), layer1_buffer.length(), "layer1");

        layer2.SerializePartialToString(&layer2_buffer);
        tile.append_layer_buffer(layer2_buffer.data(), layer2_buffer.length(), "layer2");

        const std::vector<std::string> expected_vec{"layer1", "layer2"};

        // Both of the layers are here, in order
        CHECK(tile.get_layers() == expected_vec);
        CHECK(tile.has_layer("layer1") == true);
        CHECK(tile.has_layer("layer2") == true);

        // layer_reader reads them in the same order
        protozero::pbf_reader layer_reader1, layer_reader2;
        bool status1 = tile.layer_reader(0, layer_reader1);
        CHECK(status1 == true);
        CHECK(layer_reader1.next(1) == true);
        CHECK(layer_reader1.get_string() == "layer1");

        bool status2 = tile.layer_reader(1, layer_reader2);
        CHECK(status2 == true);
        CHECK(layer_reader2.next(1) == true);
        CHECK(layer_reader2.get_string() == "layer2");
    }

    SECTION("cannot add same layer buffer twice")
    {
        // Newly added layers from buffers are added to the end of
        // the tile, and are read from the tile in the same order
        // as they are added
        mapnik::vector_tile_impl::tile tile(global_extent);

        // Create layers
        vector_tile::Tile_Layer layer1, layer2;
        layer1.set_version(2);
        layer1.set_name("layer");

        layer2.set_version(2);
        layer2.set_name("layer");

        std::string layer1_buffer, layer2_buffer;
        layer1.SerializePartialToString(&layer1_buffer);
        bool status1 = tile.append_layer_buffer(layer1_buffer.data(), layer1_buffer.length(), "layer");
        CHECK(status1 == true);

        layer2.SerializePartialToString(&layer2_buffer);
        bool status2 = tile.append_layer_buffer(layer2_buffer.data(), layer2_buffer.length(), "layer");
        CHECK(status2 == false);
    }

    SECTION("index out of bounds for layer_reader method")
    {
        mapnik::vector_tile_impl::tile tile(global_extent);

        // Read a layer from an empty tile
        protozero::pbf_reader layer_reader;
        bool status = tile.layer_reader(0, layer_reader);
        CHECK(status == false);
        CHECK(layer_reader.next(1) == false);
    }

    SECTION("adding a valid layer takes name out of empty layers")
    {
        mapnik::vector_tile_impl::tile tile(global_extent);
        tile.add_empty_layer("layer");

        const std::set<std::string> expected_set{"layer"};

        CHECK(tile.get_empty_layers() == expected_set);
        CHECK(tile.has_layer("layer") == false);
        CHECK(tile.is_painted() == false);
        CHECK(tile.is_empty() == true);

        // Create layers
        vector_tile::Tile_Layer layer;
        layer.set_version(2);
        layer.set_name("layer");

        std::string layer_buffer;
        layer.SerializePartialToString(&layer_buffer);
        tile.append_layer_buffer(layer_buffer.data(), layer_buffer.length(), "layer");

        const std::set<std::string> empty_set;

        CHECK(tile.get_empty_layers() == empty_set);
        CHECK(tile.get_painted_layers() == expected_set);
        CHECK(tile.has_layer("layer") == true);
        CHECK(tile.is_painted() == true);
        CHECK(tile.is_empty() == false);
    }

    SECTION("has same extent works correctly")
    {
        mapnik::vector_tile_impl::tile tile1(global_extent);
        mapnik::vector_tile_impl::tile tile2(global_extent);

        CHECK(tile1.same_extent(tile2) == true);
        CHECK(tile2.same_extent(tile1) == true);
    }
}
