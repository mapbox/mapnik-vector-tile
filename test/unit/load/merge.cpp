#include "catch.hpp"

// mvt
#include "vector_tile_load_tile.hpp"

//protozero
#include <protozero/pbf_reader.hpp>

// std
#include <fstream>
#include <string>

TEST_CASE( "merge_from_compressed_buffer - vector" )
{
    std::ifstream stream("./test/data/0.0.0.vector.mvt",std::ios_base::in|std::ios_base::binary);
    REQUIRE(stream.is_open());
    std::string buffer(std::istreambuf_iterator<char>(stream.rdbuf()),(std::istreambuf_iterator<char>()));
    REQUIRE(buffer.size() == 3812);
    mapnik::vector_tile_impl::merc_tile tile(0,0,0);
    // not validating or upgrading
    mapnik::vector_tile_impl::merge_from_compressed_buffer(tile, buffer.data(), buffer.size());
    CHECK(tile.get_layers().size() == 1);
    {
        protozero::pbf_reader tile_msg(tile.get_buffer());
        while (tile_msg.next(mapnik::vector_tile_impl::Tile_Encoding::LAYERS))
        {
            auto layer_data = tile_msg.get_data();
            protozero::pbf_reader layer_props_msg(layer_data);
            auto layer_info = mapnik::vector_tile_impl::get_layer_name_and_version(layer_props_msg);
            std::string const& layer_name = layer_info.first;
            std::uint32_t version = layer_info.second;
            CHECK(layer_name == "water");
            CHECK(version == 1);
            std::set<mapnik::vector_tile_impl::validity_error> errors;
            protozero::pbf_reader layer_valid_msg(layer_data);
            layer_is_valid(layer_valid_msg, errors);
            CHECK(errors.size() == 0);
        }
    }

    // re-adding silently skips existing layer by the same name
    mapnik::vector_tile_impl::merge_from_compressed_buffer(tile, buffer.data(), buffer.size());
    CHECK(tile.get_layers().size() == 1);

    // re-adding and validating throws on duplicate layer name
    REQUIRE_THROWS(mapnik::vector_tile_impl::merge_from_compressed_buffer(tile, buffer.data(), buffer.size(), true));

    mapnik::vector_tile_impl::merc_tile tile2(0,0,0);
    mapnik::vector_tile_impl::merge_from_compressed_buffer(tile2, buffer.data(), buffer.size(), true, true);
    {
        protozero::pbf_reader tile_msg(tile2.get_buffer());
        while (tile_msg.next(mapnik::vector_tile_impl::Tile_Encoding::LAYERS))
        {
            auto layer_data = tile_msg.get_data();
            protozero::pbf_reader layer_props_msg(layer_data);
            auto layer_info = mapnik::vector_tile_impl::get_layer_name_and_version(layer_props_msg);
            std::string const& layer_name = layer_info.first;
            std::uint32_t version = layer_info.second;
            CHECK(layer_name == "water");
            CHECK(version == 2);
            std::set<mapnik::vector_tile_impl::validity_error> errors;
            protozero::pbf_reader layer_valid_msg(layer_data);
            layer_is_valid(layer_valid_msg, errors);
            CHECK(errors.size() == 0);
        }
    }
}

TEST_CASE( "merge_from_compressed_buffer - raster" )
{
    std::ifstream stream("./test/data/image.mvt",std::ios_base::in|std::ios_base::binary);
    REQUIRE(stream.is_open());
    std::string buffer(std::istreambuf_iterator<char>(stream.rdbuf()),(std::istreambuf_iterator<char>()));
    REQUIRE(buffer.size() == 146098);
    mapnik::vector_tile_impl::merc_tile tile(0,0,0);
    mapnik::vector_tile_impl::merge_from_compressed_buffer(tile, buffer.data(), buffer.size(), true, true);
    CHECK(tile.get_layers().size() == 1);
    {
        protozero::pbf_reader tile_msg(tile.get_buffer());
        while (tile_msg.next(mapnik::vector_tile_impl::Tile_Encoding::LAYERS))
        {
            auto layer_data = tile_msg.get_data();
            protozero::pbf_reader layer_props_msg(layer_data);
            auto layer_info = mapnik::vector_tile_impl::get_layer_name_and_version(layer_props_msg);
            std::string const& layer_name = layer_info.first;
            std::uint32_t version = layer_info.second;
            CHECK(layer_name == "layer");
            CHECK(version == 2);
            std::set<mapnik::vector_tile_impl::validity_error> errors;
            protozero::pbf_reader layer_valid_msg(layer_data);
            layer_is_valid(layer_valid_msg, errors);
            CHECK(errors.size() == 0);
        }
    }
}

TEST_CASE( "merge_from_compressed_buffer - vector with numerous empty layers" )
{
    std::map<std::string,std::pair<std::uint8_t,std::uint8_t>> expected_layers;
    expected_layers.emplace("landuse",std::make_pair(0,0));
    expected_layers.emplace("waterway",std::make_pair(0,0));
    expected_layers.emplace("water",std::make_pair(0,0));
    expected_layers.emplace("aeroway",std::make_pair(1,mapnik::vector_tile_impl::LAYER_HAS_NO_FEATURES));
    expected_layers.emplace("barrier_line",std::make_pair(1,mapnik::vector_tile_impl::LAYER_HAS_NO_FEATURES));
    expected_layers.emplace("building",std::make_pair(1,mapnik::vector_tile_impl::LAYER_HAS_NO_FEATURES));
    expected_layers.emplace("landuse_overlay",std::make_pair(0,0));
    expected_layers.emplace("tunnel",std::make_pair(1,mapnik::vector_tile_impl::LAYER_HAS_NO_FEATURES));
    expected_layers.emplace("road",std::make_pair(0,0));
    expected_layers.emplace("bridge",std::make_pair(1,mapnik::vector_tile_impl::LAYER_HAS_NO_FEATURES));
    expected_layers.emplace("admin",std::make_pair(1,mapnik::vector_tile_impl::LAYER_HAS_NO_FEATURES));
    expected_layers.emplace("country_label_line",std::make_pair(1,mapnik::vector_tile_impl::LAYER_HAS_NO_FEATURES));
    expected_layers.emplace("country_label",std::make_pair(1,mapnik::vector_tile_impl::LAYER_HAS_NO_FEATURES));
    expected_layers.emplace("marine_label",std::make_pair(1,mapnik::vector_tile_impl::LAYER_HAS_NO_FEATURES));
    expected_layers.emplace("state_label",std::make_pair(1,mapnik::vector_tile_impl::LAYER_HAS_NO_FEATURES));
    expected_layers.emplace("place_label",std::make_pair(0,0));
    expected_layers.emplace("water_label",std::make_pair(1,mapnik::vector_tile_impl::LAYER_HAS_NO_FEATURES));
    expected_layers.emplace("area_label",std::make_pair(1,mapnik::vector_tile_impl::LAYER_HAS_NO_FEATURES));
    expected_layers.emplace("rail_station_label",std::make_pair(1,mapnik::vector_tile_impl::LAYER_HAS_NO_FEATURES));
    expected_layers.emplace("airport_label",std::make_pair(1,mapnik::vector_tile_impl::LAYER_HAS_NO_FEATURES));
    expected_layers.emplace("road_label",std::make_pair(1,mapnik::vector_tile_impl::LAYER_HAS_NO_FEATURES));
    expected_layers.emplace("waterway_label",std::make_pair(1,mapnik::vector_tile_impl::LAYER_HAS_NO_FEATURES));
    expected_layers.emplace("building_label",std::make_pair(1,mapnik::vector_tile_impl::LAYER_HAS_NO_FEATURES));

    {
        std::ifstream stream("./test/data/invalid_v2_tile.mvt",std::ios_base::in|std::ios_base::binary);
        REQUIRE(stream.is_open());
        std::string buffer(std::istreambuf_iterator<char>(stream.rdbuf()),(std::istreambuf_iterator<char>()));

        REQUIRE(expected_layers.size() == 23);
        {
            mapnik::vector_tile_impl::merc_tile tile(0,0,0);
            // not validating or upgrading
            mapnik::vector_tile_impl::merge_from_compressed_buffer(tile, buffer.data(), buffer.size());
            REQUIRE(tile.get_layers().size() == 23);
            {
                protozero::pbf_reader tile_msg(tile.get_buffer());
                std::size_t idx = 0;
                while (tile_msg.next(mapnik::vector_tile_impl::Tile_Encoding::LAYERS))
                {
                    auto layer_data = tile_msg.get_data();
                    protozero::pbf_reader layer_props_msg(layer_data);
                    auto layer_info = mapnik::vector_tile_impl::get_layer_name_and_version(layer_props_msg);
                    std::string const& layer_name = layer_info.first;
                    auto const& expected = expected_layers[layer_name];
                    std::uint32_t version = layer_info.second;
                    CHECK(version == 2);
                    std::set<mapnik::vector_tile_impl::validity_error> errors;
                    protozero::pbf_reader layer_valid_msg(layer_data);
                    layer_is_valid(layer_valid_msg, errors);
                    INFO(layer_name);
                    CHECK(errors.size() == expected.first);
                    if (errors.size() > 0)
                    {
                        INFO(layer_name);
                        CHECK(*errors.begin() == expected.second);
                    }
                }
            }
        }

        {
            mapnik::vector_tile_impl::merc_tile tile(0,0,0);
            // upgrading but not validating
            mapnik::vector_tile_impl::merge_from_compressed_buffer(tile, buffer.data(), buffer.size(), false, true);
            REQUIRE(tile.get_layers().size() == 23);
            {
                protozero::pbf_reader tile_msg(tile.get_buffer());
                std::size_t idx = 0;
                while (tile_msg.next(mapnik::vector_tile_impl::Tile_Encoding::LAYERS))
                {
                    auto layer_data = tile_msg.get_data();
                    protozero::pbf_reader layer_props_msg(layer_data);
                    auto layer_info = mapnik::vector_tile_impl::get_layer_name_and_version(layer_props_msg);
                    std::string const& layer_name = layer_info.first;
                    auto const& expected = expected_layers[layer_name];
                    std::uint32_t version = layer_info.second;
                    CHECK(version == 2);
                    std::set<mapnik::vector_tile_impl::validity_error> errors;
                    protozero::pbf_reader layer_valid_msg(layer_data);
                    layer_is_valid(layer_valid_msg, errors);
                    INFO(layer_name);
                    CHECK(errors.size() == expected.first);
                    if (errors.size() > 0)
                    {
                        INFO(layer_name);
                        CHECK(*errors.begin() == expected.second);
                    }
                }
            }
        }
    }

    {
        std::ifstream stream("./test/data/invalid_v1_tile.mvt",std::ios_base::in|std::ios_base::binary);
        REQUIRE(stream.is_open());
        std::string buffer(std::istreambuf_iterator<char>(stream.rdbuf()),(std::istreambuf_iterator<char>()));

        REQUIRE(expected_layers.size() == 23);
        {
            mapnik::vector_tile_impl::merc_tile tile(0,0,0);
            // not validating or upgrading
            mapnik::vector_tile_impl::merge_from_compressed_buffer(tile, buffer.data(), buffer.size());
            REQUIRE(tile.get_layers().size() == 23);
            {
                protozero::pbf_reader tile_msg(tile.get_buffer());
                std::size_t idx = 0;
                while (tile_msg.next(mapnik::vector_tile_impl::Tile_Encoding::LAYERS))
                {
                    auto layer_data = tile_msg.get_data();
                    protozero::pbf_reader layer_props_msg(layer_data);
                    auto layer_info = mapnik::vector_tile_impl::get_layer_name_and_version(layer_props_msg);
                    std::string const& layer_name = layer_info.first;
                    auto const& expected = expected_layers[layer_name];
                    std::uint32_t version = layer_info.second;
                    CHECK(version == 1);
                    std::set<mapnik::vector_tile_impl::validity_error> errors;
                    protozero::pbf_reader layer_valid_msg(layer_data);
                    layer_is_valid(layer_valid_msg, errors);
                    INFO(layer_name);
                    CHECK(errors.size() == expected.first);
                    if (errors.size() > 0)
                    {
                        INFO(layer_name);
                        CHECK(*errors.begin() == expected.second);
                    }
                }
            }
        }

        {
            mapnik::vector_tile_impl::merc_tile tile(0,0,0);
            // upgrading but not validating
            mapnik::vector_tile_impl::merge_from_compressed_buffer(tile, buffer.data(), buffer.size(), false, true);
            REQUIRE(tile.get_layers().size() == 6);
            {
                protozero::pbf_reader tile_msg(tile.get_buffer());
                std::size_t idx = 0;
                while (tile_msg.next(mapnik::vector_tile_impl::Tile_Encoding::LAYERS))
                {
                    auto layer_data = tile_msg.get_data();
                    protozero::pbf_reader layer_props_msg(layer_data);
                    auto layer_info = mapnik::vector_tile_impl::get_layer_name_and_version(layer_props_msg);
                    std::string const& layer_name = layer_info.first;
                    auto const& expected = expected_layers[layer_name];
                    std::uint32_t version = layer_info.second;
                    CHECK(version == 2);
                    std::set<mapnik::vector_tile_impl::validity_error> errors;
                    protozero::pbf_reader layer_valid_msg(layer_data);
                    layer_is_valid(layer_valid_msg, errors);
                    INFO(layer_name);
                    CHECK(errors.size() == expected.first);
                    CHECK(errors.size() == 0);
                    if (errors.size() > 0)
                    {
                        INFO(layer_name);
                        CHECK(*errors.begin() == expected.second);
                    }
                }
            }
        }

    }
}