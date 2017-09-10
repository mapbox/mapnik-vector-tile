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

TEST_CASE("vector tile round trip point encoding")
{
    mapnik::geometry::point<double> geom(0,0);
    mapnik::geometry::geometry<double> new_geom = test_utils::round_trip(geom);
    CHECK( !mapnik::geometry::is_empty(new_geom) );
    std::string wkt;
    CHECK( test_utils::to_wkt(wkt, new_geom) );
    CHECK( wkt == "POINT(128 -128)" );
    CHECK( new_geom.is<mapnik::geometry::point<double> >() );
}

TEST_CASE("vector tile geometry collection encoding")
{
    mapnik::geometry::point<double> geom_p(0,0);
    mapnik::geometry::geometry_collection<double> geom;
    geom.push_back(geom_p);
    mapnik::geometry::geometry<double> new_geom = test_utils::round_trip(geom);
    CHECK( !mapnik::geometry::is_empty(new_geom) );
    std::string wkt;
    CHECK( test_utils::to_wkt(wkt, new_geom) );
    CHECK( wkt == "POINT(128 -128)" );
    CHECK( new_geom.is<mapnik::geometry::point<double> >() );
}

TEST_CASE("vector tile geometry collection encoding x2")
{
    mapnik::geometry::point<double> geom_p(0,0);
    mapnik::geometry::geometry_collection<double> geom_t;
    geom_t.push_back(geom_p);
    mapnik::geometry::geometry_collection<double> geom;
    geom.push_back(std::move(geom_t));
    mapnik::geometry::geometry<double> new_geom = test_utils::round_trip(geom);
    CHECK( !mapnik::geometry::is_empty(new_geom) );
    std::string wkt;
    CHECK( test_utils::to_wkt(wkt, new_geom) );
    CHECK( wkt == "POINT(128 -128)" );
    CHECK( new_geom.is<mapnik::geometry::point<double> >() );
}

TEST_CASE("vector tile multi_point encoding of single point")
{
    mapnik::geometry::multi_point<double> geom;
    geom.emplace_back(0,0);
    mapnik::geometry::geometry<double> new_geom = test_utils::round_trip(geom);
    CHECK( !mapnik::geometry::is_empty(new_geom) );
    std::string wkt;
    CHECK( test_utils::to_wkt(wkt, new_geom) );
    CHECK( wkt == "POINT(128 -128)" );
    CHECK( new_geom.is<mapnik::geometry::point<double> >() );
}

TEST_CASE("vector tile multi_point encoding of actual multi_point")
{
    mapnik::geometry::multi_point<double> geom;
    geom.emplace_back(0,0);
    geom.emplace_back(1,1);
    mapnik::geometry::geometry<double> new_geom = test_utils::round_trip(geom);
    CHECK( !mapnik::geometry::is_empty(new_geom) );
    std::string wkt;
    CHECK( test_utils::to_wkt(wkt, new_geom) );
    CHECK( wkt == "MULTIPOINT(128 -128,128.711 -126.578)" );
    CHECK( new_geom.is<mapnik::geometry::multi_point<double> >() );
}

TEST_CASE("vector tile line_string encoding")
{
    mapnik::geometry::line_string<double> geom;
    geom.emplace_back(0,0);
    geom.emplace_back(100,100);
    mapnik::geometry::geometry<double> new_geom = test_utils::round_trip(geom);
    std::string wkt;
    CHECK( test_utils::to_wkt(wkt, new_geom) );
    CHECK( wkt == "LINESTRING(128 -128,192 0)" );
    CHECK( !mapnik::geometry::is_empty(new_geom) );
    CHECK( new_geom.is<mapnik::geometry::line_string<double> >() );
}

TEST_CASE("vector tile multi_line_string encoding of single line_string")
{
    mapnik::geometry::multi_line_string<double> geom;
    mapnik::geometry::line_string<double> line;
    line.emplace_back(0,0);
    line.emplace_back(100,100);
    geom.emplace_back(std::move(line));
    mapnik::geometry::geometry<double> new_geom = test_utils::round_trip(geom);
    std::string wkt;
    CHECK( test_utils::to_wkt(wkt, new_geom) );
    CHECK( wkt == "LINESTRING(128 -128,192 0)" );
    CHECK( !mapnik::geometry::is_empty(new_geom) );
    CHECK( new_geom.is<mapnik::geometry::line_string<double> >() );
}

TEST_CASE("vector tile multi_line_string encoding of actual multi_line_string")
{
    mapnik::geometry::multi_line_string<double> geom;
    mapnik::geometry::line_string<double> line;
    line.emplace_back(0,0);
    line.emplace_back(100,100);
    geom.emplace_back(std::move(line));
    mapnik::geometry::line_string<double> line2;
    line2.emplace_back(-10,-0);
    line2.emplace_back(-100,-100);
    geom.emplace_back(std::move(line2));
    mapnik::geometry::geometry<double> new_geom = test_utils::round_trip(geom);
    std::string wkt;
    CHECK( test_utils::to_wkt(wkt, new_geom) );
    CHECK( wkt == "MULTILINESTRING((128 -128,192 0),(120.889 -128,63.289 -256))" );
    CHECK( !mapnik::geometry::is_empty(new_geom) );
    CHECK( new_geom.is<mapnik::geometry::multi_line_string<double> >() );
}

TEST_CASE("vector tile polygon encoding")
{
    mapnik::geometry::polygon<double> geom;
    geom.emplace_back();
    geom.front().emplace_back(0,0);
    geom.front().emplace_back(0,10);
    geom.front().emplace_back(-10,10);
    geom.front().emplace_back(-10,0);
    geom.front().emplace_back(0,0);
    mapnik::geometry::geometry<double> new_geom = test_utils::round_trip(geom);
    CHECK( !mapnik::geometry::is_empty(new_geom) );
    CHECK( new_geom.is<mapnik::geometry::polygon<double> >() );
    std::string wkt;
    CHECK( test_utils::to_wkt(wkt, new_geom) );
    CHECK( wkt == "POLYGON((128 -113.778,120.889 -113.778,120.889 -128,128 -128,128 -113.778))");
}

TEST_CASE("vector tile multi_polygon encoding of single polygon")
{
    mapnik::geometry::polygon<double> poly;
    poly.emplace_back();
    poly.front().emplace_back(0,0);
    poly.front().emplace_back(0,10);
    poly.front().emplace_back(-10,10);
    poly.front().emplace_back(-10,0);
    poly.front().emplace_back(0,0);
    mapnik::geometry::multi_polygon<double> geom;
    geom.emplace_back(std::move(poly));
    mapnik::geometry::geometry<double> new_geom = test_utils::round_trip(geom);
    std::string wkt;
    CHECK( test_utils::to_wkt(wkt, new_geom) );
    CHECK( wkt == "POLYGON((128 -113.778,120.889 -113.778,120.889 -128,128 -128,128 -113.778))");
    CHECK( !mapnik::geometry::is_empty(new_geom) );
    CHECK( new_geom.is<mapnik::geometry::polygon<double> >() );
}

TEST_CASE("vector tile multi_polygon with multipolygon union")
{
    mapnik::geometry::polygon<double> poly;
    poly.emplace_back();
    poly.front().emplace_back(0,0);
    poly.front().emplace_back(0,10);
    poly.front().emplace_back(-10,10);
    poly.front().emplace_back(-10,0);
    poly.front().emplace_back(0,0);
    mapnik::geometry::polygon<double> poly2;
    poly2.emplace_back();
    poly2.front().emplace_back(0,0);
    poly2.front().emplace_back(0,10);
    poly2.front().emplace_back(-10,10);
    poly2.front().emplace_back(-10,0);
    poly2.front().emplace_back(0,0);
    mapnik::geometry::multi_polygon<double> geom;
    geom.emplace_back(std::move(poly));
    geom.emplace_back(std::move(poly2));
    mapnik::geometry::geometry<double> new_geom = test_utils::round_trip(geom, 0, mapnik::vector_tile_impl::non_zero_fill, true);
    std::string wkt;
    CHECK( test_utils::to_wkt(wkt, new_geom) );
    CHECK( wkt == "POLYGON((128 -113.778,120.889 -113.778,120.889 -128,128 -128,128 -113.778))");
    CHECK( !mapnik::geometry::is_empty(new_geom) );
    CHECK( new_geom.is<mapnik::geometry::polygon<double> >() );
}

TEST_CASE("vector tile multi_polygon with out multipolygon union")
{
    mapnik::geometry::polygon<double> poly;
    poly.emplace_back();
    poly.front().emplace_back(0,0);
    poly.front().emplace_back(0,10);
    poly.front().emplace_back(-10,10);
    poly.front().emplace_back(-10,0);
    poly.front().emplace_back(0,0);
    mapnik::geometry::polygon<double> poly2;
    poly2.emplace_back();
    poly2.front().emplace_back(0,0);
    poly2.front().emplace_back(0,10);
    poly2.front().emplace_back(-10,10);
    poly2.front().emplace_back(-10,0);
    poly2.front().emplace_back(0,0);
    mapnik::geometry::multi_polygon<double> geom;
    geom.emplace_back(std::move(poly));
    geom.emplace_back(std::move(poly2));
    mapnik::geometry::geometry<double> new_geom = test_utils::round_trip(geom, 0, mapnik::vector_tile_impl::non_zero_fill, false);
    std::string wkt;
    CHECK( test_utils::to_wkt(wkt, new_geom) );
    CHECK( wkt == "MULTIPOLYGON(((128 -113.778,120.889 -113.778,120.889 -128,128 -128,128 -113.778)),((128 -113.778,120.889 -113.778,120.889 -128,128 -128,128 -113.778)))");
    CHECK( !mapnik::geometry::is_empty(new_geom) );
    CHECK( new_geom.is<mapnik::geometry::multi_polygon<double> >() );
}

TEST_CASE("vector tile multi_polygon encoding of actual multi_polygon")
{
    mapnik::geometry::multi_polygon<double> geom;
    mapnik::geometry::polygon<double> poly;
    poly.emplace_back();
    poly.front().emplace_back(0,0);
    poly.front().emplace_back(0,10);
    poly.front().emplace_back(-10,10);
    poly.front().emplace_back(-10,0);
    poly.front().emplace_back(0,0);
    geom.emplace_back(std::move(poly));
    mapnik::geometry::polygon<double> poly2;
    poly2.emplace_back();
    poly2.front().emplace_back(11,11);
    poly2.front().emplace_back(11,21);
    poly2.front().emplace_back(1,21);
    poly2.front().emplace_back(1,11);
    poly2.front().emplace_back(11,11);
    geom.emplace_back(std::move(poly2));
    mapnik::geometry::geometry<double> new_geom = test_utils::round_trip(geom);
    CHECK( !mapnik::geometry::is_empty(new_geom) );
    CHECK( new_geom.is<mapnik::geometry::multi_polygon<double> >() );
}

TEST_CASE("vector tile multi_polygon encoding overlapping multipolygons")
{
    mapnik::geometry::multi_polygon<double> geom;
    mapnik::geometry::polygon<double> poly;
    poly.emplace_back();
    poly.front().emplace_back(0,0);
    poly.front().emplace_back(0,10);
    poly.front().emplace_back(-10,10);
    poly.front().emplace_back(-10,0);
    poly.front().emplace_back(0,0);
    geom.emplace_back(std::move(poly));
    mapnik::geometry::polygon<double> poly2;
    poly2.emplace_back();
    poly2.front().emplace_back(-5,5);
    poly2.front().emplace_back(-5,15);
    poly2.front().emplace_back(-15,15);
    poly2.front().emplace_back(-15,5);
    poly2.front().emplace_back(-5,5);
    geom.emplace_back(std::move(poly2));
    mapnik::geometry::geometry<double> new_geom = test_utils::round_trip(geom);
    CHECK( !mapnik::geometry::is_empty(new_geom) );
    CHECK( new_geom.is<mapnik::geometry::multi_polygon<double> >() );
}
