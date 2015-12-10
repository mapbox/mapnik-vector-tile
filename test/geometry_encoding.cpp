#include "catch.hpp"

#include "encoding_util.hpp"
#include "decoding_util.hpp"
#include <mapnik/geometry_is_valid.hpp>
#include <mapnik/geometry_is_simple.hpp>
#include <mapnik/geometry_correct.hpp>
#include <mapnik/geometry_envelope.hpp>
#include <mapnik/util/geometry_to_wkt.hpp>

//
// Polygon Type Tests
//

TEST_CASE( "round trip polygon with degenerate exterior ring and interior ring" )
{
    mapnik::geometry::polygon<std::int64_t> p0;
    // invalid exterior ring
    p0.exterior_ring.add_coord(0,0);
    p0.exterior_ring.add_coord(0,10);
    // invalid interior ring -- is counter clockwise
    mapnik::geometry::linear_ring<std::int64_t> hole;
    hole.add_coord(-7,7);
    hole.add_coord(-3,7);
    hole.add_coord(-3,3);
    hole.add_coord(-7,3);
    hole.add_coord(-7,7);
    p0.add_hole(std::move(hole));

    // encoder should cull the first invalid ring, which triggers
    // the entire polygon to be culled.
    vector_tile::Tile_Feature feature = geometry_to_feature(p0);
    mapnik::vector_tile_impl::Geometry<double> geoms(feature,0.0,0.0,1.0,1.0);
    
    SECTION("VT Spec v1")
    {
        auto p1 = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 1);
        CHECK( p1.is<mapnik::geometry::geometry_empty>() );
    }
    
    SECTION("VT Spec v2")
    {
        auto p1 = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 2);
        CHECK( p1.is<mapnik::geometry::geometry_empty>() );
    }
}

TEST_CASE( "round trip polygon with valid exterior ring but degenerate interior ring")
{
    mapnik::geometry::polygon<std::int64_t> p0;
    p0.exterior_ring.add_coord(0,0);
    p0.exterior_ring.add_coord(0,10);
    p0.exterior_ring.add_coord(-10,10);
    p0.exterior_ring.add_coord(-10,0);
    p0.exterior_ring.add_coord(0,0);
    // invalid interior ring
    mapnik::geometry::linear_ring<std::int64_t> hole;
    hole.add_coord(-7,7);
    hole.add_coord(-3,7);
    p0.add_hole(std::move(hole));

    // since interior ring is degenerate it should have been culled when encoding occured
    vector_tile::Tile_Feature feature = geometry_to_feature(p0);
    mapnik::vector_tile_impl::Geometry<double> geoms(feature,0.0,0.0,1.0,1.0);

    SECTION("VT Spec v1")
    {
        auto p1 = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 1);
        CHECK( p1.is<mapnik::geometry::polygon<double> >() );
        auto const& poly = mapnik::util::get<mapnik::geometry::polygon<double> >(p1);
        auto const& holes = poly.interior_rings;
        CHECK( holes.empty() == true );
    }

    SECTION("VT Spec v2")
    {
        auto p1 = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 2);
        CHECK( p1.is<mapnik::geometry::polygon<double> >() );
        auto const& poly = mapnik::util::get<mapnik::geometry::polygon<double> >(p1);
        auto const& holes = poly.interior_rings;
        CHECK( holes.empty() == true );
    }
}

TEST_CASE( "round trip polygon with valid exterior ring but one degenerate interior ring of two" )
{
    mapnik::geometry::polygon<std::int64_t> p0;
    p0.exterior_ring.add_coord(0,0);
    p0.exterior_ring.add_coord(0,10);
    p0.exterior_ring.add_coord(-10,10);
    p0.exterior_ring.add_coord(-10,0);
    p0.exterior_ring.add_coord(0,0);
    // invalid interior ring
    {
        mapnik::geometry::linear_ring<std::int64_t> hole;
        hole.add_coord(-7,7);
        hole.add_coord(-3,7);
        p0.add_hole(std::move(hole));
    }
    // valid interior ring
    {
        mapnik::geometry::linear_ring<std::int64_t> hole_in_hole;
        hole_in_hole.add_coord(-6,4);
        hole_in_hole.add_coord(-6,6);
        hole_in_hole.add_coord(-4,6);
        hole_in_hole.add_coord(-4,4);
        hole_in_hole.add_coord(-6,4);
        p0.add_hole(std::move(hole_in_hole));
    }

    // since first interior ring is degenerate it should have been culled when encoding occurs
    vector_tile::Tile_Feature feature = geometry_to_feature(p0);
    mapnik::vector_tile_impl::Geometry<double> geoms(feature,0.0,0.0,1.0,1.0);
    
    SECTION("VT Spec v1")
    {
        auto p1 = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 1);
        CHECK( p1.is<mapnik::geometry::polygon<double> >() );
        auto const& poly = mapnik::util::get<mapnik::geometry::polygon<double> >(p1);
        auto const& holes = poly.interior_rings;
        CHECK( holes.size() == 1 );
    }
    
    SECTION("VT Spec v2")
    {
        auto p1 = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 2);
        CHECK( p1.is<mapnik::geometry::polygon<double> >() );
        auto const& poly = mapnik::util::get<mapnik::geometry::polygon<double> >(p1);
        auto const& holes = poly.interior_rings;
        CHECK( holes.size() == 1 );
    }
}

TEST_CASE( "round trip polygon with degenerate exterior ring, full of repeated points" )
{
    // create invalid (exterior) polygon
    mapnik::geometry::polygon<std::int64_t> p0;
    p0.exterior_ring.add_coord(10,10);
    p0.exterior_ring.add_coord(10,10);
    p0.exterior_ring.add_coord(10,10);
    p0.exterior_ring.add_coord(10,10);
    mapnik::geometry::linear_ring<std::int64_t> hole;
    hole.add_coord(-7,7);
    hole.add_coord(-3,7);
    hole.add_coord(-3,3);
    hole.add_coord(-7,3);
    hole.add_coord(-7,7);
    p0.add_hole(std::move(hole));

    // The encoder should remove repeated points, so the entire polygon should be
    // thrown out
    vector_tile::Tile_Feature feature = geometry_to_feature(p0);
    mapnik::vector_tile_impl::Geometry<double> geoms(feature,0.0,0.0,1.0,1.0);

    SECTION("VT Spec v1")
    {
        auto p1 = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 1);
        CHECK( p1.is<mapnik::geometry::geometry_empty>() );
    }
    
    SECTION("VT Spec v2")
    {
        auto p1 = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 2);
        CHECK( p1.is<mapnik::geometry::geometry_empty>() );
    }
}


TEST_CASE( "round trip multipolygon with hole" )
{
    // NOTE: this polygon should have correct winding order:
    // CCW for exterior, CW for interior
    mapnik::geometry::polygon<std::int64_t> p0;
    p0.exterior_ring.add_coord(0,0);
    p0.exterior_ring.add_coord(0,10);
    p0.exterior_ring.add_coord(-10,10);
    p0.exterior_ring.add_coord(-10,0);
    p0.exterior_ring.add_coord(0,0);
    mapnik::geometry::linear_ring<std::int64_t> hole;
    hole.add_coord(-7,7);
    hole.add_coord(-3,7);
    hole.add_coord(-3,3);
    hole.add_coord(-7,3);
    hole.add_coord(-7,7);
    p0.add_hole(std::move(hole));

    mapnik::box2d<double> extent = mapnik::geometry::envelope(p0);
    std::string expected_wkt0("POLYGON((0 0,0 10,-10 10,-10 0,0 0),(-7 7,-3 7,-3 3,-7 3,-7 7))");
    std::string wkt0;

    vector_tile::Tile_Feature feature = geometry_to_feature(p0);
    mapnik::vector_tile_impl::Geometry<double> geoms(feature,0.0,0.0,1.0,1.0);
    
    SECTION("VT Spec v1")
    {
        auto p1 = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 1);
        CHECK( p1.is<mapnik::geometry::polygon<double> >() );
        CHECK( extent == mapnik::geometry::envelope(p1) );
        CHECK( mapnik::util::to_wkt(wkt0,p1) );
        CHECK( wkt0 == expected_wkt0);
    }

    SECTION("VT Spec v2")
    {
        auto p1 = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 2);
        CHECK( p1.is<mapnik::geometry::polygon<double> >() );
        CHECK( extent == mapnik::geometry::envelope(p1) );
        CHECK( mapnik::util::to_wkt(wkt0,p1) );
        CHECK( wkt0 == expected_wkt0);
    }
}

// We no longer drop coincidental points in the encoder it should be
// done prior to reaching encoder.
/*TEST_CASE( "test 2", "should drop coincident line_to commands" ) {
    mapnik::geometry::line_string<std::int64_t> g;
    g.add_coord(0,0);
    g.add_coord(3,3);
    g.add_coord(3,3);
    g.add_coord(3,3);
    g.add_coord(3,3);
    g.add_coord(4,4);
    std::string expected(
    "move_to(0,0)\n"
    "line_to(3,3)\n"
    "line_to(4,4)\n"
    );
    CHECK( compare(g) == expected);
}*/

/*
TEST_CASE( "test 2b", "should drop vertices" ) {
    mapnik::geometry::line_string<std::int64_t> g;
    g.add_coord(0,0);
    g.add_coord(0,0);
    g.add_coord(1,1);
    std::string expected(
    "move_to(0,0)\n"
    "line_to(0,0)\n" // TODO - should we try to drop this?
    "line_to(1,1)\n"
    );
    CHECK(compare(g) == expected);
}*/
/*
TEST_CASE( "test 3", "should not drop first move_to or last vertex in line" ) {
    mapnik::geometry::multi_line_string<std::int64_t> g;
    mapnik::geometry::line_string<std::int64_t> l1;
    l1.add_coord(0,0);
    l1.add_coord(1,1);
    g.push_back(std::move(l1));
    mapnik::geometry::line_string<std::int64_t> l2;
    l2.add_coord(2,2);
    l2.add_coord(3,3);
    g.push_back(std::move(l2));

    std::string expected(
    "move_to(0,0)\n"
    "line_to(1,1)\n"
    "move_to(2,2)\n"
    "line_to(3,3)\n"
    );
    CHECK(compare(g) == expected);
}
*/
/*

TEST_CASE( "test 4", "should not drop first move_to or last vertex in polygon" ) {
    mapnik::geometry::multi_polygon g;
    mapnik::geometry::polygon p0;
    p0.exterior_ring.add_coord(0,0);
    p0.exterior_ring.add_coord(1,0);
    g.push_back(std::move(p0));

    mapnik::geometry::polygon p1;
    p1.exterior_ring.add_coord(1,1);
    p1.exterior_ring.add_coord(0,1);
    p1.exterior_ring.add_coord(1,1);
    g.push_back(std::move(p1));

    std::string expected(
    "move_to(0,0)\n"
    "line_to(1,1)\n"
    "move_to(0,0)\n"
    "line_to(1,1)\n"
    "close_path(0,0)\n"
    );
    CHECK(compare(g,1000) == expected);
}



TEST_CASE( "test 5", "can drop duplicate move_to" ) {

    mapnik::geometry::path p(mapnik::path::LineString);
    g.move_to(0,0);
    g.move_to(1,1); // skipped
    g.line_to(4,4); // skipped
    g.line_to(5,5);

    std::string expected(
    "move_to(0,0)\n" // TODO - should we keep move_to(1,1) instead?
    "line_to(5,5)\n"
    );
    CHECK(compare(g,2) == expected);
}


TEST_CASE( "test 5b", "can drop duplicate move_to" ) {
    mapnik::geometry::line_string g;
    g.move_to(0,0);
    g.move_to(1,1);
    g.line_to(2,2);
    std::string expected(
    "move_to(0,0)\n"
    "line_to(2,2)\n"
    );
    CHECK(compare(g,3) == expected);
}

TEST_CASE( "test 5c", "can drop duplicate move_to but not second" ) {
    mapnik::geometry::line_string g;
    g.move_to(0,0);
    g.move_to(1,1);
    g.line_to(2,2);
    g.move_to(3,3);
    g.line_to(4,4);
    std::string expected(
    "move_to(0,0)\n"
    "line_to(2,2)\n"
    "move_to(3,3)\n"
    "line_to(4,4)\n"
    );
    CHECK(compare(g,3) == expected);
}

TEST_CASE( "test 6", "should not drop last line_to if repeated" ) {
    mapnik::geometry::line_string g;
    g.move_to(0,0);
    g.line_to(2,2);
    g.line_to(1000,1000); // skipped
    g.line_to(1001,1001); // skipped
    g.line_to(1001,1001);
    std::string expected(
    "move_to(0,0)\n"
    "line_to(2,2)\n"
    "line_to(1001,1001)\n"
    );
    CHECK(compare(g,2) == expected);
}

TEST_CASE( "test 7", "ensure proper handling of skipping + close commands" ) {
    mapnik::geometry_type g(mapnik::geometry_type::types::Polygon);
    g.move_to(0,0);
    g.line_to(2,2);
    g.close_path();
    g.move_to(5,5);
    g.line_to(10,10); // skipped
    g.line_to(21,21);
    g.close_path();
    std::string expected(
    "move_to(0,0)\n"
    "line_to(2,2)\n"
    "close_path(0,0)\n"
    "move_to(5,5)\n"
    "line_to(21,21)\n"
    "close_path(0,0)\n"
    );
    CHECK(compare(g,100) == expected);
}

TEST_CASE( "test 8", "should drop repeated close commands" ) {
    mapnik::geometry_type g(mapnik::geometry_type::types::Polygon);
    g.move_to(0,0);
    g.line_to(2,2);
    g.close_path();
    g.close_path();
    g.close_path();
    std::string expected(
    "move_to(0,0)\n"
    "line_to(2,2)\n"
    "close_path(0,0)\n"
    );
    CHECK(compare(g,100) == expected);
}

TEST_CASE( "test 9a", "should not drop last vertex" ) {
    mapnik::geometry::line_string g;
    g.move_to(0,0);
    g.line_to(9,0); // skipped
    g.line_to(0,10);
    std::string expected(
    "move_to(0,0)\n"
    "line_to(0,10)\n"
    );
    CHECK(compare(g,11) == expected);
}

TEST_CASE( "test 9b", "should not drop last vertex" ) {
    mapnik::geometry_type g(mapnik::geometry_type::types::Polygon);
    g.move_to(0,0);
    g.line_to(10,0); // skipped
    g.line_to(0,10);
    g.close_path();
    std::string expected(
    "move_to(0,0)\n"
    "line_to(0,10)\n"
    "close_path(0,0)\n"
    );
    CHECK(compare(g,11) == expected);
}

TEST_CASE( "test 9c", "should not drop last vertex" ) {
    mapnik::geometry_type g(mapnik::geometry_type::types::Polygon);
    g.move_to(0,0);
    g.line_to(0,10);
    g.close_path();
    std::string expected(
    "move_to(0,0)\n"
    "line_to(0,10)\n"
    "close_path(0,0)\n"
    );
    CHECK(compare(g,11) == expected);
}

TEST_CASE( "test 10", "should skip repeated close and coincident line_to commands" ) {
    mapnik::geometry_type g(mapnik::geometry_type::types::Polygon);
    g.move_to(0,0);
    g.line_to(10,10);
    g.line_to(10,10); // skipped
    g.line_to(20,20);
    g.line_to(20,20); // skipped, but added back and replaces previous
    g.close_path();
    g.close_path(); // skipped
    g.close_path(); // skipped
    g.close_path(); // skipped
    g.move_to(0,0);
    g.line_to(10,10);
    g.line_to(20,20);
    g.close_path();
    g.close_path(); // skipped
    std::string expected(
    "move_to(0,0)\n"
    "line_to(10,10)\n"
    "line_to(20,20)\n"
    "close_path(0,0)\n"
    "move_to(0,0)\n"
    "line_to(10,10)\n"
    "line_to(20,20)\n"
    "close_path(0,0)\n"
    );
    CHECK(compare(g,1) == expected);
}

TEST_CASE( "test 11", "should correctly encode multiple paths" ) {
    using namespace mapnik::vector_tile_impl;
    vector_tile::Tile_Feature feature0;
    int32_t x = 0;
    int32_t y = 0;
    unsigned path_multiplier = 1;
    unsigned tolerance = 10000;
    mapnik::geometry_type g0(mapnik::geometry_type::types::Polygon);
    g0.move_to(0,0);
    g0.line_to(-10,-10);
    g0.line_to(-20,-20);
    g0.close_path();
    mapnik::vertex_adapter va(g0);
    encode_geometry(va,(vector_tile::Tile_GeomType)g0.type(),feature0,x,y,tolerance,path_multiplier);
    CHECK(x == -20);
    CHECK(y == -20);
    mapnik::geometry_type g1(mapnik::geometry_type::types::Polygon);
    g1.move_to(1000,1000);
    g1.line_to(1010,1010);
    g1.line_to(1020,1020);
    g1.close_path();
    mapnik::vertex_adapter va1(g1);
    encode_geometry(va1,(vector_tile::Tile_GeomType)g1.type(),feature0,x,y,tolerance,path_multiplier);
    CHECK(x == 1020);
    CHECK(y == 1020);
    mapnik::geometry_type g2(mapnik::geometry_type::types::Polygon);
    double x0 = 0;
    double y0 = 0;
    decode_geometry<double>(feature0,g2,x0,y0,path_multiplier);
    mapnik::vertex_adapter va2(g2);
    std::string actual = show_path(va2);
    std::string expected(
    "move_to(0,0)\n"
    "line_to(-20,-20)\n"
    "close_path(0,0)\n"
    "move_to(1000,1000)\n"
    "line_to(1020,1020)\n"
    "close_path(0,0)\n"
    );
    CHECK(actual == expected);
}

TEST_CASE( "test 12", "should correctly encode multiple paths" ) {
    using namespace mapnik::vector_tile_impl;
    vector_tile::Tile_Feature feature0;
    int32_t x = 0;
    int32_t y = 0;
    unsigned path_multiplier = 1;
    unsigned tolerance = 10;
    mapnik::geometry_type g(mapnik::geometry_type::types::Polygon);
    g.move_to(0,0);
    g.line_to(100,0);
    g.line_to(100,100);
    g.line_to(0,100);
    g.line_to(0,5);
    g.line_to(0,0);
    g.close_path();
    g.move_to(20,20);
    g.line_to(20,60);
    g.line_to(60,60);
    g.line_to(60,20);
    g.line_to(25,20);
    g.line_to(20,20);
    g.close_path();
    mapnik::vertex_adapter va(g);
    encode_geometry(va,(vector_tile::Tile_GeomType)g.type(),feature0,x,y,tolerance,path_multiplier);

    mapnik::geometry_type g2(mapnik::geometry_type::types::Polygon);
    double x0 = 0;
    double y0 = 0;
    decode_geometry<double>(feature0,g2,x0,y0,path_multiplier);
    mapnik::vertex_adapter va2(g2);
    std::string actual = show_path(va2);
    std::string expected(
        "move_to(0,0)\n"
        "line_to(100,0)\n"
        "line_to(100,100)\n"
        "line_to(0,100)\n"
        "line_to(0,0)\n"
        "close_path(0,0)\n"
        "move_to(20,20)\n"
        "line_to(20,60)\n"
        "line_to(60,60)\n"
        "line_to(60,20)\n"
        "line_to(20,20)\n"
        "close_path(0,0)\n"
        );
    CHECK(actual == expected);
}
*/
