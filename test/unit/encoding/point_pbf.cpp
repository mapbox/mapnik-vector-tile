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
// Unit tests for geometry encoding of points
//

TEST_CASE("encode pbf simple point")
{
    mapnik::geometry::point<std::int64_t> point(10,10);
    
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::string feature_str;
    protozero::pbf_writer feature_writer(feature_str);
    vector_tile::Tile_Feature feature;
    REQUIRE(mapnik::vector_tile_impl::encode_geometry_pbf(point, feature_writer, x, y));
    feature.ParseFromString(feature_str);
    REQUIRE(feature.type() == vector_tile::Tile_GeomType_POINT);
    
    // MoveTo, ParameterInteger, ParameterInteger
    // Therefore 1 commands + 2 parameters = 3
    REQUIRE(feature.geometry_size() == 3);
    // MoveTo(10,10)
    CHECK(feature.geometry(0) == ((1 << 3) | 1u)); // 9
    CHECK(feature.geometry(1) == 20);
    CHECK(feature.geometry(2) == 20);
}

TEST_CASE("encode pbf simple point -- geometry type")
{
    mapnik::geometry::point<std::int64_t> point(10,10);
    mapnik::geometry::geometry<std::int64_t> geom(point);

    std::int32_t x = 0;
    std::int32_t y = 0;
    std::string feature_str;
    protozero::pbf_writer feature_writer(feature_str);
    vector_tile::Tile_Feature feature;
    REQUIRE(mapnik::vector_tile_impl::encode_geometry_pbf(geom, feature_writer, x, y));
    feature.ParseFromString(feature_str);
    REQUIRE(feature.type() == vector_tile::Tile_GeomType_POINT);
    
    // MoveTo, ParameterInteger, ParameterInteger
    // Therefore 1 commands + 2 parameters = 3
    REQUIRE(feature.geometry_size() == 3);
    // MoveTo(10,10)
    CHECK(feature.geometry(0) == ((1 << 3) | 1u)); // 9
    CHECK(feature.geometry(1) == 20);
    CHECK(feature.geometry(2) == 20);
}

TEST_CASE("encode pbf simple negative point")
{
    mapnik::geometry::point<std::int64_t> point(-10,-10);
    
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::string feature_str;
    protozero::pbf_writer feature_writer(feature_str);
    vector_tile::Tile_Feature feature;
    REQUIRE(mapnik::vector_tile_impl::encode_geometry_pbf(point, feature_writer, x, y));
    feature.ParseFromString(feature_str);
    REQUIRE(feature.type() == vector_tile::Tile_GeomType_POINT);
    
    // MoveTo, ParameterInteger, ParameterInteger
    // Therefore 1 commands + 2 parameters = 3
    REQUIRE( feature.geometry_size() == 3);
    // MoveTo(10,10)
    CHECK(feature.geometry(0) == ((1 << 3) | 1u)); // 9
    CHECK(feature.geometry(1) == 19);
    CHECK(feature.geometry(2) == 19);
}

TEST_CASE("encode pbf simple multi point -- geometry type")
{
    mapnik::geometry::multi_point<std::int64_t> mp;
    mp.add_coord(10,10);
    mp.add_coord(20,20);
    mp.add_coord(30,30);
    mapnik::geometry::geometry<std::int64_t> geom(mp);
    
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::string feature_str;
    protozero::pbf_writer feature_writer(feature_str);
    vector_tile::Tile_Feature feature;
    REQUIRE(mapnik::vector_tile_impl::encode_geometry_pbf(geom, feature_writer, x, y));
    feature.ParseFromString(feature_str);
    REQUIRE(feature.type() == vector_tile::Tile_GeomType_POINT);
    
    // MoveTo, ParameterInteger, ParameterInteger, ParameterInteger, ParameterInteger, ParameterInteger, ParameterInteger
    // Therefore 1 commands + 6 parameters = 7
    REQUIRE( feature.geometry_size() == 7);
    // MoveTo(10,10)
    CHECK(feature.geometry(0) == ((3 << 3) | 1u)); // 25
    CHECK(feature.geometry(1) == 20);
    CHECK(feature.geometry(2) == 20);
    // MoveTo(10,10)
    CHECK(feature.geometry(3) == 20);
    CHECK(feature.geometry(4) == 20);
    // MoveTo(20,20)
    CHECK(feature.geometry(5) == 20);
    CHECK(feature.geometry(6) == 20);
}

TEST_CASE("encode pbf simple multi point")
{
    mapnik::geometry::multi_point<std::int64_t> mp;
    mp.add_coord(10,10);
    mp.add_coord(20,20);
    mp.add_coord(30,30);
    
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::string feature_str;
    protozero::pbf_writer feature_writer(feature_str);
    vector_tile::Tile_Feature feature;
    REQUIRE(mapnik::vector_tile_impl::encode_geometry_pbf(mp, feature_writer, x, y));
    feature.ParseFromString(feature_str);
    REQUIRE(feature.type() == vector_tile::Tile_GeomType_POINT);
    
    // MoveTo, ParameterInteger, ParameterInteger, ParameterInteger, ParameterInteger, ParameterInteger, ParameterInteger
    // Therefore 1 commands + 6 parameters = 7
    REQUIRE( feature.geometry_size() == 7);
    // MoveTo(10,10)
    CHECK(feature.geometry(0) == ((3 << 3) | 1u)); // 25
    CHECK(feature.geometry(1) == 20);
    CHECK(feature.geometry(2) == 20);
    // MoveTo(10,10)
    CHECK(feature.geometry(3) == 20);
    CHECK(feature.geometry(4) == 20);
    // MoveTo(20,20)
    CHECK(feature.geometry(5) == 20);
    CHECK(feature.geometry(6) == 20);
}

TEST_CASE("encode pbf multi point with repeated points")
{
    mapnik::geometry::multi_point<std::int64_t> mp;
    mp.add_coord(10,10);
    mp.add_coord(10,10);
    mp.add_coord(20,20);
    
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::string feature_str;
    protozero::pbf_writer feature_writer(feature_str);
    vector_tile::Tile_Feature feature;
    REQUIRE(mapnik::vector_tile_impl::encode_geometry_pbf(mp, feature_writer, x, y));
    feature.ParseFromString(feature_str);
    REQUIRE(feature.type() == vector_tile::Tile_GeomType_POINT);
    
    // MoveTo, ParameterInteger, ParameterInteger, ParameterInteger, ParameterInteger, ParameterInteger, ParameterInteger
    // Therefore 1 commands + 6 parameters = 7
    REQUIRE( feature.geometry_size() == 7);
    // MoveTo(10,10)
    CHECK(feature.geometry(0) == ((3 << 3) | 1u)); // 25
    CHECK(feature.geometry(1) == 20);
    CHECK(feature.geometry(2) == 20);
    // MoveTo(10,10)
    CHECK(feature.geometry(3) == 0);
    CHECK(feature.geometry(4) == 0);
    // MoveTo(20,20)
    CHECK(feature.geometry(5) == 20);
    CHECK(feature.geometry(6) == 20);
}

TEST_CASE("encode pbf empty multi point geometry")
{
    mapnik::geometry::multi_point<std::int64_t> mp;
    
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::string feature_str;
    protozero::pbf_writer feature_writer(feature_str);
    vector_tile::Tile_Feature feature;
    REQUIRE_FALSE(mapnik::vector_tile_impl::encode_geometry_pbf(mp, feature_writer, x, y));
    feature.ParseFromString(feature_str);
    //REQUIRE(!feature.has_type());
    
    REQUIRE(feature.geometry_size() == 0);
}

