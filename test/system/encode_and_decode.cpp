#include "catch.hpp"

// test utils
#include "encoding_util.hpp"

//
// Point Round Trip
//

TEST_CASE("encode and decode point") 
{
    mapnik::geometry::point<std::int64_t> g(0,0);
    std::string expected(
    "move_to(0,0)\n"
    );

    SECTION("protozero decoder VT Spec v1") 
    {
        CHECK(compare_pbf(g,1) == expected);
    }

    SECTION("protozero decoder VT Spec v2") 
    {
        CHECK(compare_pbf(g,2) == expected);
    }
}

TEST_CASE( "encode and decode multipoint" ) 
{
    mapnik::geometry::multi_point<std::int64_t> g;
    g.add_coord(0,0);
    g.add_coord(1,1);
    g.add_coord(2,2);
    std::string expected(
    "move_to(0,0)\n"
    "move_to(1,1)\n"
    "move_to(2,2)\n"
    );
    
    SECTION("protozero decoder VT Spec v1") 
    {
        CHECK(compare_pbf(g,1) == expected);
    }

    SECTION("protozero decoder VT Spec v2") 
    {
        CHECK(compare_pbf(g,2) == expected);
    }
}

//
// Linestring Round Trip
//

TEST_CASE( "encode and decode linestring" )
{
    mapnik::geometry::line_string<std::int64_t> g;
    g.add_coord(0,0);
    g.add_coord(1,1);
    g.add_coord(100,100);
    std::string expected(
    "move_to(0,0)\n"
    "line_to(1,1)\n"
    "line_to(100,100)\n"
    );

    SECTION("protozero decoder VT Spec v1") 
    {
        CHECK(compare_pbf(g,1) == expected);
    }

    SECTION("protozero decoder VT Spec v2") 
    {
        CHECK(compare_pbf(g,2) == expected);
    }
}

TEST_CASE( "encode and decode multi_line_string" )
{
    mapnik::geometry::multi_line_string<std::int64_t> g;
    {
        mapnik::geometry::line_string<std::int64_t> line;
        line.add_coord(0,0);
        line.add_coord(1,1);
        line.add_coord(100,100);
        g.emplace_back(std::move(line));
    }
    {
        mapnik::geometry::line_string<std::int64_t> line;
        line.add_coord(-10,-10);
        line.add_coord(-20,-20);
        line.add_coord(-100,-100);
        g.emplace_back(std::move(line));
    }
    std::string expected(
    "move_to(0,0)\n"
    "line_to(1,1)\n"
    "line_to(100,100)\n"
    "move_to(-10,-10)\n"
    "line_to(-20,-20)\n"
    "line_to(-100,-100)\n"
    );

    SECTION("protozero decoder VT Spec v1") 
    {
        CHECK(compare_pbf(g,1) == expected);
    }

    SECTION("protozero decoder VT Spec v2") 
    {
        CHECK(compare_pbf(g,2) == expected);
    }
}

TEST_CASE( "encode and decode polygon" ) 
{
    mapnik::geometry::polygon<std::int64_t> g;
    g.exterior_ring.add_coord(0,0);
    g.exterior_ring.add_coord(100,0);
    g.exterior_ring.add_coord(100,100);
    g.exterior_ring.add_coord(0,0);
    std::string expected(
    "move_to(0,0)\n"
    "line_to(100,0)\n"
    "line_to(100,100)\n"
    "close_path(0,0)\n"
    );

    SECTION("protozero decoder VT Spec v1") 
    {
        CHECK(compare_pbf(g,1) == expected);
    }

    SECTION("protozero decoder VT Spec v2") 
    {
        CHECK(compare_pbf(g,2) == expected);
    }
}
