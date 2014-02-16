// https://github.com/philsquared/Catch/wiki/Supplying-your-own-main()
#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#include <mapnik/vertex.hpp>
#include <mapnik/geometry.hpp>
#include "vector_tile_geometry_encoder.hpp"

void decode_geometry(mapnik::vector::tile_feature const& f,
                     mapnik::geometry_type & geom,
                     double scale)
{
    int cmd = -1;
    const int cmd_bits = 3;
    unsigned length = 0;
    double x = 0,y = 0;
    for (int k = 0; k < f.geometry_size();)
    {
        if (!length) {
            unsigned cmd_length = f.geometry(k++);
            cmd = cmd_length & ((1 << cmd_bits) - 1);
            length = cmd_length >> cmd_bits;
        }
        if (length > 0) {
            length--;
            if (cmd == mapnik::SEG_MOVETO || cmd == mapnik::SEG_LINETO)
            {
                int32_t dx = f.geometry(k++);
                int32_t dy = f.geometry(k++);
                dx = ((dx >> 1) ^ (-(dx & 1)));
                dy = ((dy >> 1) ^ (-(dy & 1)));
                x += (static_cast<double>(dx) / scale);
                y += (static_cast<double>(dy) / scale);
                geom.push_vertex(x, y, static_cast<mapnik::CommandType>(cmd));
            }
            else if (cmd == (mapnik::SEG_CLOSE & ((1 << cmd_bits) - 1)))
            {
                geom.push_vertex(0, 0, mapnik::SEG_CLOSE);
            }
            else
            {
                std::stringstream msg;
                msg << "Unknown command type (decode_geometry): "
                    << cmd;
                throw std::runtime_error(msg.str());
            }
        }
    }
}

template <typename T>
std::string show_path(T & path)
{
    unsigned cmd = -1;
    double x = 0;
    double y = 0;
    std::ostringstream s;
    path.rewind(0);
    while ((cmd = path.vertex(&x, &y)) != mapnik::SEG_END)
    {
        switch (cmd)
        {
            case mapnik::SEG_MOVETO: s << "move_to("; break;
            case mapnik::SEG_LINETO: s << "line_to("; break;
            case mapnik::SEG_CLOSE: s << "close_path("; break;
            default: std::clog << "unhandled cmd " << cmd << "\n"; break;
        }
        s << x << "," << y << ")\n";
    }
    return s.str();
}

std::string compare(mapnik::geometry_type const & g,
                    unsigned tolerance=0,
                    unsigned path_multiplier=1)
{
    using namespace mapnik::vector;
    // encode
    tile_feature feature;
    int32_t x = 0;
    int32_t y = 0;
    encode_geometry(g,(tile_GeomType)g.type(),feature,x,y,tolerance,path_multiplier);
    // decode
    mapnik::geometry_type g2(mapnik::Polygon);
    decode_geometry(feature,g2,path_multiplier);
    return show_path(g2);
}

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

int main (int argc, char* const argv[])
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    int result = Catch::Session().run( argc, argv );
    if (!result) printf("\x1b[1;32m ✓ \x1b[0m\n");
    google::protobuf::ShutdownProtobufLibrary();
    return result;
}
