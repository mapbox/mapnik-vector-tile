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

TEST_CASE("vector tile point correctly passed through simplification code path")
{
    mapnik::geometry::point<double> geom(-122,48);
    mapnik::geometry::geometry<double> new_geom = test_utils::round_trip(geom,500);
    std::string wkt;
    CHECK( test_utils::to_wkt(wkt, new_geom) );
    CHECK( wkt == "POINT(41.244 -59.733)" );
    CHECK( !mapnik::geometry::is_empty(new_geom) );
    CHECK( new_geom.is<mapnik::geometry::point<double> >() );
}

TEST_CASE("vector tile mulit_point correctly passed through simplification code path")
{
    mapnik::geometry::multi_point<double> geom;
    geom.emplace_back(-122,48);
    geom.emplace_back(-123,49);
    mapnik::geometry::geometry<double> new_geom = test_utils::round_trip(geom,500);
    std::string wkt;
    CHECK( test_utils::to_wkt(wkt, new_geom) );
    CHECK( wkt == "MULTIPOINT(41.244 -59.733,40.533 -58.311)" );
    CHECK( !mapnik::geometry::is_empty(new_geom) );
    CHECK( new_geom.is<mapnik::geometry::multi_point<double> >() );
}

TEST_CASE("vector tile line_string is simplified")
{
    mapnik::geometry::line_string<double> line;
    line.emplace_back(0,0);
    line.emplace_back(1,1);
    line.emplace_back(2,2);
    line.emplace_back(100,100);
    mapnik::geometry::geometry<double> new_geom = test_utils::round_trip(line,500);
    std::string wkt;
    CHECK( test_utils::to_wkt(wkt, new_geom) );
    CHECK( wkt == "LINESTRING(128 -128,192 0)" );
    CHECK( !mapnik::geometry::is_empty(new_geom) );
    CHECK( new_geom.is<mapnik::geometry::line_string<double> >() );
    auto const& line2 = mapnik::util::get<mapnik::geometry::line_string<double> >(new_geom);
    CHECK( line2.size() == 2 );
}

TEST_CASE("vector tile multi_line_string is simplified")
{
    mapnik::geometry::multi_line_string<double> geom;
    mapnik::geometry::line_string<double> line;
    line.emplace_back(0,0);
    line.emplace_back(1,1);
    line.emplace_back(2,2);
    line.emplace_back(100,100);
    geom.emplace_back(std::move(line));
    mapnik::geometry::geometry<double> new_geom = test_utils::round_trip(geom,500);
    std::string wkt;
    CHECK( test_utils::to_wkt(wkt, new_geom) );
    CHECK( wkt == "LINESTRING(128 -128,192 0)" );
    CHECK( !mapnik::geometry::is_empty(new_geom) );
    CHECK( new_geom.is<mapnik::geometry::line_string<double> >() );
    auto const& line2 = mapnik::util::get<mapnik::geometry::line_string<double> >(new_geom);
    CHECK( line2.size() == 2 );
}

TEST_CASE("vector tile polygon is simplified")
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
    mapnik::geometry::geometry<double> new_geom = test_utils::round_trip(poly,500);
    std::string wkt;
    CHECK( test_utils::to_wkt(wkt, new_geom) );
    CHECK( wkt == "POLYGON((128 -113.778,120.889 -113.778,120.889 -128,128 -128,128 -113.778),(123.022 -123.733,123.022 -118.044,125.867 -118.044,125.867 -123.733,123.022 -123.733))");
    CHECK( !mapnik::geometry::is_empty(new_geom) );
    REQUIRE( new_geom.is<mapnik::geometry::polygon<double> >() );
}

TEST_CASE("vector tile mulit_polygon is simplified")
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
    multi_polygon<double> mp;
    mp.push_back(poly);
    mapnik::geometry::geometry<double> new_geom = test_utils::round_trip(mp,500);
    std::string wkt;
    CHECK( test_utils::to_wkt(wkt, new_geom) );
    CHECK( wkt == "POLYGON((128 -113.778,120.889 -113.778,120.889 -128,128 -128,128 -113.778),(123.022 -123.733,123.022 -118.044,125.867 -118.044,125.867 -123.733,123.022 -123.733))");
    CHECK( !mapnik::geometry::is_empty(new_geom) );
    REQUIRE( new_geom.is<mapnik::geometry::polygon<double> >() );
}

TEST_CASE("vector tile line_string is simplified when outside bounds", "should create vector tile with data" ) {
    mapnik::geometry::multi_line_string<double> geom;
    mapnik::geometry::line_string<double> line;
    line.emplace_back(-10000,0);
    line.emplace_back(-10000.1,0);
    line.emplace_back(100000,0);
    geom.emplace_back(std::move(line));
    mapnik::geometry::geometry<double> new_geom = test_utils::round_trip(geom,100);
    std::string wkt;
    CHECK( test_utils::to_wkt(wkt, new_geom) );
    // yep this test is weird - more of a fuzz than anything
    CHECK( wkt == "LINESTRING(0 -128,256 -128)" );
    CHECK( !mapnik::geometry::is_empty(new_geom) );
    CHECK( new_geom.is<mapnik::geometry::line_string<double> >() );
    auto const& line2 = mapnik::util::get<mapnik::geometry::line_string<double> >(new_geom);
    CHECK( line2.size() == 2 );
}
