// https://github.com/philsquared/Catch/wiki/Supplying-your-own-main()
#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#include "encoding_util.hpp"

// https://github.com/mapbox/mapnik-vector-tile/issues/36


TEST_CASE( "test 1", "should round trip without changes" ) {
    mapnik::geometry_type g(mapnik::Polygon);
    g.move_to(0,0);
    g.line_to(1,1);
    g.line_to(100,100);
    g.close_path();
    std::string expected(
    "move_to(0,0)\n"
    "line_to(1,1)\n"
    "line_to(100,100)\n"
    "close_path(0,0)\n"
    );
    CHECK(compare(g) == expected);
}

TEST_CASE( "test 2", "should drop vertices" ) {
    mapnik::geometry_type g(mapnik::LineString);
    g.move_to(0,0);
    g.line_to(3,3);
    g.line_to(3,3);
    g.line_to(3,3);
    g.line_to(3,3);
    g.line_to(4,4);
    std::string expected(
    "move_to(0,0)\n"
    "line_to(3,3)\n"
    "line_to(4,4)\n"
    );
    CHECK(compare(g,1) == expected);
}

TEST_CASE( "test 2b", "should drop vertices" ) {
    mapnik::geometry_type g(mapnik::LineString);
    g.move_to(0,0);
    g.line_to(0,0);
    g.line_to(1,1);
    std::string expected(
    "move_to(0,0)\n"
    "line_to(0,0)\n" // TODO - drop this
    "line_to(1,1)\n"
    );
    CHECK(compare(g,1) == expected);
}

TEST_CASE( "test 3", "should not drop first move_to or last vertex in line" ) {
    mapnik::geometry_type g(mapnik::LineString);
    g.move_to(0,0);
    g.line_to(1,1);
    g.move_to(0,0);
    g.line_to(1,1);
    std::string expected(
    "move_to(0,0)\n"
    "line_to(1,1)\n"
    "move_to(0,0)\n"
    "line_to(1,1)\n"
    );
    CHECK(compare(g,1000) == expected);
}

TEST_CASE( "test 4", "should not drop first move_to or last vertex in polygon" ) {
    mapnik::geometry_type g(mapnik::Polygon);
    g.move_to(0,0);
    g.line_to(1,1);
    g.move_to(0,0);
    g.line_to(1,1);
    g.close_path();
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
    mapnik::geometry_type g(mapnik::LineString);
    g.move_to(0,0);
    g.move_to(1,1);
    g.line_to(4,4);
    g.line_to(5,5);
    std::string expected(
    "move_to(0,0)\n"
    "line_to(5,5)\n"
    );
    CHECK(compare(g,2) == expected);
}

TEST_CASE( "test 5b", "can drop duplicate move_to" ) {
    mapnik::geometry_type g(mapnik::LineString);
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
    mapnik::geometry_type g(mapnik::LineString);
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

TEST_CASE( "test 6", "should not drop last move_to if repeated" ) {
    mapnik::geometry_type g(mapnik::LineString);
    g.move_to(0,0);
    g.line_to(2,2);
    g.line_to(1000,1000);
    g.line_to(1001,1001);
    g.line_to(1001,1001);
    std::string expected(
    "move_to(0,0)\n"
    "line_to(2,2)\n"
    "line_to(1001,1001)\n"
    );
    CHECK(compare(g,2) == expected);
}

TEST_CASE( "test 7", "ensure proper handling of skipping and close commands" ) {
    mapnik::geometry_type g(mapnik::Polygon);
    g.move_to(0,0);
    g.line_to(2,2);
    g.close_path();
    g.move_to(5,5);
    g.line_to(10,10);
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
    mapnik::geometry_type g(mapnik::Polygon);
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
    mapnik::geometry_type g(mapnik::LineString);
    g.move_to(0,0);
    g.line_to(9,0); // skipped
    g.line_to(0,10); // skipped
    std::string expected(
    "move_to(0,0)\n"
    "line_to(0,10)\n"
    );
    CHECK(compare(g,11) == expected);
}

TEST_CASE( "test 9b", "should not drop last vertex" ) {
    mapnik::geometry_type g(mapnik::Polygon);
    g.move_to(0,0);
    g.line_to(10,0);
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
    mapnik::geometry_type g(mapnik::Polygon);
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
    mapnik::geometry_type g(mapnik::Polygon);
    g.move_to(0,0);
    g.line_to(10,10);
    g.line_to(10,10);
    g.line_to(20,20);
    g.line_to(20,20);
    g.close_path();
    g.close_path();
    g.close_path();
    g.close_path();
    g.move_to(0,0);
    g.line_to(10,10);
    g.line_to(20,20);
    g.close_path();
    g.close_path();
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
    using namespace mapnik::vector;
    tile_feature feature0;
    int32_t x = 0;
    int32_t y = 0;
    unsigned path_multiplier = 1;
    unsigned tolerance = 10000;
    mapnik::geometry_type g0(mapnik::Polygon);
    g0.move_to(0,0);
    g0.line_to(-10,-10);
    g0.line_to(-20,-20);
    g0.close_path();
    encode_geometry(g0,(tile_GeomType)g0.type(),feature0,x,y,tolerance,path_multiplier);
    CHECK(x == -20);
    CHECK(y == -20);
    mapnik::geometry_type g1(mapnik::Polygon);
    g1.move_to(1000,1000);
    g1.line_to(1010,1010);
    g1.line_to(1020,1020);
    g1.close_path();
    encode_geometry(g1,(tile_GeomType)g1.type(),feature0,x,y,tolerance,path_multiplier);
    CHECK(x == 1020);
    CHECK(y == 1020);
    mapnik::geometry_type g2(mapnik::Polygon);
    double x0 = 0;
    double y0 = 0;
    decode_geometry(feature0,g2,x0,y0,path_multiplier);
    std::string actual = show_path(g2);
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
int main (int argc, char* const argv[])
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    int result = Catch::Session().run( argc, argv );
    if (!result) printf("\x1b[1;32m ✓ \x1b[0m\n");
    google::protobuf::ShutdownProtobufLibrary();
    return result;
}
