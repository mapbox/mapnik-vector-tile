#include "catch.hpp"

// mapnik vector tile
#include "vector_tile_geometry_encoder_pbf.hpp"

// mapbox
#include <mapbox/geometry/geometry.hpp>

// protozero
#include <protozero/pbf_writer.hpp>

// libprotobuf
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include "vector_tile.pb.h"
#pragma GCC diagnostic pop

//
// Unit tests for geometry encoding pbf of polygons
//

TEST_CASE("encoding pbf simple polygon")
{
    mapbox::geometry::polygon<std::int64_t> p0;
    mapbox::geometry::linear_ring<std::int64_t> r0;
    r0.emplace_back(0,0);
    r0.emplace_back(0,10);
    r0.emplace_back(-10,10);
    r0.emplace_back(-10,0);
    r0.emplace_back(0,0);
    p0.push_back(r0);
    
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::string feature_str;
    protozero::pbf_writer feature_writer(feature_str);
    vector_tile::Tile_Feature feature;
    REQUIRE(mapnik::vector_tile_impl::encode_geometry_pbf(p0, feature_writer, x, y));
    feature.ParseFromString(feature_str);
    REQUIRE(feature.type() == vector_tile::Tile_GeomType_POLYGON);
    
    // MoveTo, ParameterInteger, ParameterInteger
    // LineTo, ParameterInteger, ParameterInteger, ParameterInteger, ParameterInteger, ParameterInteger, ParameterInteger
    // Close
    // 3 commands + 8 Params = 11 
    REQUIRE(feature.geometry_size() == 11);
    // MoveTo(0,0)
    CHECK(feature.geometry(0) == ((1 << 3) | 1u)); // 9
    CHECK(feature.geometry(1) == 0);
    CHECK(feature.geometry(2) == 0);
    // LineTo(0,10)
    CHECK(feature.geometry(3) == ((3 << 3) | 2u));
    CHECK(feature.geometry(4) == 0);
    CHECK(feature.geometry(5) == 20);
    // LineTo(-10,10)
    CHECK(feature.geometry(6) == 19);
    CHECK(feature.geometry(7) == 0);
    // LineTo(-10,0)
    CHECK(feature.geometry(8) == 0);
    CHECK(feature.geometry(9) == 19);
    // Close
    CHECK(feature.geometry(10) == 15);
}

TEST_CASE("encoding pbf simple polygon -- geometry")
{
    mapbox::geometry::polygon<std::int64_t> p0;
    mapbox::geometry::linear_ring<std::int64_t> r0;
    r0.emplace_back(0,0);
    r0.emplace_back(0,10);
    r0.emplace_back(-10,10);
    r0.emplace_back(-10,0);
    r0.emplace_back(0,0);
    p0.push_back(r0);
    mapbox::geometry::geometry<std::int64_t> geom(p0);
    
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::string feature_str;
    protozero::pbf_writer feature_writer(feature_str);
    vector_tile::Tile_Feature feature;
    REQUIRE(mapnik::vector_tile_impl::encode_geometry_pbf(geom, feature_writer, x, y));
    feature.ParseFromString(feature_str);
    REQUIRE(feature.type() == vector_tile::Tile_GeomType_POLYGON);
    
    // MoveTo, ParameterInteger, ParameterInteger
    // LineTo, ParameterInteger, ParameterInteger, ParameterInteger, ParameterInteger, ParameterInteger, ParameterInteger
    // Close
    // 3 commands + 8 Params = 11 
    REQUIRE(feature.geometry_size() == 11);
    // MoveTo(0,0)
    CHECK(feature.geometry(0) == ((1 << 3) | 1u)); // 9
    CHECK(feature.geometry(1) == 0);
    CHECK(feature.geometry(2) == 0);
    // LineTo(0,10)
    CHECK(feature.geometry(3) == ((3 << 3) | 2u));
    CHECK(feature.geometry(4) == 0);
    CHECK(feature.geometry(5) == 20);
    // LineTo(-10,10)
    CHECK(feature.geometry(6) == 19);
    CHECK(feature.geometry(7) == 0);
    // LineTo(-10,0)
    CHECK(feature.geometry(8) == 0);
    CHECK(feature.geometry(9) == 19);
    // Close
    CHECK(feature.geometry(10) == 15);
}

TEST_CASE("encoding pbf simple polygon with hole")
{
    mapbox::geometry::polygon<std::int64_t> p0;
    mapbox::geometry::linear_ring<std::int64_t> r0;
    r0.emplace_back(0,0);
    r0.emplace_back(0,10);
    r0.emplace_back(-10,10);
    r0.emplace_back(-10,0);
    r0.emplace_back(0,0);
    p0.push_back(r0);
    mapbox::geometry::linear_ring<std::int64_t> hole;
    hole.emplace_back(-7,7);
    hole.emplace_back(-3,7);
    hole.emplace_back(-3,3);
    hole.emplace_back(-7,3);
    hole.emplace_back(-7,7);
    p0.push_back(std::move(hole));

    std::int32_t x = 0;
    std::int32_t y = 0;
    std::string feature_str;
    protozero::pbf_writer feature_writer(feature_str);
    vector_tile::Tile_Feature feature;
    REQUIRE(mapnik::vector_tile_impl::encode_geometry_pbf(p0, feature_writer, x, y));
    feature.ParseFromString(feature_str);
    REQUIRE(feature.type() == vector_tile::Tile_GeomType_POLYGON);
    
    // MoveTo, ParameterInteger, ParameterInteger
    // LineTo, ParameterInteger, ParameterInteger, ParameterInteger, ParameterInteger, ParameterInteger, ParameterInteger
    // Close
    // MoveTo, ParameterInteger, ParameterInteger
    // LineTo, ParameterInteger, ParameterInteger, ParameterInteger, ParameterInteger, ParameterInteger, ParameterInteger
    // Close
    // 6 commands + 16 Params = 22 
    REQUIRE(feature.geometry_size() == 22);
    // MoveTo(0,0)
    CHECK(feature.geometry(0) == ((1 << 3) | 1u)); // 9
    CHECK(feature.geometry(1) == 0);
    CHECK(feature.geometry(2) == 0);
    // LineTo(0,10)
    CHECK(feature.geometry(3) == ((3 << 3) | 2u));
    CHECK(feature.geometry(4) == 0);
    CHECK(feature.geometry(5) == 20);
    // LineTo(-10,10)
    CHECK(feature.geometry(6) == 19);
    CHECK(feature.geometry(7) == 0);
    // LineTo(-10,0)
    CHECK(feature.geometry(8) == 0);
    CHECK(feature.geometry(9) == 19);
    // Close
    CHECK(feature.geometry(10) == 15);
    // Remember the cursor didn't move after the close
    // so it is at -10,0
    // MoveTo(-7,7)
    CHECK(feature.geometry(11) == ((1 << 3) | 1u)); // 9
    CHECK(feature.geometry(12) == 6);
    CHECK(feature.geometry(13) == 14);
    // LineTo(-3,7)
    CHECK(feature.geometry(14) == ((3 << 3) | 2u));
    CHECK(feature.geometry(15) == 8);
    CHECK(feature.geometry(16) == 0);
    // LineTo(-3,3)
    CHECK(feature.geometry(17) == 0);
    CHECK(feature.geometry(18) == 7);
    // LineTo(-7,3)
    CHECK(feature.geometry(19) == 7);
    CHECK(feature.geometry(20) == 0);
    // Close
    CHECK(feature.geometry(21) == 15);
}

TEST_CASE("encoding pbf empty polygon")
{
    mapbox::geometry::polygon<std::int64_t> p;
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::string feature_str;
    protozero::pbf_writer feature_writer(feature_str);
    vector_tile::Tile_Feature feature;
    REQUIRE_FALSE(mapnik::vector_tile_impl::encode_geometry_pbf(p, feature_writer, x, y));
    feature.ParseFromString(feature_str);
    REQUIRE(feature.has_type());
    CHECK(feature.type() == vector_tile::Tile_GeomType_POLYGON);
    REQUIRE(feature.geometry_size() == 0);
}

TEST_CASE("encoding pbf multi polygons with holes")
{
    mapbox::geometry::multi_polygon<std::int64_t> mp;
    {
        mapbox::geometry::polygon<std::int64_t> p0;
        mapbox::geometry::linear_ring<std::int64_t> r0;
        r0.emplace_back(0,0);
        r0.emplace_back(0,10);
        r0.emplace_back(-10,10);
        r0.emplace_back(-10,0);
        r0.emplace_back(0,0);
        p0.push_back(std::move(r0));
        mapbox::geometry::linear_ring<std::int64_t> hole;
        hole.emplace_back(-7,7);
        hole.emplace_back(-3,7);
        hole.emplace_back(-3,3);
        hole.emplace_back(-7,3);
        hole.emplace_back(-7,7);
        p0.push_back(std::move(hole));
        mp.push_back(p0);
    }
    // yeah so its the same polygon -- haters gonna hate.
    {
        mapbox::geometry::polygon<std::int64_t> p0;
        mapbox::geometry::linear_ring<std::int64_t> r0;
        r0.emplace_back(0,0);
        r0.emplace_back(0,10);
        r0.emplace_back(-10,10);
        r0.emplace_back(-10,0);
        r0.emplace_back(0,0);
        p0.push_back(std::move(r0));
        mapbox::geometry::linear_ring<std::int64_t> hole;
        hole.emplace_back(-7,7);
        hole.emplace_back(-3,7);
        hole.emplace_back(-3,3);
        hole.emplace_back(-7,3);
        hole.emplace_back(-7,7);
        p0.push_back(std::move(hole));
        mp.push_back(p0);
    }

    std::int32_t x = 0;
    std::int32_t y = 0;
    std::string feature_str;
    protozero::pbf_writer feature_writer(feature_str);
    vector_tile::Tile_Feature feature;
    REQUIRE(mapnik::vector_tile_impl::encode_geometry_pbf(mp, feature_writer, x, y));
    feature.ParseFromString(feature_str);
    REQUIRE(feature.type() == vector_tile::Tile_GeomType_POLYGON);
    
    // MoveTo, ParameterInteger, ParameterInteger
    // LineTo, ParameterInteger, ParameterInteger, ParameterInteger, ParameterInteger, ParameterInteger, ParameterInteger
    // Close
    // MoveTo, ParameterInteger, ParameterInteger
    // LineTo, ParameterInteger, ParameterInteger, ParameterInteger, ParameterInteger, ParameterInteger, ParameterInteger
    // Close
    // MoveTo, ParameterInteger, ParameterInteger
    // LineTo, ParameterInteger, ParameterInteger, ParameterInteger, ParameterInteger, ParameterInteger, ParameterInteger
    // Close
    // MoveTo, ParameterInteger, ParameterInteger
    // LineTo, ParameterInteger, ParameterInteger, ParameterInteger, ParameterInteger, ParameterInteger, ParameterInteger
    // Close
    // 12 commands + 32 Params = 44
    REQUIRE(feature.geometry_size() == 44);
    // MoveTo(0,0)
    CHECK(feature.geometry(0) == ((1 << 3) | 1u)); // 9
    CHECK(feature.geometry(1) == 0);
    CHECK(feature.geometry(2) == 0);
    // LineTo(0,10)
    CHECK(feature.geometry(3) == ((3 << 3) | 2u));
    CHECK(feature.geometry(4) == 0);
    CHECK(feature.geometry(5) == 20);
    // LineTo(-10,10)
    CHECK(feature.geometry(6) == 19);
    CHECK(feature.geometry(7) == 0);
    // LineTo(-10,0)
    CHECK(feature.geometry(8) == 0);
    CHECK(feature.geometry(9) == 19);
    // Close
    CHECK(feature.geometry(10) == 15);
    // Remember the cursor didn't move after the close
    // so it is at -10,0
    // MoveTo(-7,7)
    CHECK(feature.geometry(11) == ((1 << 3) | 1u)); // 9
    CHECK(feature.geometry(12) == 6);
    CHECK(feature.geometry(13) == 14);
    // LineTo(-3,7)
    CHECK(feature.geometry(14) == ((3 << 3) | 2u));
    CHECK(feature.geometry(15) == 8);
    CHECK(feature.geometry(16) == 0);
    // LineTo(-3,3)
    CHECK(feature.geometry(17) == 0);
    CHECK(feature.geometry(18) == 7);
    // LineTo(-7,3)
    CHECK(feature.geometry(19) == 7);
    CHECK(feature.geometry(20) == 0);
    // Close
    CHECK(feature.geometry(21) == 15);
    // Remember the cursor didn't move after the close
    // so it is at -7,3
    // MoveTo(0,0)
    CHECK(feature.geometry(22) == ((1 << 3) | 1u)); // 9
    CHECK(feature.geometry(23) == 14);
    CHECK(feature.geometry(24) == 5);
    // LineTo(0,10)
    CHECK(feature.geometry(25) == ((3 << 3) | 2u));
    CHECK(feature.geometry(26) == 0);
    CHECK(feature.geometry(27) == 20);
    // LineTo(-10,10)
    CHECK(feature.geometry(28) == 19);
    CHECK(feature.geometry(29) == 0);
    // LineTo(-10,0)
    CHECK(feature.geometry(30) == 0);
    CHECK(feature.geometry(31) == 19);
    // Close
    CHECK(feature.geometry(32) == 15);
    // Remember the cursor didn't move after the close
    // so it is at -10,0
    // MoveTo(-7,7)
    CHECK(feature.geometry(33) == ((1 << 3) | 1u)); // 9
    CHECK(feature.geometry(34) == 6);
    CHECK(feature.geometry(35) == 14);
    // LineTo(-3,7)
    CHECK(feature.geometry(36) == ((3 << 3) | 2u));
    CHECK(feature.geometry(37) == 8);
    CHECK(feature.geometry(38) == 0);
    // LineTo(-3,3)
    CHECK(feature.geometry(39) == 0);
    CHECK(feature.geometry(40) == 7);
    // LineTo(-7,3)
    CHECK(feature.geometry(41) == 7);
    CHECK(feature.geometry(42) == 0);
    // Close
    CHECK(feature.geometry(43) == 15);
}

TEST_CASE("encoding pbf multi polygons with holes -- geometry type")
{
    mapbox::geometry::multi_polygon<std::int64_t> mp;
    {
        mapbox::geometry::polygon<std::int64_t> p0;
        mapbox::geometry::linear_ring<std::int64_t> r0;
        r0.emplace_back(0,0);
        r0.emplace_back(0,10);
        r0.emplace_back(-10,10);
        r0.emplace_back(-10,0);
        r0.emplace_back(0,0);
        p0.push_back(std::move(r0));
        mapbox::geometry::linear_ring<std::int64_t> hole;
        hole.emplace_back(-7,7);
        hole.emplace_back(-3,7);
        hole.emplace_back(-3,3);
        hole.emplace_back(-7,3);
        hole.emplace_back(-7,7);
        p0.push_back(std::move(hole));
        mp.push_back(p0);
    }
    // yeah so its the same polygon -- haters gonna hate.
    {
        mapbox::geometry::polygon<std::int64_t> p0;
        mapbox::geometry::linear_ring<std::int64_t> r0;
        r0.emplace_back(0,0);
        r0.emplace_back(0,10);
        r0.emplace_back(-10,10);
        r0.emplace_back(-10,0);
        r0.emplace_back(0,0);
        p0.push_back(std::move(r0));
        mapbox::geometry::linear_ring<std::int64_t> hole;
        hole.emplace_back(-7,7);
        hole.emplace_back(-3,7);
        hole.emplace_back(-3,3);
        hole.emplace_back(-7,3);
        hole.emplace_back(-7,7);
        p0.push_back(std::move(hole));
        mp.push_back(p0);
    }
    mapbox::geometry::geometry<std::int64_t> geom(mp);

    std::int32_t x = 0;
    std::int32_t y = 0;
    std::string feature_str;
    protozero::pbf_writer feature_writer(feature_str);
    vector_tile::Tile_Feature feature;
    REQUIRE(mapnik::vector_tile_impl::encode_geometry_pbf(geom, feature_writer, x, y));
    feature.ParseFromString(feature_str);
    REQUIRE(feature.type() == vector_tile::Tile_GeomType_POLYGON);
    
    // MoveTo, ParameterInteger, ParameterInteger
    // LineTo, ParameterInteger, ParameterInteger, ParameterInteger, ParameterInteger, ParameterInteger, ParameterInteger
    // Close
    // MoveTo, ParameterInteger, ParameterInteger
    // LineTo, ParameterInteger, ParameterInteger, ParameterInteger, ParameterInteger, ParameterInteger, ParameterInteger
    // Close
    // MoveTo, ParameterInteger, ParameterInteger
    // LineTo, ParameterInteger, ParameterInteger, ParameterInteger, ParameterInteger, ParameterInteger, ParameterInteger
    // Close
    // MoveTo, ParameterInteger, ParameterInteger
    // LineTo, ParameterInteger, ParameterInteger, ParameterInteger, ParameterInteger, ParameterInteger, ParameterInteger
    // Close
    // 12 commands + 32 Params = 44
    REQUIRE(feature.geometry_size() == 44);
    // MoveTo(0,0)
    CHECK(feature.geometry(0) == ((1 << 3) | 1u)); // 9
    CHECK(feature.geometry(1) == 0);
    CHECK(feature.geometry(2) == 0);
    // LineTo(0,10)
    CHECK(feature.geometry(3) == ((3 << 3) | 2u));
    CHECK(feature.geometry(4) == 0);
    CHECK(feature.geometry(5) == 20);
    // LineTo(-10,10)
    CHECK(feature.geometry(6) == 19);
    CHECK(feature.geometry(7) == 0);
    // LineTo(-10,0)
    CHECK(feature.geometry(8) == 0);
    CHECK(feature.geometry(9) == 19);
    // Close
    CHECK(feature.geometry(10) == 15);
    // Remember the cursor didn't move after the close
    // so it is at -10,0
    // MoveTo(-7,7)
    CHECK(feature.geometry(11) == ((1 << 3) | 1u)); // 9
    CHECK(feature.geometry(12) == 6);
    CHECK(feature.geometry(13) == 14);
    // LineTo(-3,7)
    CHECK(feature.geometry(14) == ((3 << 3) | 2u));
    CHECK(feature.geometry(15) == 8);
    CHECK(feature.geometry(16) == 0);
    // LineTo(-3,3)
    CHECK(feature.geometry(17) == 0);
    CHECK(feature.geometry(18) == 7);
    // LineTo(-7,3)
    CHECK(feature.geometry(19) == 7);
    CHECK(feature.geometry(20) == 0);
    // Close
    CHECK(feature.geometry(21) == 15);
    // Remember the cursor didn't move after the close
    // so it is at -7,3
    // MoveTo(0,0)
    CHECK(feature.geometry(22) == ((1 << 3) | 1u)); // 9
    CHECK(feature.geometry(23) == 14);
    CHECK(feature.geometry(24) == 5);
    // LineTo(0,10)
    CHECK(feature.geometry(25) == ((3 << 3) | 2u));
    CHECK(feature.geometry(26) == 0);
    CHECK(feature.geometry(27) == 20);
    // LineTo(-10,10)
    CHECK(feature.geometry(28) == 19);
    CHECK(feature.geometry(29) == 0);
    // LineTo(-10,0)
    CHECK(feature.geometry(30) == 0);
    CHECK(feature.geometry(31) == 19);
    // Close
    CHECK(feature.geometry(32) == 15);
    // Remember the cursor didn't move after the close
    // so it is at -10,0
    // MoveTo(-7,7)
    CHECK(feature.geometry(33) == ((1 << 3) | 1u)); // 9
    CHECK(feature.geometry(34) == 6);
    CHECK(feature.geometry(35) == 14);
    // LineTo(-3,7)
    CHECK(feature.geometry(36) == ((3 << 3) | 2u));
    CHECK(feature.geometry(37) == 8);
    CHECK(feature.geometry(38) == 0);
    // LineTo(-3,3)
    CHECK(feature.geometry(39) == 0);
    CHECK(feature.geometry(40) == 7);
    // LineTo(-7,3)
    CHECK(feature.geometry(41) == 7);
    CHECK(feature.geometry(42) == 0);
    // Close
    CHECK(feature.geometry(43) == 15);
}

TEST_CASE("encoding pbf empty multi polygon")
{
    mapbox::geometry::multi_polygon<std::int64_t> mp;
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::string feature_str;
    protozero::pbf_writer feature_writer(feature_str);
    vector_tile::Tile_Feature feature;
    REQUIRE_FALSE(mapnik::vector_tile_impl::encode_geometry_pbf(mp, feature_writer, x, y));
    feature.ParseFromString(feature_str);
    REQUIRE(feature.has_type());
    CHECK(feature.type() == vector_tile::Tile_GeomType_POLYGON);
    REQUIRE(feature.geometry_size() == 0);
}

TEST_CASE("encoding pbf polygon with degenerate exterior ring full of repeated points")
{
    mapbox::geometry::polygon<std::int64_t> p0;
    mapbox::geometry::linear_ring<std::int64_t> r0;
    // invalid exterior ring
    r0.emplace_back(0,0);
    r0.emplace_back(0,10);
    r0.emplace_back(0,10);
    r0.emplace_back(0,10);
    r0.emplace_back(0,10);
    r0.emplace_back(0,10);
    r0.emplace_back(0,10);
    r0.emplace_back(0,10);
    p0.push_back(std::move(r0));

    std::int32_t x = 0;
    std::int32_t y = 0;
    std::string feature_str;
    protozero::pbf_writer feature_writer(feature_str);
    vector_tile::Tile_Feature feature;
    REQUIRE_FALSE(mapnik::vector_tile_impl::encode_geometry_pbf(p0, feature_writer, x, y));
    feature.ParseFromString(feature_str);
    REQUIRE(feature.has_type());
    CHECK(feature.type() == vector_tile::Tile_GeomType_POLYGON);
    CHECK(feature.geometry_size() == 0);
}

TEST_CASE("encoding pbf polygon with degenerate exterior ring")
{
    mapbox::geometry::polygon<std::int64_t> p0;
    mapbox::geometry::linear_ring<std::int64_t> r0;
    // invalid exterior ring
    r0.emplace_back(0,0);
    r0.emplace_back(0,10);
    p0.push_back(std::move(r0));

    std::int32_t x = 0;
    std::int32_t y = 0;
    std::string feature_str;
    protozero::pbf_writer feature_writer(feature_str);
    vector_tile::Tile_Feature feature;
    REQUIRE_FALSE(mapnik::vector_tile_impl::encode_geometry_pbf(p0, feature_writer, x, y));
    feature.ParseFromString(feature_str);
    REQUIRE(feature.has_type());
    CHECK(feature.type() == vector_tile::Tile_GeomType_POLYGON);
    CHECK(feature.geometry_size() == 0);
}

TEST_CASE("encoding pbf polygon with degenerate exterior ring and interior ring")
{
    mapbox::geometry::polygon<std::int64_t> p0;
    mapbox::geometry::linear_ring<std::int64_t> r0;
    // invalid exterior ring
    r0.emplace_back(0,0);
    r0.emplace_back(0,10);
    p0.push_back(std::move(r0));
    // invalid interior ring -- is counter clockwise
    mapbox::geometry::linear_ring<std::int64_t> hole;
    hole.emplace_back(-7,7);
    hole.emplace_back(-3,7);
    hole.emplace_back(-3,3);
    hole.emplace_back(-7,3);
    hole.emplace_back(-7,7);
    p0.push_back(std::move(hole));

    // encoder should cull the exterior invalid ring, which triggers
    // the entire polygon to be culled.
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::string feature_str;
    protozero::pbf_writer feature_writer(feature_str);
    vector_tile::Tile_Feature feature;
    REQUIRE_FALSE(mapnik::vector_tile_impl::encode_geometry_pbf(p0, feature_writer, x, y));
    feature.ParseFromString(feature_str);
    REQUIRE(feature.has_type());
    CHECK(feature.type() == vector_tile::Tile_GeomType_POLYGON);
    CHECK(feature.geometry_size() == 0);
}

TEST_CASE("encoding pbf polygon with valid exterior ring but degenerate interior ring")
{
    mapbox::geometry::polygon<std::int64_t> p0;
    mapbox::geometry::linear_ring<std::int64_t> r0;
    r0.emplace_back(0,0);
    r0.emplace_back(0,10);
    r0.emplace_back(-10,10);
    r0.emplace_back(-10,0);
    r0.emplace_back(0,0);
    p0.push_back(std::move(r0));
    // invalid interior ring
    mapbox::geometry::linear_ring<std::int64_t> hole;
    hole.emplace_back(-7,7);
    hole.emplace_back(-3,7);
    p0.push_back(std::move(hole));

    std::int32_t x = 0;
    std::int32_t y = 0;
    std::string feature_str;
    protozero::pbf_writer feature_writer(feature_str);
    vector_tile::Tile_Feature feature;
    REQUIRE(mapnik::vector_tile_impl::encode_geometry_pbf(p0, feature_writer, x, y));
    feature.ParseFromString(feature_str);
    REQUIRE(feature.type() == vector_tile::Tile_GeomType_POLYGON);
    
    // MoveTo, ParameterInteger, ParameterInteger
    // LineTo, ParameterInteger, ParameterInteger, ParameterInteger, ParameterInteger, ParameterInteger, ParameterInteger
    // Close
    // 3 commands + 8 Params = 11 
    REQUIRE(feature.geometry_size() == 11);
    // MoveTo(0,0)
    CHECK(feature.geometry(0) == ((1 << 3) | 1u)); // 9
    CHECK(feature.geometry(1) == 0);
    CHECK(feature.geometry(2) == 0);
    // LineTo(0,10)
    CHECK(feature.geometry(3) == ((3 << 3) | 2u));
    CHECK(feature.geometry(4) == 0);
    CHECK(feature.geometry(5) == 20);
    // LineTo(-10,10)
    CHECK(feature.geometry(6) == 19);
    CHECK(feature.geometry(7) == 0);
    // LineTo(-10,0)
    CHECK(feature.geometry(8) == 0);
    CHECK(feature.geometry(9) == 19);
    // Close
    CHECK(feature.geometry(10) == 15);
}

