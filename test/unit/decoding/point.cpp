#include "catch.hpp"

// mapnik vector tile
#include "vector_tile_geometry_decoder.hpp"

// test utils
#include "decoding_util.hpp"
#include "geom_to_wkt.hpp"

// mapnik
#include <mapnik/geometry.hpp>

// libprotobuf
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include "vector_tile.pb.h"
#pragma GCC diagnostic pop

// std
#include <limits>

//
// Unit tests for geometry decoding of points
//

TEST_CASE("decode simple point")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POINT);
    // MoveTo(5,5)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(5));
    feature.add_geometry(protozero::encode_zigzag32(5));
    
    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);

    SECTION("VT Spec v1") 
    {
        auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 1, 0.0, 0.0, 1.0, 1.0);
        std::string wkt0;
        CHECK( test_utils::to_wkt(wkt0,geom) );
        CHECK( wkt0 == "POINT(5 5)");
        CHECK( geom.is<mapnik::geometry::point<double> >() );
    }

    SECTION("VT Spec v2") 
    {
        auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 2, 0.0, 0.0, 1.0, 1.0);
        std::string wkt0;
        CHECK( test_utils::to_wkt(wkt0,geom) );
        CHECK( wkt0 == "POINT(5 5)");
        CHECK( geom.is<mapnik::geometry::point<double> >() );
    }
}

TEST_CASE("decode simple negative point")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POINT);
    // MoveTo(-5,-5)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(-5));
    feature.add_geometry(protozero::encode_zigzag32(-5));
    
    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);

    SECTION("VT Spec v1") 
    {
        auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 1, 0.0, 0.0, 1.0, 1.0);
        std::string wkt0;
        CHECK( test_utils::to_wkt(wkt0,geom) );
        CHECK( wkt0 == "POINT(-5 -5)");
        CHECK( geom.is<mapnik::geometry::point<double> >() );
    }

    SECTION("VT Spec v2") 
    {
        auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 2, 0.0, 0.0, 1.0, 1.0);
        std::string wkt0;
        CHECK( test_utils::to_wkt(wkt0,geom) );
        CHECK( wkt0 == "POINT(-5 -5)");
        CHECK( geom.is<mapnik::geometry::point<double> >() );
    }
}

TEST_CASE("point with delta of max int32")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POINT);
    feature.add_geometry(9); // move_to | (1 << 3)
    std::int64_t max_32t = std::numeric_limits<int32_t>::max();
    feature.add_geometry(protozero::encode_zigzag32(max_32t));
    feature.add_geometry(protozero::encode_zigzag32(max_32t));
    
    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);

    SECTION("VT Spec v1") 
    {
        auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 1, 0.0, 0.0, 1.0, 1.0);
        std::string wkt0;
        CHECK( test_utils::to_wkt(wkt0,geom) );
        CHECK( wkt0 == "POINT(2147483647 2147483647)");
        CHECK( geom.is<mapnik::geometry::point<double> >() );
    }

    SECTION("VT Spec v2") 
    {
        auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 2, 0.0, 0.0, 1.0, 1.0);
        std::string wkt0;
        CHECK( test_utils::to_wkt(wkt0,geom) );
        CHECK( wkt0 == "POINT(2147483647 2147483647)");
        CHECK( geom.is<mapnik::geometry::point<double> >() );
    }
}

TEST_CASE("point with delta of min int32")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POINT);
    feature.add_geometry(9); // move_to | (1 << 3)
    std::int64_t min_32t = std::numeric_limits<int32_t>::min();
    feature.add_geometry(protozero::encode_zigzag32(min_32t));
    feature.add_geometry(protozero::encode_zigzag32(min_32t));
    
    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);

    SECTION("VT Spec v1") 
    {
        auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 1, 0.0, 0.0, 1.0, 1.0);
        std::string wkt0;
        CHECK( test_utils::to_wkt(wkt0,geom) );
        CHECK( wkt0 == "POINT(-2147483648 -2147483648)");
        CHECK( geom.is<mapnik::geometry::point<double> >() );
    }

    SECTION("VT Spec v2") 
    {
        auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 2, 0.0, 0.0, 1.0, 1.0);
        std::string wkt0;
        CHECK( test_utils::to_wkt(wkt0,geom) );
        CHECK( wkt0 == "POINT(-2147483648 -2147483648)");
        CHECK( geom.is<mapnik::geometry::point<double> >() );
    }
}

TEST_CASE("point with delta of min int32 + 1")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POINT);
    feature.add_geometry(9); // move_to | (1 << 3)
    std::int64_t min_32t = std::numeric_limits<int32_t>::min() + 1;
    feature.add_geometry(protozero::encode_zigzag32(min_32t));
    feature.add_geometry(protozero::encode_zigzag32(min_32t));
    
    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);

    SECTION("VT Spec v1") 
    {
        auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 1, 0.0, 0.0, 1.0, 1.0);
        std::string wkt0;
        CHECK( test_utils::to_wkt(wkt0,geom) );
        CHECK( wkt0 == "POINT(-2147483647 -2147483647)");
        CHECK( geom.is<mapnik::geometry::point<double> >() );
    }

    SECTION("VT Spec v2") 
    {
        auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 2, 0.0, 0.0, 1.0, 1.0);
        std::string wkt0;
        CHECK( test_utils::to_wkt(wkt0,geom) );
        CHECK( wkt0 == "POINT(-2147483647 -2147483647)");
        CHECK( geom.is<mapnik::geometry::point<double> >() );
    }
}

TEST_CASE("degenerate point with close command")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POINT);
    
    // MoveTo(1,1)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    // Close Path
    feature.add_geometry(15); // close_path

    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);
    
    SECTION("VT Spec v1") 
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POINT, 1, 0.0, 0.0, 1.0, 1.0));
    }

    SECTION("VT Spec v2") 
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POINT, 2, 0.0, 0.0, 1.0, 1.0));
    }
}

TEST_CASE("degenerate point with lineto command")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POINT);
    // MoveTo(1,1)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    // LineTo(1,1)
    feature.add_geometry((1 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));

    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);
    
    SECTION("VT Spec v1") 
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POINT, 1, 0.0, 0.0, 1.0, 1.0));
    }

    SECTION("VT Spec v2") 
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POINT, 2, 0.0, 0.0, 1.0, 1.0));
    }
}

TEST_CASE("degenerate point with moveto with out enough parameters")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POINT);
    // MoveTo(1,1)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(1));

    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);
    
    SECTION("VT Spec v1") 
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POINT, 1, 0.0, 0.0, 1.0, 1.0));
    }

    SECTION("VT Spec v2") 
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POINT, 2, 0.0, 0.0, 1.0, 1.0));
    }
}

TEST_CASE("degenerate point with moveto with out enough parameters - case 2")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POINT);
    // MoveTo(1,1)
    feature.add_geometry((2 << 3u) | 1u);
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));

    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);
    
    SECTION("VT Spec v1") 
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POINT, 1, 0.0, 0.0, 1.0, 1.0));
    }

    SECTION("VT Spec v2") 
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POINT, 2, 0.0, 0.0, 1.0, 1.0));
    }
}

TEST_CASE("degenerate point with moveto with command count of zero")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POINT);
    // MoveTo(1,1)
    feature.add_geometry((0 << 3u) | 1u);
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));

    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);
    
    SECTION("VT Spec v1") 
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POINT, 1, 0.0, 0.0, 1.0, 1.0));
    }

    SECTION("VT Spec v2") 
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POINT, 2, 0.0, 0.0, 1.0, 1.0));
    }
}

TEST_CASE("degenerate point with invalid command")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POINT);
    // MoveTo(1,1)
    feature.add_geometry((1 << 3u) | 5u); // 5 isn't valid
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));

    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);
    
    SECTION("VT Spec v1") 
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POINT, 1, 0.0, 0.0, 1.0, 1.0));
    }

    SECTION("VT Spec v2") 
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POINT, 2, 0.0, 0.0, 1.0, 1.0));
    }
}

TEST_CASE("multipoint with three movetos with command count 1")
{
    // While this is not the proper way to encode two movetwos we want to make sure
    // that it still works properly in the decoder.
    vector_tile::Tile_Feature feature;
    //feature.set_type(vector_tile::Tile_GeomType_POINT);
    // MoveTo(1,1)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    // MoveTo(2,2)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    // MoveTo(3,3)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));

    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);

    SECTION("VT Spec v2") 
    {
        auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POINT, 2, 0.0, 0.0, 1.0, 1.0);
        std::string wkt0;
        CHECK( test_utils::to_wkt(wkt0,geom) );
        CHECK( wkt0 == "MULTIPOINT(1 1,2 2,3 3)");
        CHECK( geom.is<mapnik::geometry::multi_point<double> >() );
    }

    SECTION("VT Spec v1") 
    {
        auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POINT, 1, 0.0, 0.0, 1.0, 1.0);
        std::string wkt0;
        CHECK( test_utils::to_wkt(wkt0,geom) );
        CHECK( wkt0 == "MULTIPOINT(1 1,2 2,3 3)");
        CHECK( geom.is<mapnik::geometry::multi_point<double> >() );
    }
}
