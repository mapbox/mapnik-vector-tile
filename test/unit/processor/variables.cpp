#include "catch.hpp"

// mapnik
#include <mapnik/map.hpp>
#include <mapnik/attribute.hpp>
#include <mapnik/memory_datasource.hpp>
#include <mapnik/feature_factory.hpp>

// mapnik-vector-tile
#include "vector_tile_processor.hpp"
#include "vector_tile_tile.hpp"


TEST_CASE("feature processor - can pass variables down to layers")
{
    SECTION("variables are optional")
    {
        mapnik::Map map(256, 256);
        mapnik::attributes empty_vars;

        mapnik::vector_tile_impl::processor some_processor(map);
        CHECK( ( empty_vars == some_processor.get_variables() ) );
    }

    SECTION("variables can be passed down to the processor")
    {
        mapnik::Map map(256, 256);
        const mapnik::attributes vars { {"zoom_level", 20} };

        mapnik::vector_tile_impl::processor some_processor(map, vars);
        CHECK( ( vars == some_processor.get_variables() ) );
    }
}
