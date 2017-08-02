#include "catch.hpp"

// mapnik
#include <mapnik/map.hpp>
#include <mapnik/attribute.hpp>
#include <mapnik/memory_datasource.hpp>

// mapnik vector tile layer class
#include "vector_tile_layer.hpp"

TEST_CASE("Vector tile layer class")
{
    SECTION("The constructor can produce a valid tile_layer with empty vars")
    {
        mapnik::Map map(256, 256);

        // Create memory datasource
        mapnik::parameters params;
        params["type"] = "memory";
        auto ds = std::make_shared<mapnik::memory_datasource>(params);

        mapnik::layer layer("layer", "+init=epsg:3857");
        layer.set_datasource(ds);
        mapnik::box2d<double> extent(-20037508.342789,-20037508.342789,20037508.342789,20037508.342789);
        const mapnik::attributes empty_vars;

        mapnik::vector_tile_impl::tile_layer some_layer(map,
                                                        layer,
                                                        extent,
                                                        256, // tile_size
                                                        10, // buffer_size
                                                        1.0, // scale_factor
                                                        0, // scale_denom
                                                        0, // offset_x
                                                        0, // offset_y
                                                        empty_vars);
        CHECK(some_layer.is_valid());
    }

    SECTION("The query has the variables passed to the constructor")
    {
        mapnik::Map map(256, 256);

        // Create memory datasource
        mapnik::parameters params;
        params["type"] = "memory";
        auto ds = std::make_shared<mapnik::memory_datasource>(params);

        mapnik::layer layer("layer", "+init=epsg:3857");
        layer.set_datasource(ds);
        mapnik::box2d<double> extent(-20037508.342789,-20037508.342789,20037508.342789,20037508.342789);
        const mapnik::attributes vars { {"zoom_level", 20} };

        mapnik::vector_tile_impl::tile_layer some_layer(map,
                                                        layer,
                                                        extent,
                                                        256, // tile_size
                                                        10, // buffer_size
                                                        1.0, // scale_factor
                                                        0, // scale_denom
                                                        0, // offset_x
                                                        0, // offset_y
                                                        vars);
        CHECK(some_layer.is_valid());
        CHECK( ( vars == some_layer.get_query().variables() ) );
    }
}
