#include "catch.hpp"

// mapnik vector tile
#include "vector_tile_geometry_encoder_pbf.hpp"

// mapnik
#include <mapnik/geometry.hpp>

// protozero
#include <protozero/pbf_writer.hpp>

// libprotobuf
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include "vector_tile.pb.h"
#pragma GCC diagnostic pop

// std
#include <limits>

//
// Unit tests for geometry encoding of linestrings
//

TEST_CASE("encode pbf simple line_string")
{
    mapnik::geometry::line_string<std::int64_t> line;
    line.add_coord(10,10);
    line.add_coord(20,20);
    line.add_coord(30,30);
    
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::string feature_str;
    protozero::pbf_writer feature_writer(feature_str);
    vector_tile::Tile_Feature feature;
    REQUIRE(mapnik::vector_tile_impl::encode_geometry_pbf(line, feature_writer, x, y));
    feature.ParseFromString(feature_str);
    REQUIRE(feature.type() == vector_tile::Tile_GeomType_LINESTRING);
    
    // MoveTo, ParameterInteger, ParameterInteger
    // LineTo, ParameterInteger, ParameterInteger, ParameterInteger, ParameterInteger
    // Therefore 2 commands + 6 parameters = 8
    REQUIRE(feature.geometry_size() == 8);
    // MoveTo(10,10)
    CHECK(feature.geometry(0) == ((1 << 3) | 1u)); // 9
    CHECK(feature.geometry(1) == 20);
    CHECK(feature.geometry(2) == 20);
    // LineTo(20,20)
    CHECK(feature.geometry(3) == ((2 << 3) | 2u)); 
    CHECK(feature.geometry(4) == 20);
    CHECK(feature.geometry(5) == 20);
    // LineTo(30,30)
    CHECK(feature.geometry(6) == 20);
    CHECK(feature.geometry(7) == 20);
}

TEST_CASE("encode pbf simple line_string -- geometry type")
{
    mapnik::geometry::line_string<std::int64_t> line;
    line.add_coord(10,10);
    line.add_coord(20,20);
    line.add_coord(30,30);
    mapnik::geometry::geometry<std::int64_t> geom(line);
    
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::string feature_str;
    protozero::pbf_writer feature_writer(feature_str);
    vector_tile::Tile_Feature feature;
    REQUIRE(mapnik::vector_tile_impl::encode_geometry_pbf(geom, feature_writer, x, y));
    feature.ParseFromString(feature_str);
    REQUIRE(feature.type() == vector_tile::Tile_GeomType_LINESTRING);
    
    // MoveTo, ParameterInteger, ParameterInteger
    // LineTo, ParameterInteger, ParameterInteger, ParameterInteger, ParameterInteger
    // Therefore 2 commands + 6 parameters = 8
    REQUIRE(feature.geometry_size() == 8);
    // MoveTo(10,10)
    CHECK(feature.geometry(0) == ((1 << 3) | 1u)); // 9
    CHECK(feature.geometry(1) == 20);
    CHECK(feature.geometry(2) == 20);
    // LineTo(20,20)
    CHECK(feature.geometry(3) == ((2 << 3) | 2u)); 
    CHECK(feature.geometry(4) == 20);
    CHECK(feature.geometry(5) == 20);
    // LineTo(30,30)
    CHECK(feature.geometry(6) == 20);
    CHECK(feature.geometry(7) == 20);
}

TEST_CASE("encode pbf overlapping line_string")
{
    mapnik::geometry::line_string<std::int64_t> line;
    line.add_coord(10,10);
    line.add_coord(20,20);
    line.add_coord(10,10);
    
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::string feature_str;
    protozero::pbf_writer feature_writer(feature_str);
    vector_tile::Tile_Feature feature;
    REQUIRE(mapnik::vector_tile_impl::encode_geometry_pbf(line, feature_writer, x, y));
    feature.ParseFromString(feature_str);
    REQUIRE(feature.type() == vector_tile::Tile_GeomType_LINESTRING);
    
    // MoveTo, ParameterInteger, ParameterInteger
    // LineTo, ParameterInteger, ParameterInteger, ParameterInteger, ParameterInteger
    // Therefore 2 commands + 6 parameters = 8
    REQUIRE(feature.geometry_size() == 8);
    // MoveTo(10,10)
    CHECK(feature.geometry(0) == ((1 << 3) | 1u)); // 9
    CHECK(feature.geometry(1) == 20);
    CHECK(feature.geometry(2) == 20);
    // LineTo(20,20)
    CHECK(feature.geometry(3) == ((2 << 3) | 2u)); 
    CHECK(feature.geometry(4) == 20);
    CHECK(feature.geometry(5) == 20);
    // LineTo(10,10)
    CHECK(feature.geometry(6) == 19);
    CHECK(feature.geometry(7) == 19);
}

TEST_CASE("encode pbf line_string with repeated points")
{
    mapnik::geometry::line_string<std::int64_t> line;
    line.add_coord(10,10);
    line.add_coord(10,10);
    line.add_coord(10,10);
    line.add_coord(20,20);
    line.add_coord(20,20);
    line.add_coord(20,20);
    line.add_coord(30,30);
    line.add_coord(30,30);
    line.add_coord(30,30);
    
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::string feature_str;
    protozero::pbf_writer feature_writer(feature_str);
    vector_tile::Tile_Feature feature;
    REQUIRE(mapnik::vector_tile_impl::encode_geometry_pbf(line, feature_writer, x, y));
    feature.ParseFromString(feature_str);
    REQUIRE(feature.type() == vector_tile::Tile_GeomType_LINESTRING);
    
    // All of the repeated points should be removed resulting in the following:
    // MoveTo, ParameterInteger, ParameterInteger
    // LineTo, ParameterInteger, ParameterInteger, ParameterInteger, ParameterInteger
    // Therefore 2 commands + 6 parameters = 8
    REQUIRE(feature.geometry_size() == 8);
    // MoveTo(10,10)
    CHECK(feature.geometry(0) == ((1 << 3) | 1u)); // 9
    CHECK(feature.geometry(1) == 20);
    CHECK(feature.geometry(2) == 20);
    // LineTo(20,20)
    CHECK(feature.geometry(3) == ((2 << 3) | 2u)); 
    CHECK(feature.geometry(4) == 20);
    CHECK(feature.geometry(5) == 20);
    // LineTo(30,30)
    CHECK(feature.geometry(6) == 20);
    CHECK(feature.geometry(7) == 20);
}

TEST_CASE("encode pbf degenerate line_string")
{
    mapnik::geometry::line_string<std::int64_t> line;
    line.add_coord(10,10);
    
    // since the line is degenerate the whole line should be culled during encoding
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::string feature_str;
    protozero::pbf_writer feature_writer(feature_str);
    vector_tile::Tile_Feature feature;
    REQUIRE_FALSE(mapnik::vector_tile_impl::encode_geometry_pbf(line, feature_writer, x, y));
    feature.ParseFromString(feature_str);
    REQUIRE(feature.has_type());
    CHECK(feature.type() == vector_tile::Tile_GeomType_LINESTRING);
    CHECK(feature.geometry_size() == 0);
}

TEST_CASE("encode pbf degenerate line_string all repeated points")
{
    mapnik::geometry::line_string<std::int64_t> line;
    line.add_coord(10,10);
    line.add_coord(10,10);
    line.add_coord(10,10);
    line.add_coord(10,10);
    
    // since the line is degenerate the whole line should be culled during encoding
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::string feature_str;
    protozero::pbf_writer feature_writer(feature_str);
    vector_tile::Tile_Feature feature;
    REQUIRE_FALSE(mapnik::vector_tile_impl::encode_geometry_pbf(line, feature_writer, x, y));
    feature.ParseFromString(feature_str);
    CHECK(feature.has_type());
    CHECK(feature.type() == vector_tile::Tile_GeomType_LINESTRING);
    CHECK(feature.geometry_size() == 0);
}

TEST_CASE("encode pbf incredibly large segments")
{
    // This is a test case added that is known to completely break the logic
    // within the encoder. 
    std::int64_t val = std::numeric_limits<std::int64_t>::max();
    mapnik::geometry::line_string<std::int64_t> line;
    line.add_coord(0,0);
    line.add_coord(val,val);
    line.add_coord(0,0);

    std::int32_t x = 0;
    std::int32_t y = 0;
    std::string feature_str;
    protozero::pbf_writer feature_writer(feature_str);
    vector_tile::Tile_Feature feature;
    REQUIRE(mapnik::vector_tile_impl::encode_geometry_pbf(line, feature_writer, x, y));
    feature.ParseFromString(feature_str);
    REQUIRE(feature.type() == vector_tile::Tile_GeomType_LINESTRING);
    // MoveTo, ParameterInteger, ParameterInteger
    // LineTo, ParameterInteger, ParameterInteger, ParameterInteger, ParameterInteger
    // Therefore 2 commands + 6 parameters = 8
    REQUIRE(feature.geometry_size() == 8);
    // MoveTo(10,10)
    CHECK(feature.geometry(0) == ((1 << 3) | 1u)); // 9
    CHECK(feature.geometry(1) == 0);
    CHECK(feature.geometry(2) == 0);
    // LineTo(0,0)
    CHECK(feature.geometry(3) == ((2 << 3) | 2u)); 
    CHECK(feature.geometry(4) == 1);
    CHECK(feature.geometry(5) == 1);
    // LineTo(1,1)
    CHECK(feature.geometry(6) == 2);
    CHECK(feature.geometry(7) == 2);
}

TEST_CASE("encode pbf simple multi_line_string")
{
    mapnik::geometry::multi_line_string<std::int64_t> g;
    mapnik::geometry::line_string<std::int64_t> l1;
    l1.add_coord(0,0);
    l1.add_coord(1,1);
    l1.add_coord(2,2);
    g.push_back(std::move(l1));
    mapnik::geometry::line_string<std::int64_t> l2;
    l2.add_coord(5,5);
    l2.add_coord(0,0);
    g.push_back(std::move(l2));

    std::int32_t x = 0;
    std::int32_t y = 0;
    std::string feature_str;
    protozero::pbf_writer feature_writer(feature_str);
    vector_tile::Tile_Feature feature;
    REQUIRE(mapnik::vector_tile_impl::encode_geometry_pbf(g, feature_writer, x, y));
    feature.ParseFromString(feature_str);
    REQUIRE(feature.type() == vector_tile::Tile_GeomType_LINESTRING);
    
    // MoveTo, ParameterInteger, ParameterInteger
    // LineTo, ParameterInteger, ParameterInteger, ParameterInteger, ParameterInteger
    // MoveTo, ParameterInteger, ParameterInteger
    // LineTo, ParameterInteger, ParameterInteger
    // Therefore 4 commands + 10 parameters = 14
    REQUIRE(feature.geometry_size() == 14);
    // MoveTo(0,0)
    CHECK(feature.geometry(0) == ((1 << 3) | 1u)); // 9
    CHECK(feature.geometry(1) == 0);
    CHECK(feature.geometry(2) == 0);
    // LineTo(1,1)
    CHECK(feature.geometry(3) == ((2 << 3) | 2u)); 
    CHECK(feature.geometry(4) == 2);
    CHECK(feature.geometry(5) == 2);
    // LineTo(2,2)
    CHECK(feature.geometry(6) == 2);
    CHECK(feature.geometry(7) == 2);
    // MoveTo(5,5)
    CHECK(feature.geometry(8) == ((1 << 3) | 1u)); // 9
    CHECK(feature.geometry(9) == 6);
    CHECK(feature.geometry(10) == 6);
    // LineTo(0,0)
    CHECK(feature.geometry(11) == ((1 << 3) | 2u)); 
    CHECK(feature.geometry(12) == 9);
    CHECK(feature.geometry(13) == 9);
}

TEST_CASE("encode pbf simple multi_line_string -- geometry type")
{
    mapnik::geometry::multi_line_string<std::int64_t> g;
    mapnik::geometry::line_string<std::int64_t> l1;
    l1.add_coord(0,0);
    l1.add_coord(1,1);
    l1.add_coord(2,2);
    g.push_back(std::move(l1));
    mapnik::geometry::line_string<std::int64_t> l2;
    l2.add_coord(5,5);
    l2.add_coord(0,0);
    g.push_back(std::move(l2));
    mapnik::geometry::geometry<std::int64_t> geom(g);

    std::int32_t x = 0;
    std::int32_t y = 0;
    std::string feature_str;
    protozero::pbf_writer feature_writer(feature_str);
    vector_tile::Tile_Feature feature;
    REQUIRE(mapnik::vector_tile_impl::encode_geometry_pbf(geom, feature_writer, x, y));
    feature.ParseFromString(feature_str);
    REQUIRE(feature.type() == vector_tile::Tile_GeomType_LINESTRING);
    
    // MoveTo, ParameterInteger, ParameterInteger
    // LineTo, ParameterInteger, ParameterInteger, ParameterInteger, ParameterInteger
    // MoveTo, ParameterInteger, ParameterInteger
    // LineTo, ParameterInteger, ParameterInteger
    // Therefore 4 commands + 10 parameters = 14
    REQUIRE(feature.geometry_size() == 14);
    // MoveTo(0,0)
    CHECK(feature.geometry(0) == ((1 << 3) | 1u)); // 9
    CHECK(feature.geometry(1) == 0);
    CHECK(feature.geometry(2) == 0);
    // LineTo(1,1)
    CHECK(feature.geometry(3) == ((2 << 3) | 2u)); 
    CHECK(feature.geometry(4) == 2);
    CHECK(feature.geometry(5) == 2);
    // LineTo(2,2)
    CHECK(feature.geometry(6) == 2);
    CHECK(feature.geometry(7) == 2);
    // MoveTo(5,5)
    CHECK(feature.geometry(8) == ((1 << 3) | 1u)); // 9
    CHECK(feature.geometry(9) == 6);
    CHECK(feature.geometry(10) == 6);
    // LineTo(0,0)
    CHECK(feature.geometry(11) == ((1 << 3) | 2u)); 
    CHECK(feature.geometry(12) == 9);
    CHECK(feature.geometry(13) == 9);
}

TEST_CASE("encode pbf multi_line_string with repeated points")
{
    mapnik::geometry::multi_line_string<std::int64_t> g;
    mapnik::geometry::line_string<std::int64_t> l1;
    l1.add_coord(0,0);
    l1.add_coord(0,0);
    l1.add_coord(0,0);
    l1.add_coord(1,1);
    l1.add_coord(1,1);
    l1.add_coord(1,1);
    l1.add_coord(2,2);
    l1.add_coord(2,2);
    l1.add_coord(2,2);
    g.push_back(std::move(l1));
    mapnik::geometry::line_string<std::int64_t> l2;
    l2.add_coord(5,5);
    l2.add_coord(5,5);
    l2.add_coord(5,5);
    l2.add_coord(0,0);
    l2.add_coord(0,0);
    l2.add_coord(0,0);
    g.push_back(std::move(l2));

    std::int32_t x = 0;
    std::int32_t y = 0;
    std::string feature_str;
    protozero::pbf_writer feature_writer(feature_str);
    vector_tile::Tile_Feature feature;
    REQUIRE(mapnik::vector_tile_impl::encode_geometry_pbf(g, feature_writer, x, y));
    feature.ParseFromString(feature_str);
    REQUIRE(feature.type() == vector_tile::Tile_GeomType_LINESTRING);
    
    // Repeated commands should be removed points should be as follows:
    // MoveTo, ParameterInteger, ParameterInteger
    // LineTo, ParameterInteger, ParameterInteger, ParameterInteger, ParameterInteger
    // MoveTo, ParameterInteger, ParameterInteger
    // LineTo, ParameterInteger, ParameterInteger
    // Therefore 4 commands + 10 parameters = 14
    REQUIRE(feature.geometry_size() == 14);
    // MoveTo(0,0)
    CHECK(feature.geometry(0) == ((1 << 3) | 1u)); // 9
    CHECK(feature.geometry(1) == 0);
    CHECK(feature.geometry(2) == 0);
    // LineTo(1,1)
    CHECK(feature.geometry(3) == ((2 << 3) | 2u)); 
    CHECK(feature.geometry(4) == 2);
    CHECK(feature.geometry(5) == 2);
    // LineTo(2,2)
    CHECK(feature.geometry(6) == 2);
    CHECK(feature.geometry(7) == 2);
    // MoveTo(5,5)
    CHECK(feature.geometry(8) == ((1 << 3) | 1u)); // 9
    CHECK(feature.geometry(9) == 6);
    CHECK(feature.geometry(10) == 6);
    // LineTo(0,0)
    CHECK(feature.geometry(11) == ((1 << 3) | 2u)); 
    CHECK(feature.geometry(12) == 9);
    CHECK(feature.geometry(13) == 9);
}

TEST_CASE("encode pbf multi_line_string with two degenerate linestrings")
{
    mapnik::geometry::multi_line_string<std::int64_t> g;
    mapnik::geometry::line_string<std::int64_t> l1;
    l1.add_coord(0,0);
    g.push_back(std::move(l1));
    mapnik::geometry::line_string<std::int64_t> l2;
    l2.add_coord(5,0);
    l2.add_coord(5,0);
    l2.add_coord(5,0);
    g.push_back(std::move(l2));
    mapnik::geometry::line_string<std::int64_t> l3;
    l3.add_coord(5,5);
    l3.add_coord(0,0);
    g.push_back(std::move(l3));

    // Should remove first line string as it does not have enough points
    // and second linestring should be removed because it only has repeated
    // points and therefore is too small
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::string feature_str;
    protozero::pbf_writer feature_writer(feature_str);
    vector_tile::Tile_Feature feature;
    REQUIRE(mapnik::vector_tile_impl::encode_geometry_pbf(g, feature_writer, x, y));
    feature.ParseFromString(feature_str);
    REQUIRE(feature.type() == vector_tile::Tile_GeomType_LINESTRING);
    
    // MoveTo, ParameterInteger, ParameterInteger
    // LineTo, ParameterInteger, ParameterInteger
    // Therefore 2 commands + 4 parameters = 6
    REQUIRE(feature.geometry_size() == 6);
    // MoveTo(5,5)
    CHECK(feature.geometry(0) == ((1 << 3) | 1u)); // 9
    CHECK(feature.geometry(1) == 10);
    CHECK(feature.geometry(2) == 10);
    // LineTo(0,0)
    CHECK(feature.geometry(3) == ((1 << 3) | 2u)); 
    CHECK(feature.geometry(4) == 9);
    CHECK(feature.geometry(5) == 9);
}
