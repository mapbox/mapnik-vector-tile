#include "catch.hpp"

// mapnik vector tile
#include "vector_tile_geometry_encoder.hpp"

// test utils
#include "encoding_util.hpp"

// mapnik
#include <mapnik/geometry.hpp>
#include <mapnik/util/geometry_to_wkt.hpp>

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

TEST_CASE("encode simple point")
{
    mapnik::geometry::point<std::int64_t> point(10,10);
    
    vector_tile::Tile_Feature feature = geometry_to_feature(point);
    
    // MoveTo, ParameterInteger, ParameterInteger
    // Therefore 1 commands + 2 parameters = 3
    REQUIRE(feature.geometry_size() == 3);
    // MoveTo(10,10)
    CHECK(feature.geometry(0) == ((1 << 3) | 1u)); // 9
    CHECK(feature.geometry(1) == 20);
    CHECK(feature.geometry(2) == 20);
}

TEST_CASE("encode simple negative point")
{
    mapnik::geometry::point<std::int64_t> point(-10,-10);
    
    vector_tile::Tile_Feature feature = geometry_to_feature(point);
    
    // MoveTo, ParameterInteger, ParameterInteger
    // Therefore 1 commands + 2 parameters = 3
    REQUIRE( feature.geometry_size() == 3);
    // MoveTo(10,10)
    CHECK(feature.geometry(0) == ((1 << 3) | 1u)); // 9
    CHECK(feature.geometry(1) == 19);
    CHECK(feature.geometry(2) == 19);
}

TEST_CASE("encode multi point with repeated points")
{
    mapnik::geometry::multi_point<std::int64_t> mp;
    mp.add_coord(10,10);
    mp.add_coord(10,10);
    mp.add_coord(20,20);
    
    vector_tile::Tile_Feature feature = geometry_to_feature(mp);
    
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

TEST_CASE("encode empty multi point geometry")
{
    mapnik::geometry::multi_point<std::int64_t> mp;
    
    vector_tile::Tile_Feature feature = geometry_to_feature(mp);
    
    REQUIRE(feature.geometry_size() == 0);
}

