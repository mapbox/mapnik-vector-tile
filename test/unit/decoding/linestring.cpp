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

//
// Unit tests for geometry decoding of linestrings
//

TEST_CASE("decode simple linestring")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_LINESTRING);
    // MoveTo(1,1)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    // LineTo(2,2)
    feature.add_geometry((3 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    // LineTo(10,10)
    feature.add_geometry(protozero::encode_zigzag32(8));
    feature.add_geometry(protozero::encode_zigzag32(8));
    // LineTo(0,10)
    feature.add_geometry(protozero::encode_zigzag32(-10));
    feature.add_geometry(protozero::encode_zigzag32(0));
    
    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);
    
    SECTION("VT Spec v1") 
    {
        auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 1, 0.0, 0.0, 1.0, 1.0);
        std::string wkt0;
        CHECK( test_utils::to_wkt(wkt0,geom) );
        CHECK( wkt0 == "LINESTRING(1 1,2 2,10 10,0 10)");
        CHECK( geom.is<mapnik::geometry::line_string<double> >() );
    }

    SECTION("VT Spec v2")
    {
        auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 2, 0.0, 0.0, 1.0, 1.0);
        std::string wkt0;
        CHECK( test_utils::to_wkt(wkt0,geom) );
        CHECK( wkt0 == "LINESTRING(1 1,2 2,10 10,0 10)");
        CHECK( geom.is<mapnik::geometry::line_string<double> >() );
    }
}

TEST_CASE("decode degenerate line_string only moveto")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_LINESTRING);
    // MoveTo(1,1)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    
    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);

    SECTION("VT Spec v1") 
    {
        auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 1, 0.0, 0.0, 1.0, 1.0);
        CHECK( geom.is<mapnik::geometry::geometry_empty>() );
    }

    SECTION("VT Spec v2") 
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 2, 0.0, 0.0, 1.0, 1.0));
    }
}

TEST_CASE("decode degenerate line_string lineto(0,0)")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_LINESTRING);
    // MoveTo(1,1)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    // LineTo(1,1)
    feature.add_geometry((1 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(0));
    
    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);

    SECTION("VT Spec v1") 
    {
        auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 1, 0.0, 0.0, 1.0, 1.0);
        CHECK( geom.is<mapnik::geometry::geometry_empty>() );
    }

    SECTION("VT Spec v2") 
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 2, 0.0, 0.0, 1.0, 1.0));
    }
}

TEST_CASE("decode line_string with first lineto command having delta zero")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_LINESTRING);
    // MoveTo(1,1)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    // LineTo(1,1),LineTo(2,2)
    feature.add_geometry((2 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    
    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);
    
    SECTION("VT Spec v1") 
    {
        auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 1, 0.0, 0.0, 1.0, 1.0);
        std::string wkt0;
        CHECK( test_utils::to_wkt(wkt0,geom) );
        CHECK( wkt0 == "LINESTRING(1 1,2 2)");
        CHECK( geom.is<mapnik::geometry::line_string<double> >() );
    }

    SECTION("VT Spec v2")
    {
        auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 2, 0.0, 0.0, 1.0, 1.0);
        std::string wkt0;
        CHECK( test_utils::to_wkt(wkt0,geom) );
        CHECK( wkt0 == "LINESTRING(1 1,2 2)");
        CHECK( geom.is<mapnik::geometry::line_string<double> >() );
    }
}

TEST_CASE("decode line_string with second lineto command having delta zero")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_LINESTRING);
    // MoveTo(1,1)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    // LineTo(2,2),LineTo(2,2)
    feature.add_geometry((2 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(0));
    
    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);
    
    SECTION("VT Spec v1") 
    {
        auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 1, 0.0, 0.0, 1.0, 1.0);
        std::string wkt0;
        CHECK( test_utils::to_wkt(wkt0,geom) );
        CHECK( wkt0 == "LINESTRING(1 1,2 2)");
        CHECK( geom.is<mapnik::geometry::line_string<double> >() );
    }

    SECTION("VT Spec v2")
    {
        auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 2, 0.0, 0.0, 1.0, 1.0);
        std::string wkt0;
        CHECK( test_utils::to_wkt(wkt0,geom) );
        CHECK( wkt0 == "LINESTRING(1 1,2 2)");
        CHECK( geom.is<mapnik::geometry::line_string<double> >() );
    }
}

TEST_CASE("decode line_string with third lineto command having delta zero")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_LINESTRING);
    // MoveTo(1,1)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    // LineTo(2,2),LineTo(3,3),LineTo(3,3)
    feature.add_geometry((3 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(0));
    
    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);
    
    SECTION("VT Spec v1") 
    {
        auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 1, 0.0, 0.0, 1.0, 1.0);
        std::string wkt0;
        CHECK( test_utils::to_wkt(wkt0,geom) );
        CHECK( wkt0 == "LINESTRING(1 1,2 2,3 3)");
        CHECK( geom.is<mapnik::geometry::line_string<double> >() );
    }

    SECTION("VT Spec v2")
    {
        auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 2, 0.0, 0.0, 1.0, 1.0);
        std::string wkt0;
        CHECK( test_utils::to_wkt(wkt0,geom) );
        CHECK( wkt0 == "LINESTRING(1 1,2 2,3 3)");
        CHECK( geom.is<mapnik::geometry::line_string<double> >() );
    }
}

TEST_CASE("decode degenerate linestring with close command at end")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_LINESTRING);
    // MoveTo(1,1)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    // LineTo(2,2)
    feature.add_geometry((1 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    // Close Path
    feature.add_geometry(15); // close_path
    
    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);
    
    SECTION("VT Spec v1")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_LINESTRING, 1, 0.0, 0.0, 1.0, 1.0));
    }

    SECTION("VT Spec v2")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_LINESTRING, 2, 0.0, 0.0, 1.0, 1.0));
    }
}

TEST_CASE("decode degenerate linestring with close command first")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_LINESTRING);
    // Close Path
    feature.add_geometry(15); // close_path
    // MoveTo(1,1)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    // LineTo(2,2)
    feature.add_geometry((1 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));

    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);
    
    SECTION("VT Spec v1")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_LINESTRING, 1, 0.0, 0.0, 1.0, 1.0));
    }

    SECTION("VT Spec v2")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_LINESTRING, 2, 0.0, 0.0, 1.0, 1.0));
    }
}

TEST_CASE("decode degenerate linestring with moveto command count greater then 1")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_LINESTRING);
    // MoveTo(1,1)
    feature.add_geometry((2 << 3u) | 1u); // command count 2
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    // MoveTo(2,2)
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    // LineTo(3,3)
    feature.add_geometry((1 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    
    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);
    
    SECTION("VT Spec v1")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_LINESTRING, 1, 0.0, 0.0, 1.0, 1.0));
    }

    SECTION("VT Spec v2")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_LINESTRING, 2, 0.0, 0.0, 1.0, 1.0));
    }
}

TEST_CASE("decode degenerate linestring with moveto command count of zero")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_LINESTRING);
    // MoveTo(1,1)
    feature.add_geometry((0 << 3u) | 1u); // command count 0
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    // LineTo(3,3)
    feature.add_geometry((1 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    
    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);
    
    SECTION("VT Spec v1")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_LINESTRING, 1, 0.0, 0.0, 1.0, 1.0));
    }

    SECTION("VT Spec v2")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_LINESTRING, 2, 0.0, 0.0, 1.0, 1.0));
    }
}

TEST_CASE("decode degenerate linestring with lineto command count of zero")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_LINESTRING);
    // MoveTo(1,1)
    feature.add_geometry((1 << 3u) | 1u);
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    // LineTo command count 0
    feature.add_geometry((0 << 3u) | 2u);
    // LineTo(2,2)
    feature.add_geometry((1 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    
    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);
    
    SECTION("VT Spec v1")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_LINESTRING, 1, 0.0, 0.0, 1.0, 1.0));
    }

    SECTION("VT Spec v2")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_LINESTRING, 2, 0.0, 0.0, 1.0, 1.0));
    }
}

TEST_CASE("decode degenerate linestring that starts with unknown command")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_LINESTRING);
    feature.add_geometry((1 << 3u) | 5u); // invalid command
    // MoveTo(1,1)
    feature.add_geometry((1 << 3u) | 1u);
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    // LineTo(2,2)
    feature.add_geometry((1 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    
    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);
    
    SECTION("VT Spec v1")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_LINESTRING, 1, 0.0, 0.0, 1.0, 1.0));
    }

    SECTION("VT Spec v2")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_LINESTRING, 2, 0.0, 0.0, 1.0, 1.0));
    }
}

TEST_CASE("decode degenerate linestring that ends with unknown command")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_LINESTRING);
    // MoveTo(1,1)
    feature.add_geometry((1 << 3u) | 1u);
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    // LineTo(2,2)
    feature.add_geometry((1 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry((1 << 3u) | 5u); // invalid command
    
    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);
    
    SECTION("VT Spec v1")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_LINESTRING, 1, 0.0, 0.0, 1.0, 1.0));
    }

    SECTION("VT Spec v2")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_LINESTRING, 2, 0.0, 0.0, 1.0, 1.0));
    }
}

TEST_CASE("decode degenerate linestring that begins with lineto")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_LINESTRING);
    // LineTo(1,1)
    feature.add_geometry((1 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    // MoveTo(2,2)
    feature.add_geometry((1 << 3u) | 1u); 
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    // LineTo(3,3)
    feature.add_geometry((1 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    
    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);
    
    SECTION("VT Spec v1")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_LINESTRING, 1, 0.0, 0.0, 1.0, 1.0));
    }

    SECTION("VT Spec v2")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_LINESTRING, 2, 0.0, 0.0, 1.0, 1.0));
    }
}

TEST_CASE("decode degenerate linestring that begins with lineto delta zero")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_LINESTRING);
    // LineTo(0,0)
    feature.add_geometry((1 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // MoveTo(1,1)
    feature.add_geometry((1 << 3u) | 1u); 
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    // LineTo(2,2)
    feature.add_geometry((1 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    
    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);
    
    SECTION("VT Spec v1")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_LINESTRING, 1, 0.0, 0.0, 1.0, 1.0));
    }

    SECTION("VT Spec v2")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_LINESTRING, 2, 0.0, 0.0, 1.0, 1.0));
    }
}

TEST_CASE("decode degenerate linestring that begins with close")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_LINESTRING);
    // Close
    feature.add_geometry(15);
    // MoveTo(1,1)
    feature.add_geometry((1 << 3u) | 1u); 
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    // LineTo(2,2)
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    
    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);
    
    SECTION("VT Spec v1")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_LINESTRING, 1, 0.0, 0.0, 1.0, 1.0));
    }

    SECTION("VT Spec v2")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_LINESTRING, 2, 0.0, 0.0, 1.0, 1.0));
    }
}

TEST_CASE("decode linestring that begins with two moveto commands")
{
    // This should work with v1 but throw with v2
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_LINESTRING);
    // MoveTo(1,1)
    feature.add_geometry((1 << 3u) | 1u);
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    // MoveTo(2,2)
    feature.add_geometry((1 << 3u) | 1u); 
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    // LineTo(3,3)
    feature.add_geometry((1 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    
    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);
    
    SECTION("VT Spec v1")
    {
        auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 1, 0.0, 0.0, 1.0, 1.0);
        std::string wkt0;
        CHECK( test_utils::to_wkt(wkt0,geom) );
        CHECK( wkt0 == "LINESTRING(2 2,3 3)");
        CHECK( geom.is<mapnik::geometry::line_string<double> >() );
    }

    SECTION("VT Spec v2")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_LINESTRING, 2, 0.0, 0.0, 1.0, 1.0));
    }
}
