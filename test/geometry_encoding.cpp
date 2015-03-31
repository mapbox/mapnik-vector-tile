#include "catch.hpp"

#include "encoding_util.hpp"
//#include <mapnik/geometry_correct.hpp>
//#include <mapnik/geometry_unique.hpp>

/*

low level encoding and decoding that skips clipping

*/

TEST_CASE( "point", "should round trip without changes" ) {
    mapnik::geometry::point g(0,0);
    std::string expected(
    "move_to(0,0)\n"
    );
    CHECK(compare(g) == expected);
}

TEST_CASE( "multi_point", "should round trip without changes" ) {
    mapnik::geometry::multi_point g;
    g.add_coord(0,0);
    g.add_coord(1,1);
    g.add_coord(2,2);
    std::string expected(
    "move_to(0,0)\n"
    "move_to(1,1)\n"
    "move_to(2,2)\n"
    );
    CHECK(compare(g) == expected);
}

TEST_CASE( "line_string", "should round trip without changes" ) {
    mapnik::geometry::line_string g;
    g.add_coord(0,0);
    g.add_coord(1,1);
    g.add_coord(100,100);
    std::string expected(
    "move_to(0,0)\n"
    "line_to(1,1)\n"
    "line_to(100,100)\n"
    );
    CHECK(compare(g) == expected);
}

TEST_CASE( "multi_line_string", "should round trip without changes" ) {
    mapnik::geometry::multi_line_string g;
    {
        mapnik::geometry::line_string line;
        line.add_coord(0,0);
        line.add_coord(1,1);
        line.add_coord(100,100);
        g.emplace_back(std::move(line));
    }
    {
        mapnik::geometry::line_string line;
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
    CHECK(compare(g) == expected);
}

TEST_CASE( "polygon", "should round trip without changes" ) {
    mapnik::geometry::polygon g;
    g.exterior_ring.add_coord(0,0);
    g.exterior_ring.add_coord(1,1);
    g.exterior_ring.add_coord(100,100);
    g.exterior_ring.add_coord(0,0);
    std::string expected(
    "move_to(0,0)\n"
    "line_to(1,1)\n"
    "line_to(100,100)\n"
    "close_path(0,0)\n"
    );
    CHECK(compare(g) == expected);
}

TEST_CASE( "polygon with hole", "should round trip without changes" ) {
    // NOTE: this polygon should have correct winding order:
    // CCW for exterior, CW for interior
    // TODO: confirm with boost::geometry::correct
    mapnik::geometry::polygon g;
    g.exterior_ring.add_coord(0,0);
    g.exterior_ring.add_coord(0,10);
    g.exterior_ring.add_coord(-10,10);
    g.exterior_ring.add_coord(-10,0);
    g.exterior_ring.add_coord(0,0);
    mapnik::geometry::linear_ring hole;
    hole.add_coord(-7,7);
    hole.add_coord(-3,7);
    hole.add_coord(-3,3);
    hole.add_coord(-7,3);
    hole.add_coord(-7,7);
    g.add_hole(std::move(hole));
    mapnik::geometry::linear_ring hole_in_hole;
    hole_in_hole.add_coord(-6,4);
    hole_in_hole.add_coord(-6,6);
    hole_in_hole.add_coord(-4,6);
    hole_in_hole.add_coord(-4,4);
    hole_in_hole.add_coord(-6,4);
    g.add_hole(std::move(hole_in_hole));
    std::string expected(
    "move_to(0,0)\n"
    "line_to(0,10)\n"
    "line_to(-10,10)\n"
    "line_to(-10,0)\n"
    "close_path(0,0)\n"
    );
    CHECK(compare(g) == expected);
}

TEST_CASE( "test 2", "should drop coincident line_to commands" ) {
    mapnik::geometry::line_string g;
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
    CHECK( compare(g,1) == expected);
}

TEST_CASE( "test 2b", "should drop vertices" ) {
    mapnik::geometry::line_string g;
    g.add_coord(0,0);
    g.add_coord(0,0);
    g.add_coord(1,1);
    std::string expected(
    "move_to(0,0)\n"
    "line_to(0,0)\n" // TODO - should we try to drop this?
    "line_to(1,1)\n"
    );
    CHECK(compare(g,1) == expected);
}

TEST_CASE( "test 3", "should not drop first move_to or last vertex in line" ) {
    mapnik::geometry::multi_line_string g;
    mapnik::geometry::line_string l1;
    l1.add_coord(0,0);
    l1.add_coord(1,1);
    g.push_back(std::move(l1));
    mapnik::geometry::line_string l2;
    l2.add_coord(2,2);
    l2.add_coord(3,3);
    g.push_back(std::move(l2));

    std::string expected(
    "move_to(0,0)\n"
    "line_to(1,1)\n"
    "move_to(2,2)\n"
    "line_to(3,3)\n"
    );
    CHECK(compare(g,1000) == expected);
}

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
    decode_geometry(feature0,g2,x0,y0,path_multiplier);
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
    decode_geometry(feature0,g2,x0,y0,path_multiplier);
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
