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
// Unit tests for geometry decoding of polygons
//

TEST_CASE("decode simple polygon")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POLYGON);
    // MoveTo(0,0)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(0,10)
    feature.add_geometry((3 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(10));
    // LineTo(-10,10)
    feature.add_geometry(protozero::encode_zigzag32(-10));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(-10,0)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(-10));
    // Close
    feature.add_geometry(15);

    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);

    SECTION("VT Spec v1")
    {
        auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 1, 0.0, 0.0, 1.0, 1.0);
        std::string wkt0;
        CHECK( test_utils::to_wkt(wkt0,geom) );
        CHECK( wkt0 == "POLYGON((0 0,0 10,-10 10,-10 0,0 0))");
        CHECK( geom.is<mapnik::geometry::polygon<double> >() );
    }

    SECTION("VT Spec v2")
    {
        auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 2, 0.0, 0.0, 1.0, 1.0);
        std::string wkt0;
        CHECK( test_utils::to_wkt(wkt0,geom) );
        CHECK( wkt0 == "POLYGON((0 0,0 10,-10 10,-10 0,0 0))");
        CHECK( geom.is<mapnik::geometry::polygon<double> >() );
    }
}

TEST_CASE("decode simple polygon - int64 decode")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POLYGON);
    // MoveTo(0,0)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(0,10)
    feature.add_geometry((3 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(10));
    // LineTo(-10,10)
    feature.add_geometry(protozero::encode_zigzag32(-10));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(-10,0)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(-10));
    // Close
    feature.add_geometry(15);

    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);

    SECTION("VT Spec v1")
    {
        auto geom = mapnik::vector_tile_impl::decode_geometry<std::int64_t>(geoms, feature.type(), 1, 0.0, 0.0, 1.0, 1.0);
        std::string wkt0;
        CHECK( test_utils::to_wkt(wkt0,geom) );
        CHECK( wkt0 == "POLYGON((0 0,0 10,-10 10,-10 0,0 0))");
        CHECK( geom.is<mapnik::geometry::polygon<std::int64_t> >() );
    }

    SECTION("VT Spec v2")
    {
        auto geom = mapnik::vector_tile_impl::decode_geometry<std::int64_t>(geoms, feature.type(), 2, 0.0, 0.0, 1.0, 1.0);
        std::string wkt0;
        CHECK( test_utils::to_wkt(wkt0,geom) );
        CHECK( wkt0 == "POLYGON((0 0,0 10,-10 10,-10 0,0 0))");
        CHECK( geom.is<mapnik::geometry::polygon<std::int64_t> >() );
    }
}

TEST_CASE("decode simple polygon with hole")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POLYGON);
    // This ring is counter clockwise
    // MoveTo(0,0)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(0,10)
    feature.add_geometry((3 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(10));
    // LineTo(-10,10)
    feature.add_geometry(protozero::encode_zigzag32(-10));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(-10,0)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(-10));
    // Close
    feature.add_geometry(15);
    // This ring is clockwise!
    // Cursor is still at -10,0
    // MoveTo(-7,7)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(3));
    feature.add_geometry(protozero::encode_zigzag32(7));
    // LineTo(-3,7)
    feature.add_geometry((3 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(4));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(-3,3)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(-4));
    // LineTo(-7,3)
    feature.add_geometry(protozero::encode_zigzag32(-4));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // Close
    feature.add_geometry(15);

    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);

    SECTION("VT Spec v1")
    {
        auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 1, 0.0, 0.0, 1.0, 1.0);
        std::string wkt0;
        CHECK( test_utils::to_wkt(wkt0,geom) );
        CHECK( wkt0 == "POLYGON((0 0,0 10,-10 10,-10 0,0 0),(-7 7,-3 7,-3 3,-7 3,-7 7))");
        CHECK( geom.is<mapnik::geometry::polygon<double> >() );
    }

    SECTION("VT Spec v2")
    {
        auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 2, 0.0, 0.0, 1.0, 1.0);
        std::string wkt0;
        CHECK( test_utils::to_wkt(wkt0,geom) );
        CHECK( wkt0 == "POLYGON((0 0,0 10,-10 10,-10 0,0 0),(-7 7,-3 7,-3 3,-7 3,-7 7))");
        CHECK( geom.is<mapnik::geometry::polygon<double> >() );
    }
}

TEST_CASE("decode simple multipolygon")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POLYGON);
    // This ring is counter clockwise
    // MoveTo(0,0)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(0,10)
    feature.add_geometry((3 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(10));
    // LineTo(-10,10)
    feature.add_geometry(protozero::encode_zigzag32(-10));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(-10,0)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(-10));
    // Close
    feature.add_geometry(15);
    // This ring is counter clockwise -- so it is not a hole but a new polygon
    // Cursor is still at -10,0
    // MoveTo(-7,7)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(3));
    feature.add_geometry(protozero::encode_zigzag32(7));
    // LineTo(-7,3)
    feature.add_geometry((3 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(-4));
    // LineTo(-3,3)
    feature.add_geometry(protozero::encode_zigzag32(4));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(-3,7)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(4));
    // Close
    feature.add_geometry(15);

    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);

    SECTION("VT Spec v1")
    {
        auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 1, 0.0, 0.0, 1.0, 1.0);
        std::string wkt0;
        CHECK( test_utils::to_wkt(wkt0,geom) );
        CHECK( wkt0 == "MULTIPOLYGON(((0 0,0 10,-10 10,-10 0,0 0)),((-7 7,-7 3,-3 3,-3 7,-7 7)))");
        CHECK( geom.is<mapnik::geometry::multi_polygon<double> >() );
    }

    SECTION("VT Spec v2")
    {
        auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 2, 0.0, 0.0, 1.0, 1.0);
        std::string wkt0;
        CHECK( test_utils::to_wkt(wkt0,geom) );
        CHECK( wkt0 == "MULTIPOLYGON(((0 0,0 10,-10 10,-10 0,0 0)),((-7 7,-7 3,-3 3,-3 7,-7 7)))");
        CHECK( geom.is<mapnik::geometry::multi_polygon<double> >() );
    }
}

TEST_CASE("decode polygon with hole where winding orders are reversed.")
{
    // This should work with v1 parser
    // but fail with the v2 parsers.
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POLYGON);
    // This ring is clockwise
    // MoveTo(0,0)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(-10,0)
    feature.add_geometry((3 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(-10));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(-10,10)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(10));
    // LineTo(0,10)
    feature.add_geometry(protozero::encode_zigzag32(10));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // Close
    feature.add_geometry(15);
    // This ring is counter clockwise -- so it is not a hole but a new polygon
    // Cursor is still at 0,10
    // MoveTo(-7,7)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(-7));
    feature.add_geometry(protozero::encode_zigzag32(-3));
    // LineTo(-7,3)
    feature.add_geometry((3 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(-4));
    // LineTo(-3,3)
    feature.add_geometry(protozero::encode_zigzag32(4));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(-3,7)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(4));
    // Close
    feature.add_geometry(15);

    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);

    SECTION("VT Spec v1")
    {
        auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 1, 0.0, 0.0, 1.0, 1.0);
        std::string wkt0;
        CHECK( test_utils::to_wkt(wkt0,geom) );
        CHECK( wkt0 == "POLYGON((0 0,0 10,-10 10,-10 0,0 0),(-7 7,-3 7,-3 3,-7 3,-7 7))");
        CHECK( geom.is<mapnik::geometry::polygon<double> >() );
    }

    SECTION("VT Spec v2")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 2, 0.0, 0.0, 1.0, 1.0));
    }
}

TEST_CASE("decode simple multi polygon with hole")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POLYGON);
    // This ring is counter clockwise
    // MoveTo(0,0)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(0,10)
    feature.add_geometry((3 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(10));
    // LineTo(-10,10)
    feature.add_geometry(protozero::encode_zigzag32(-10));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(-10,0)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(-10));
    // Close
    feature.add_geometry(15);
    // This ring is clockwise!
    // Cursor is still at -10,0
    // MoveTo(-7,7)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(3));
    feature.add_geometry(protozero::encode_zigzag32(7));
    // LineTo(-3,7)
    feature.add_geometry((3 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(4));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(-3,3)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(-4));
    // LineTo(-7,3)
    feature.add_geometry(protozero::encode_zigzag32(-4));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // Close
    feature.add_geometry(15);
    // This ring is counter clockwise
    // Cursor is still at -7,3
    // MoveTo(0,0)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(7));
    feature.add_geometry(protozero::encode_zigzag32(-3));
    // LineTo(0,10)
    feature.add_geometry((3 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(10));
    // LineTo(-10,10)
    feature.add_geometry(protozero::encode_zigzag32(-10));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(-10,0)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(-10));
    // Close
    feature.add_geometry(15);
    // This ring is clockwise!
    // Cursor is still at -10,0
    // MoveTo(-7,7)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(3));
    feature.add_geometry(protozero::encode_zigzag32(7));
    // LineTo(-3,7)
    feature.add_geometry((3 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(4));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(-3,3)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(-4));
    // LineTo(-7,3)
    feature.add_geometry(protozero::encode_zigzag32(-4));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // Close
    feature.add_geometry(15);

    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);

    SECTION("VT Spec v1")
    {
        auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 1, 0.0, 0.0, 1.0, 1.0);
        std::string wkt0;
        CHECK( test_utils::to_wkt(wkt0,geom) );
        CHECK( wkt0 == "MULTIPOLYGON(((0 0,0 10,-10 10,-10 0,0 0),(-7 7,-3 7,-3 3,-7 3,-7 7)),((0 0,0 10,-10 10,-10 0,0 0),(-7 7,-3 7,-3 3,-7 3,-7 7)))");
        CHECK( geom.is<mapnik::geometry::multi_polygon<double> >() );
    }

    SECTION("VT Spec v2")
    {
        auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 2, 0.0, 0.0, 1.0, 1.0);
        std::string wkt0;
        CHECK( test_utils::to_wkt(wkt0,geom) );
        CHECK( wkt0 == "MULTIPOLYGON(((0 0,0 10,-10 10,-10 0,0 0),(-7 7,-3 7,-3 3,-7 3,-7 7)),((0 0,0 10,-10 10,-10 0,0 0),(-7 7,-3 7,-3 3,-7 3,-7 7)))");
        CHECK( geom.is<mapnik::geometry::multi_polygon<double> >() );
    }
}

TEST_CASE("decode multi polygon with holes - first ring invalid")
{
    // One of the problems with the v1 logic is that because we are
    // not sure about the correct winding order if we run into an exterior ring
    // that is invalid in the first ring it could reverse all the rest of the winding orders.
    // Within v2 this should simply throw.
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POLYGON);
    // This ring is bad exterior ring. For v1, it will make the next interior ring as the first
    // exterior ring!
    // MoveTo(0,0)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(-10,0)
    feature.add_geometry((1 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(-10));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // Close
    feature.add_geometry(15);
    // This ring is clockwise!
    // Cursor is still at -10,0
    // MoveTo(-7,7)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(3));
    feature.add_geometry(protozero::encode_zigzag32(7));
    // LineTo(-3,7)
    feature.add_geometry((3 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(4));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(-3,3)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(-4));
    // LineTo(-7,3)
    feature.add_geometry(protozero::encode_zigzag32(-4));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // Close
    feature.add_geometry(15);
    // This ring is counter clockwise
    // Cursor is still at -7,3
    // MoveTo(0,0)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(7));
    feature.add_geometry(protozero::encode_zigzag32(-3));
    // LineTo(0,10)
    feature.add_geometry((3 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(10));
    // LineTo(-10,10)
    feature.add_geometry(protozero::encode_zigzag32(-10));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(-10,0)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(-10));
    // Close
    feature.add_geometry(15);
    // This ring is clockwise!
    // Cursor is still at -10,0
    // MoveTo(-7,7)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(3));
    feature.add_geometry(protozero::encode_zigzag32(7));
    // LineTo(-3,7)
    feature.add_geometry((3 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(4));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(-3,3)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(-4));
    // LineTo(-7,3)
    feature.add_geometry(protozero::encode_zigzag32(-4));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // Close
    feature.add_geometry(15);

    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);

    SECTION("VT Spec v1")
    {
        auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 1, 0.0, 0.0, 1.0, 1.0);
        std::string wkt0;
        CHECK( test_utils::to_wkt(wkt0,geom) );
        CHECK( wkt0 == "MULTIPOLYGON(((-7 7,-7 3,-3 3,-3 7,-7 7),(0 0,-10 0,-10 10,0 10,0 0)),((-7 7,-7 3,-3 3,-3 7,-7 7)))");
        CHECK( geom.is<mapnik::geometry::multi_polygon<double> >() );
    }

    SECTION("VT Spec v2")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 2, 0.0, 0.0, 1.0, 1.0));
    }
}

TEST_CASE("decode simple polygon -- incorrect exterior winding order")
{
    // winding order is clockwise
    // v1 will reverse the order
    // v2 will throw as invalid
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POLYGON);
    // MoveTo(0,0)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(0,10)
    feature.add_geometry((3 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(10));
    // LineTo(10,10)
    feature.add_geometry(protozero::encode_zigzag32(10));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(10,0)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(-10));
    // Close
    feature.add_geometry(15);

    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);

    SECTION("VT Spec v1")
    {
        auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 1, 0.0, 0.0, 1.0, 1.0);
        std::string wkt0;
        CHECK( test_utils::to_wkt(wkt0,geom) );
        CHECK( wkt0 == "POLYGON((0 0,10 0,10 10,0 10,0 0))");
        CHECK( geom.is<mapnik::geometry::polygon<double> >() );
    }

    SECTION("VT Spec v2")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POLYGON, 2, 0.0, 0.0, 1.0, 1.0));
    }
}

TEST_CASE("decode polygon - only moveto")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POLYGON);
    // MoveTo(1,1)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));

    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);

    SECTION("VT Spec v1")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POLYGON, 1, 0.0, 0.0, 1.0, 1.0));
    }
    
    SECTION("VT Spec v2")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POLYGON, 2, 0.0, 0.0, 1.0, 1.0));
    }
}

TEST_CASE("decode polygon - only moveto and close")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POLYGON);
    // MoveTo(1,1)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    // Close
    feature.add_geometry(15);

    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);

    SECTION("VT Spec v1")
    {
        auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 1, 0.0, 0.0, 1.0, 1.0);
        CHECK( geom.is<mapnik::geometry::geometry_empty>() );
    }
    
    SECTION("VT Spec v2")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POLYGON, 2, 0.0, 0.0, 1.0, 1.0));
    }
}

TEST_CASE("decode polygon - only moveto and close followed by close")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POLYGON);
    // MoveTo(1,1)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    // Close
    feature.add_geometry(15);
    // Close
    feature.add_geometry(15);

    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);

    SECTION("VT Spec v1")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POLYGON, 1, 0.0, 0.0, 1.0, 1.0));
    }
    
    SECTION("VT Spec v2")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POLYGON, 2, 0.0, 0.0, 1.0, 1.0));
    }
}

TEST_CASE("decode polygon - only moveto and close followed by lineto")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POLYGON);
    // MoveTo(1,1)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    // Close
    feature.add_geometry(15);
    // LineTo(2,2)
    feature.add_geometry((1 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));

    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);

    SECTION("VT Spec v1")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POLYGON, 1, 0.0, 0.0, 1.0, 1.0));
    }
    
    SECTION("VT Spec v2")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POLYGON, 2, 0.0, 0.0, 1.0, 1.0));
    }
}

TEST_CASE("decode polygon - only moveto and close followed by lineto -- delta zero")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POLYGON);
    // MoveTo(1,1)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    // Close
    feature.add_geometry(15);
    // LineTo(2,2)
    feature.add_geometry((1 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(0));

    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);

    SECTION("VT Spec v1")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POLYGON, 1, 0.0, 0.0, 1.0, 1.0));
    }
    
    SECTION("VT Spec v2")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POLYGON, 2, 0.0, 0.0, 1.0, 1.0));
    }
}

TEST_CASE("decode polygon - moveto and close followed by real polygon")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POLYGON);
    // MoveTo(1,1)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    // Close
    feature.add_geometry(15);
    // MoveTo(0,0)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(-1));
    feature.add_geometry(protozero::encode_zigzag32(-1));
    // LineTo(0,10)
    feature.add_geometry((3 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(10));
    // LineTo(-10,10)
    feature.add_geometry(protozero::encode_zigzag32(-10));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(-10,0)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(-10));
    // Close
    feature.add_geometry(15);

    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);

    SECTION("VT Spec v1")
    {
        auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 1, 0.0, 0.0, 1.0, 1.0);
        std::string wkt0;
        CHECK( test_utils::to_wkt(wkt0,geom) );
        CHECK( wkt0 == "POLYGON((0 0,0 10,-10 10,-10 0,0 0))");
        CHECK( geom.is<mapnik::geometry::polygon<double> >() );
    }

    SECTION("VT Spec v2")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POLYGON, 2, 0.0, 0.0, 1.0, 1.0));
    }
}

TEST_CASE("decode polygon - only moveto and lineto")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POLYGON);
    // MoveTo(1,1)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    // LineTo(10,0)
    feature.add_geometry((1 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(10));
    feature.add_geometry(protozero::encode_zigzag32(0));

    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);

    SECTION("VT Spec v1")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POLYGON, 1, 0.0, 0.0, 1.0, 1.0));
    }
    
    SECTION("VT Spec v2")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POLYGON, 2, 0.0, 0.0, 1.0, 1.0));
    }
}

TEST_CASE("decode polygon - moveto and lineto followed by real polygon")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POLYGON);
    // MoveTo(1,1)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    // LineTo(2,2)
    feature.add_geometry((1 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    // MoveTo(0,0)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(-2));
    feature.add_geometry(protozero::encode_zigzag32(-2));
    // LineTo(0,10)
    feature.add_geometry((3 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(10));
    // LineTo(-10,10)
    feature.add_geometry(protozero::encode_zigzag32(-10));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(-10,0)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(-10));
    // Close
    feature.add_geometry(15);

    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);

    SECTION("VT Spec v1")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POLYGON, 1, 0.0, 0.0, 1.0, 1.0));
    }

    SECTION("VT Spec v2")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POLYGON, 2, 0.0, 0.0, 1.0, 1.0));
    }
}


TEST_CASE("decode polygon - only moveto lineto and close")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POLYGON);
    // MoveTo(1,1)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    // LineTo(10,0)
    feature.add_geometry((1 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(10));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // Close
    feature.add_geometry(15);

    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);

    SECTION("VT Spec v1")
    {
        auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 1, 0.0, 0.0, 1.0, 1.0);
        CHECK( geom.is<mapnik::geometry::geometry_empty>() );
    }
    
    SECTION("VT Spec v2")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POLYGON, 2, 0.0, 0.0, 1.0, 1.0));
    }
}

TEST_CASE("decode polygon - only moveto, lineto, and close followed by close")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POLYGON);
    // MoveTo(1,1)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    // LineTo(10,0)
    feature.add_geometry((1 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(10));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // Close
    feature.add_geometry(15);
    // Close
    feature.add_geometry(15);

    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);

    SECTION("VT Spec v1")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POLYGON, 1, 0.0, 0.0, 1.0, 1.0));
    }
    
    SECTION("VT Spec v2")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POLYGON, 2, 0.0, 0.0, 1.0, 1.0));
    }
}

TEST_CASE("decode polygon - only moveto, lineto, and close followed by lineto")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POLYGON);
    // MoveTo(1,1)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    // LineTo(10,0)
    feature.add_geometry((1 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(10));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // Close
    feature.add_geometry(15);
    // LineTo(11,1)
    feature.add_geometry((1 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
        
    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);

    SECTION("VT Spec v1")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POLYGON, 1, 0.0, 0.0, 1.0, 1.0));
    }
    
    SECTION("VT Spec v2")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POLYGON, 2, 0.0, 0.0, 1.0, 1.0));
    }
}

TEST_CASE("decode polygon - only moveto, lineto, and close followed by lineto -- delta zero")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POLYGON);
    // MoveTo(1,1)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    // LineTo(10,0)
    feature.add_geometry((1 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(10));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // Close
    feature.add_geometry(15);
    // LineTo(10,0)
    feature.add_geometry((1 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(0));

    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);

    SECTION("VT Spec v1")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POLYGON, 1, 0.0, 0.0, 1.0, 1.0));
    }
    
    SECTION("VT Spec v2")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POLYGON, 2, 0.0, 0.0, 1.0, 1.0));
    }
}

TEST_CASE("decode polygon - moveto lineto and close followed by real polygon")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POLYGON);
    // MoveTo(1,1)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    // LineTo(2,2)
    feature.add_geometry((1 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    // Close
    feature.add_geometry(15);
    // MoveTo(0,0)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(-2));
    feature.add_geometry(protozero::encode_zigzag32(-2));
    // LineTo(0,10)
    feature.add_geometry((3 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(10));
    // LineTo(-10,10)
    feature.add_geometry(protozero::encode_zigzag32(-10));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(-10,0)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(-10));
    // Close
    feature.add_geometry(15);

    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);

    SECTION("VT Spec v1")
    {
        auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 1, 0.0, 0.0, 1.0, 1.0);
        std::string wkt0;
        CHECK( test_utils::to_wkt(wkt0,geom) );
        CHECK( wkt0 == "POLYGON((0 0,0 10,-10 10,-10 0,0 0))");
        CHECK( geom.is<mapnik::geometry::polygon<double> >() );
    }

    SECTION("VT Spec v2")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POLYGON, 2, 0.0, 0.0, 1.0, 1.0));
    }
}

TEST_CASE("decode polygon - only moveto, lineto, lineto and close - both delta zero one command")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POLYGON);
    // MoveTo(0,0)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(0,0)
    feature.add_geometry((2 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(0,0)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // Close
    feature.add_geometry(15);

    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);

    SECTION("VT Spec v1")
    {
        auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 1, 0.0, 0.0, 1.0, 1.0);
        CHECK( geom.is<mapnik::geometry::geometry_empty>() );
    }
    
    SECTION("VT Spec v2")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POLYGON, 2, 0.0, 0.0, 1.0, 1.0));
    }
}

TEST_CASE("decode polygon - only moveto, lineto, lineto and close - both delta zero two commands")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POLYGON);
    // MoveTo(0,0)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(0,0)
    feature.add_geometry((1 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(0,0)
    feature.add_geometry((1 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // Close
    feature.add_geometry(15);

    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);

    SECTION("VT Spec v1")
    {
        auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 1, 0.0, 0.0, 1.0, 1.0);
        CHECK( geom.is<mapnik::geometry::geometry_empty>() );
    }
    
    SECTION("VT Spec v2")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POLYGON, 2, 0.0, 0.0, 1.0, 1.0));
    }
}

TEST_CASE("decode polygon - only moveto, lineto, lineto and close - first delta zero")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POLYGON);
    // MoveTo(0,0)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(0,0)
    feature.add_geometry((1 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(1,1)
    feature.add_geometry((1 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    // Close
    feature.add_geometry(15);

    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);

    SECTION("VT Spec v1")
    {
        auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 1, 0.0, 0.0, 1.0, 1.0);
        CHECK( geom.is<mapnik::geometry::geometry_empty>() );
    }
    
    SECTION("VT Spec v2")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POLYGON, 2, 0.0, 0.0, 1.0, 1.0));
    }
}

TEST_CASE("decode polygon - only moveto, lineto, lineto and close - second delta zero")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POLYGON);
    // MoveTo(0,0)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(1,1)
    feature.add_geometry((1 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    // LineTo(1,1)
    feature.add_geometry((1 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // Close
    feature.add_geometry(15);

    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);

    SECTION("VT Spec v1")
    {
        auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 1, 0.0, 0.0, 1.0, 1.0);
        CHECK( geom.is<mapnik::geometry::geometry_empty>() );
    }
    
    SECTION("VT Spec v2")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POLYGON, 2, 0.0, 0.0, 1.0, 1.0));
    }
}

TEST_CASE("decode polygon - only moveto, lineto, lineto and close - lineto two commands")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POLYGON);
    // MoveTo(0,0)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(10,0)
    feature.add_geometry((1 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(10));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(10,10)
    feature.add_geometry((1 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(10));
    // Close
    feature.add_geometry(15);

    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);

    SECTION("VT Spec v1")
    {
        auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 1, 0.0, 0.0, 1.0, 1.0);
        std::string wkt0;
        CHECK( test_utils::to_wkt(wkt0,geom) );
        CHECK( wkt0 == "POLYGON((0 0,10 0,10 10,0 0))");
        CHECK( geom.is<mapnik::geometry::polygon<double> >() );
    }

    SECTION("VT Spec v2")
    {
        auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 2, 0.0, 0.0, 1.0, 1.0);
        std::string wkt0;
        CHECK( test_utils::to_wkt(wkt0,geom) );
        CHECK( wkt0 == "POLYGON((0 0,10 0,10 10,0 0))");
        CHECK( geom.is<mapnik::geometry::polygon<double> >() );
    }
}

TEST_CASE("decode polygon - only moveto, lineto, lineto and close - lineto one command")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POLYGON);
    // MoveTo(0,0)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(10,0)
    feature.add_geometry((2 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(10));
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(10));
    // Close
    feature.add_geometry(15);

    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);

    SECTION("VT Spec v1")
    {
        auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 1, 0.0, 0.0, 1.0, 1.0);
        std::string wkt0;
        CHECK( test_utils::to_wkt(wkt0,geom) );
        CHECK( wkt0 == "POLYGON((0 0,10 0,10 10,0 0))");
        CHECK( geom.is<mapnik::geometry::polygon<double> >() );
    }

    SECTION("VT Spec v2")
    {
        auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 2, 0.0, 0.0, 1.0, 1.0);
        std::string wkt0;
        CHECK( test_utils::to_wkt(wkt0,geom) );
        CHECK( wkt0 == "POLYGON((0 0,10 0,10 10,0 0))");
        CHECK( geom.is<mapnik::geometry::polygon<double> >() );
    }
}

TEST_CASE("decode polygon -- moveto command count zero")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POLYGON);
    // MoveTo(0,0)
    feature.add_geometry((0 << 3u) | 1u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(0,10)
    feature.add_geometry((3 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(10));
    // LineTo(-10,10)
    feature.add_geometry(protozero::encode_zigzag32(-10));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(-10,0)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(-10));
    // Close
    feature.add_geometry(15);

    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);

    SECTION("VT Spec v1")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POLYGON, 1, 0.0, 0.0, 1.0, 1.0));
    }

    SECTION("VT Spec v2")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POLYGON, 2, 0.0, 0.0, 1.0, 1.0));
    }
}

TEST_CASE("decode polygon -- moveto command count two")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POLYGON);
    // MoveTo(0,0)
    feature.add_geometry((2 << 3u) | 1u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(0,10)
    feature.add_geometry((3 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(10));
    // LineTo(-10,10)
    feature.add_geometry(protozero::encode_zigzag32(-10));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(-10,0)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(-10));
    // Close
    feature.add_geometry(15);

    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);

    SECTION("VT Spec v1")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POLYGON, 1, 0.0, 0.0, 1.0, 1.0));
    }

    SECTION("VT Spec v2")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POLYGON, 2, 0.0, 0.0, 1.0, 1.0));
    }
}

TEST_CASE("decode polygon -- lineto command count 0")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POLYGON);
    // MoveTo(0,0)
    feature.add_geometry((1 << 3u) | 1u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo no commands
    feature.add_geometry((0 << 3u) | 2u);
    // LineTo(0,10)
    feature.add_geometry((3 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(10));
    // LineTo(-10,10)
    feature.add_geometry(protozero::encode_zigzag32(-10));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(-10,0)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(-10));
    // Close
    feature.add_geometry(15);

    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);

    SECTION("VT Spec v1")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POLYGON, 1, 0.0, 0.0, 1.0, 1.0));
    }

    SECTION("VT Spec v2")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POLYGON, 2, 0.0, 0.0, 1.0, 1.0));
    }
}

TEST_CASE("decode polygon -- lineto command count 2 when it should be 3")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POLYGON);
    // MoveTo(0,0)
    feature.add_geometry((1 << 3u) | 1u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(0,10)
    feature.add_geometry((2 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(10));
    // LineTo(-10,10)
    feature.add_geometry(protozero::encode_zigzag32(-10));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(-10,0)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(-10));
    // Close
    feature.add_geometry(15);

    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);

    SECTION("VT Spec v1")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POLYGON, 1, 0.0, 0.0, 1.0, 1.0));
    }

    SECTION("VT Spec v2")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POLYGON, 2, 0.0, 0.0, 1.0, 1.0));
    }
}

TEST_CASE("decode polygon -- lineto command count 4 when it should be 3")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POLYGON);
    // MoveTo(0,0)
    feature.add_geometry((1 << 3u) | 1u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(0,10)
    feature.add_geometry((4 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(10));
    // LineTo(-10,10)
    feature.add_geometry(protozero::encode_zigzag32(-10));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(-10,0)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(-10));
    // Close
    feature.add_geometry(15);

    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);

    SECTION("VT Spec v1")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POLYGON, 1, 0.0, 0.0, 1.0, 1.0));
    }

    SECTION("VT Spec v2")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POLYGON, 2, 0.0, 0.0, 1.0, 1.0));
    }
}

TEST_CASE("decode polygon -- close is first command")
{
    // It is important that in this test the lineto has dx and dy of 0.
    // This checks that the skip dx,dy 0 on lineto doesn't prevent
    // a series of bad commands from throwing.
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POLYGON);
    // Close
    feature.add_geometry(15);
    // MoveTo(0,0)
    feature.add_geometry((1 << 3u) | 1u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(0,10)
    feature.add_geometry((3 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(10));
    // LineTo(-10,10)
    feature.add_geometry(protozero::encode_zigzag32(-10));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(-10,0)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(-10));
    // Close
    feature.add_geometry(15);

    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);

    SECTION("VT Spec v1")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POLYGON, 1, 0.0, 0.0, 1.0, 1.0));
    }

    SECTION("VT Spec v2")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POLYGON, 2, 0.0, 0.0, 1.0, 1.0));
    }
}

TEST_CASE("decode polygon -- extra close command")
{
    // It is important that in this test the lineto has dx and dy of 0.
    // This checks that the skip dx,dy 0 on lineto doesn't prevent
    // a series of bad commands from throwing.
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POLYGON);
    // MoveTo(0,0)
    feature.add_geometry((1 << 3u) | 1u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(0,10)
    feature.add_geometry((3 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(10));
    // LineTo(-10,10)
    feature.add_geometry(protozero::encode_zigzag32(-10));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(-10,0)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(-10));
    // Close
    feature.add_geometry(15);
    // Close
    feature.add_geometry(15);

    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);

    SECTION("VT Spec v1")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POLYGON, 1, 0.0, 0.0, 1.0, 1.0));
    }

    SECTION("VT Spec v2")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POLYGON, 2, 0.0, 0.0, 1.0, 1.0));
    }
}

TEST_CASE("decode polygon -- lineto is first command -- delta zero")
{
    // It is important that in this test the lineto has dx and dy of 0.
    // This checks that the skip dx,dy 0 on lineto doesn't prevent
    // a series of bad commands from throwing.
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POLYGON);
    // LineTo(0,0)
    feature.add_geometry((1 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // MoveTo(0,0)
    feature.add_geometry((1 << 3u) | 1u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(0,10)
    feature.add_geometry((3 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(10));
    // LineTo(-10,10)
    feature.add_geometry(protozero::encode_zigzag32(-10));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(-10,0)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(-10));
    // Close
    feature.add_geometry(15);

    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);

    SECTION("VT Spec v1")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POLYGON, 1, 0.0, 0.0, 1.0, 1.0));
    }

    SECTION("VT Spec v2")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POLYGON, 2, 0.0, 0.0, 1.0, 1.0));
    }
}

TEST_CASE("decode polygon -- lineto is first command")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POLYGON);
    // LineTo(1,1)
    feature.add_geometry((1 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));
    // MoveTo(0,0)
    feature.add_geometry((1 << 3u) | 1u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(0,10)
    feature.add_geometry((3 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(10));
    // LineTo(-10,10)
    feature.add_geometry(protozero::encode_zigzag32(-10));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(-10,0)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(-10));
    // Close
    feature.add_geometry(15);

    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);

    SECTION("VT Spec v1")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POLYGON, 1, 0.0, 0.0, 1.0, 1.0));
    }

    SECTION("VT Spec v2")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POLYGON, 2, 0.0, 0.0, 1.0, 1.0));
    }
}

TEST_CASE("decode polygon -- lineto is last command -- delta zero")
{
    // It is important that in this test the lineto has dx and dy of 0.
    // This checks that the skip dx,dy 0 on lineto doesn't prevent
    // a series of bad commands from throwing.
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POLYGON);
    // MoveTo(0,0)
    feature.add_geometry((1 << 3u) | 1u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(0,10)
    feature.add_geometry((3 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(10));
    // LineTo(-10,10)
    feature.add_geometry(protozero::encode_zigzag32(-10));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(-10,0)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(-10));
    // Close
    feature.add_geometry(15);
    // LineTo(0,0)
    feature.add_geometry((1 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(0));

    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);

    SECTION("VT Spec v1")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POLYGON, 1, 0.0, 0.0, 1.0, 1.0));
    }

    SECTION("VT Spec v2")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POLYGON, 2, 0.0, 0.0, 1.0, 1.0));
    }
}

TEST_CASE("decode polygon -- lineto is last command")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POLYGON);
    // MoveTo(0,0)
    feature.add_geometry((1 << 3u) | 1u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(0,10)
    feature.add_geometry((3 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(10));
    // LineTo(-10,10)
    feature.add_geometry(protozero::encode_zigzag32(-10));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(-10,0)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(-10));
    // Close
    feature.add_geometry(15);
    // LineTo(1,1)
    feature.add_geometry((1 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));

    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);

    SECTION("VT Spec v1")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POLYGON, 1, 0.0, 0.0, 1.0, 1.0));
    }

    SECTION("VT Spec v2")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POLYGON, 2, 0.0, 0.0, 1.0, 1.0));
    }
}

TEST_CASE("decode polygon -- has invalid command first")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POLYGON);
    feature.add_geometry((1 << 3u) | 5u); // Invalid command
    // MoveTo(0,0)
    feature.add_geometry((1 << 3u) | 1u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(0,10)
    feature.add_geometry((3 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(10));
    // LineTo(-10,10)
    feature.add_geometry(protozero::encode_zigzag32(-10));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(-10,0)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(-10));
    // Close
    feature.add_geometry(15);

    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF geoms = feature_to_pbf_geometry(feature_string);

    SECTION("VT Spec v1")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POLYGON, 1, 0.0, 0.0, 1.0, 1.0));
    }

    SECTION("VT Spec v2")
    {
        CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POLYGON, 2, 0.0, 0.0, 1.0, 1.0));
    }
}
