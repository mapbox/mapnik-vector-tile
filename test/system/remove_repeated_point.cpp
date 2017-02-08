#include "catch.hpp"

// test-utils
#include "round_trip.hpp"
#include "geom_to_wkt.hpp"

// mapnik
#if MAPNIK_VERSION >= 300100
#include <mapnik/geometry/is_empty.hpp>
#else
#include <mapnik/geometry_is_empty.hpp>
#endif

TEST_CASE("vector tile multi_point encoding with repeated points should be removed")
{
    mapnik::geometry::multi_point<double> geom;
    geom.emplace_back(0,0);
    geom.emplace_back(0,0);
    geom.emplace_back(1,1);
    geom.emplace_back(1,1);
    mapnik::geometry::geometry<double> new_geom = test_utils::round_trip(geom);
    CHECK( !mapnik::geometry::is_empty(new_geom) );
    std::string wkt;
    CHECK( test_utils::to_wkt(wkt, new_geom) );
    CHECK( wkt == "MULTIPOINT(128 -128,128.711 -126.578)" );
    CHECK( new_geom.is<mapnik::geometry::multi_point<double> >() );
}
