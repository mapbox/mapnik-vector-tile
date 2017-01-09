#include "catch.hpp"

// test utils
#include "encoding_util.hpp"

//
// Point Round Trip
//

TEST_CASE("encode and decode point") 
{
    mapbox::geometry::point<std::int64_t> g(0,0);
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
    mapbox::geometry::multi_point<std::int64_t> g;
    g.emplace_back(0,0);
    g.emplace_back(1,1);
    g.emplace_back(2,2);
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
    mapbox::geometry::line_string<std::int64_t> g;
    g.emplace_back(0,0);
    g.emplace_back(1,1);
    g.emplace_back(100,100);
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
    mapbox::geometry::multi_line_string<std::int64_t> g;
    {
        mapbox::geometry::line_string<std::int64_t> line;
        line.emplace_back(0,0);
        line.emplace_back(1,1);
        line.emplace_back(100,100);
        g.emplace_back(std::move(line));
    }
    {
        mapbox::geometry::line_string<std::int64_t> line;
        line.emplace_back(-10,-10);
        line.emplace_back(-20,-20);
        line.emplace_back(-100,-100);
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
    mapbox::geometry::polygon<std::int64_t> g;
    mapbox::geometry::linear_ring<std::int64_t> lr;
    lr.emplace_back(0,0);
    lr.emplace_back(100,0);
    lr.emplace_back(100,100);
    lr.emplace_back(0,0);
    g.push_back(std::move(lr));
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
