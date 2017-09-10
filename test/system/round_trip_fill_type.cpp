#include "catch.hpp"

// test-utils
#include "round_trip.hpp"
#include "geom_to_wkt.hpp"

// mapnik
#include <mapnik/version.hpp>
#if MAPNIK_VERSION >= 300100
#include <mapnik/geometry/is_empty.hpp>
#else
#include <mapnik/geometry_is_empty.hpp>
#endif

TEST_CASE("vector tile polygon even odd fill")
{
    using namespace mapnik::geometry;
    polygon<double> poly;
    {
        linear_ring<double> ring;
        ring.emplace_back(0,0);
        ring.emplace_back(-10,0);
        ring.emplace_back(-10,10);
        ring.emplace_back(0,10);
        ring.emplace_back(0,0);
        poly.emplace_back(std::move(ring));
        linear_ring<double> hole;
        hole.emplace_back(-7,7);
        hole.emplace_back(-7,3);
        hole.emplace_back(-3,3);
        hole.emplace_back(-3,7);
        hole.emplace_back(-7,7);
        poly.emplace_back(std::move(hole));
    }
    mapnik::geometry::geometry<double> new_geom = test_utils::round_trip(poly,0,mapnik::vector_tile_impl::even_odd_fill);
    std::string wkt;
    CHECK( test_utils::to_wkt(wkt, new_geom) );
    CHECK( wkt == "POLYGON((128 -113.778,120.889 -113.778,120.889 -128,128 -128,128 -113.778),(123.022 -123.733,123.022 -118.044,125.867 -118.044,125.867 -123.733,123.022 -123.733))");
    CHECK( !mapnik::geometry::is_empty(new_geom) );
    REQUIRE( new_geom.is<mapnik::geometry::polygon<double> >() );
}

TEST_CASE("vector tile polygon non zero fill")
{
    using namespace mapnik::geometry;
    polygon<double> poly;
    {
        linear_ring<double> ring;
        ring.emplace_back(0,0);
        ring.emplace_back(-10,0);
        ring.emplace_back(-10,10);
        ring.emplace_back(0,10);
        ring.emplace_back(0,0);
        poly.emplace_back(std::move(ring));
        linear_ring<double> hole;
        hole.emplace_back(-7,7);
        hole.emplace_back(-7,3);
        hole.emplace_back(-3,3);
        hole.emplace_back(-3,7);
        hole.emplace_back(-7,7);
        poly.emplace_back(std::move(hole));
    }
    mapnik::geometry::geometry<double> new_geom = test_utils::round_trip(poly,0,mapnik::vector_tile_impl::non_zero_fill);
    std::string wkt;
    CHECK( test_utils::to_wkt(wkt, new_geom) );
    CHECK( wkt == "POLYGON((128 -113.778,120.889 -113.778,120.889 -128,128 -128,128 -113.778),(123.022 -123.733,123.022 -118.044,125.867 -118.044,125.867 -123.733,123.022 -123.733))");
    CHECK( !mapnik::geometry::is_empty(new_geom) );
    REQUIRE( new_geom.is<mapnik::geometry::polygon<double> >() );
}

