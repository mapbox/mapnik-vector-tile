#include "catch.hpp"

// mvt
#include "vector_tile_is_valid.hpp"

// protozero
#include <protozero/pbf_writer.hpp>

#include "vector_tile.pb.h"


typedef std::set<mapnik::vector_tile_impl::validity_error> error_set_T;

TEST_CASE( "invalid empty value" )
{
    std::string buffer;
    vector_tile::Tile_Value value;
    error_set_T errs;

    value.SerializeToString(&buffer);
    protozero::pbf_reader pbf_value(buffer);

    value_is_valid(pbf_value, errs);

    CHECK(errs.empty() == false);
    CHECK(errs.count(mapnik::vector_tile_impl::validity_error::VALUE_NO_VALUE) == 1);
}

TEST_CASE( "valid values" )
{
    SECTION("valid string")
    {
        std::string buffer;
        vector_tile::Tile_Value value;
        error_set_T errs;

        value.set_string_value("just another mapnik monday");

        value.SerializeToString(&buffer);
        protozero::pbf_reader pbf_value(buffer);

        value_is_valid(pbf_value, errs);

        CHECK(errs.empty() == true);
    }

    SECTION("valid float")
    {
        std::string buffer;
        vector_tile::Tile_Value value;
        error_set_T errs;

        value.set_float_value(2015.0);

        value.SerializeToString(&buffer);
        protozero::pbf_reader pbf_value(buffer);

        value_is_valid(pbf_value, errs);

        CHECK(errs.empty() == true);
    }

    SECTION("valid double")
    {
        std::string buffer;
        vector_tile::Tile_Value value;
        error_set_T errs;

        value.set_double_value(2016.0);

        value.SerializeToString(&buffer);
        protozero::pbf_reader pbf_value(buffer);

        value_is_valid(pbf_value, errs);

        CHECK(errs.empty() == true);
    }

    SECTION("valid int")
    {
        std::string buffer;
        vector_tile::Tile_Value value;
        error_set_T errs;

        value.set_int_value(2017);

        value.SerializeToString(&buffer);
        protozero::pbf_reader pbf_value(buffer);

        value_is_valid(pbf_value, errs);

        CHECK(errs.empty() == true);
    }

    SECTION("valid uint")
    {
        std::string buffer;
        vector_tile::Tile_Value value;
        error_set_T errs;

        value.set_uint_value(2017);

        value.SerializeToString(&buffer);
        protozero::pbf_reader pbf_value(buffer);

        value_is_valid(pbf_value, errs);

        CHECK(errs.empty() == true);
    }

    SECTION("valid sint")
    {
        std::string buffer;
        vector_tile::Tile_Value value;
        error_set_T errs;

        value.set_sint_value(2017);

        value.SerializeToString(&buffer);
        protozero::pbf_reader pbf_value(buffer);

        value_is_valid(pbf_value, errs);

        CHECK(errs.empty() == true);
    }

    SECTION("valid bool")
    {
        std::string buffer;
        vector_tile::Tile_Value value;
        error_set_T errs;

        value.set_bool_value(false);

        value.SerializeToString(&buffer);
        protozero::pbf_reader pbf_value(buffer);

        value_is_valid(pbf_value, errs);

        CHECK(errs.empty() == true);
    }
}

TEST_CASE( "invalid multiple values" )
{
    std::string buffer;
    vector_tile::Tile_Value value;
    error_set_T errs;

    value.set_bool_value(false);
    value.set_string_value("false");

    value.SerializeToString(&buffer);
    protozero::pbf_reader pbf_value(buffer);

    value_is_valid(pbf_value, errs);

    CHECK(errs.empty() == false);
    CHECK(errs.count(mapnik::vector_tile_impl::validity_error::VALUE_MULTIPLE_VALUES) == 1);
}

TEST_CASE( "invalid unknown values" )
{
    std::string buffer;
    vector_tile::Tile_Value value;
    error_set_T errs;

    value.set_bool_value(false);
    value.set_string_value("false");

    value.SerializeToString(&buffer);
    protozero::pbf_reader pbf_value(buffer);

    value_is_valid(pbf_value, errs);

    CHECK(errs.empty() == false);
    CHECK(errs.count(mapnik::vector_tile_impl::validity_error::VALUE_MULTIPLE_VALUES) == 1);
}

TEST_CASE( "invalid garbage value throws" )
{
    std::string buffer = "garbage";
    error_set_T errs;

    protozero::pbf_reader pbf_value(buffer);

    CHECK_THROWS(value_is_valid(pbf_value, errs));
}

TEST_CASE( "invalid unknown tag" )
{
    std::string buffer;
    error_set_T errs;

    protozero::pbf_writer pbf_message(buffer);
    pbf_message.add_string(8, "unknown field");

    protozero::pbf_reader pbf_value(buffer);

    value_is_valid(pbf_value, errs);

    CHECK(errs.empty() == false);
    CHECK(errs.count(mapnik::vector_tile_impl::validity_error::VALUE_HAS_UNKNOWN_TAG) == 1);
}
