#include "catch.hpp"

#include "encoding_util.hpp"
#include "decoding_util.hpp"
#include <mapnik/geometry_is_valid.hpp>
#include <mapnik/geometry_is_simple.hpp>
#include <mapnik/geometry_correct.hpp>
#include <mapnik/geometry_envelope.hpp>
#include <mapnik/util/geometry_to_wkt.hpp>

//
// Polygon Type Tests
//
TEST_CASE( "round trip polygon with valid exterior ring but one degenerate interior ring of two" )
{
    mapnik::geometry::polygon<std::int64_t> p0;
    p0.exterior_ring.add_coord(0,0);
    p0.exterior_ring.add_coord(0,10);
    p0.exterior_ring.add_coord(-10,10);
    p0.exterior_ring.add_coord(-10,0);
    p0.exterior_ring.add_coord(0,0);
    // invalid interior ring
    {
        mapnik::geometry::linear_ring<std::int64_t> hole;
        hole.add_coord(-7,7);
        hole.add_coord(-3,7);
        p0.add_hole(std::move(hole));
    }
    // valid interior ring
    {
        mapnik::geometry::linear_ring<std::int64_t> hole_in_hole;
        hole_in_hole.add_coord(-6,4);
        hole_in_hole.add_coord(-6,6);
        hole_in_hole.add_coord(-4,6);
        hole_in_hole.add_coord(-4,4);
        hole_in_hole.add_coord(-6,4);
        p0.add_hole(std::move(hole_in_hole));
    }

    // since first interior ring is degenerate it should have been culled when encoding occurs
    vector_tile::Tile_Feature feature = geometry_to_feature(p0);
    mapnik::vector_tile_impl::Geometry<double> geoms(feature,0.0,0.0,1.0,1.0);
    
    SECTION("VT Spec v1")
    {
        auto p1 = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 1);
        CHECK( p1.is<mapnik::geometry::polygon<double> >() );
        auto const& poly = mapnik::util::get<mapnik::geometry::polygon<double> >(p1);
        auto const& holes = poly.interior_rings;
        CHECK( holes.size() == 1 );
    }
    
    SECTION("VT Spec v2")
    {
        auto p1 = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 2);
        CHECK( p1.is<mapnik::geometry::polygon<double> >() );
        auto const& poly = mapnik::util::get<mapnik::geometry::polygon<double> >(p1);
        auto const& holes = poly.interior_rings;
        CHECK( holes.size() == 1 );
    }
}

TEST_CASE( "round trip polygon with degenerate exterior ring, full of repeated points" )
{
    // create invalid (exterior) polygon
    mapnik::geometry::polygon<std::int64_t> p0;
    p0.exterior_ring.add_coord(10,10);
    p0.exterior_ring.add_coord(10,10);
    p0.exterior_ring.add_coord(10,10);
    p0.exterior_ring.add_coord(10,10);
    mapnik::geometry::linear_ring<std::int64_t> hole;
    hole.add_coord(-7,7);
    hole.add_coord(-3,7);
    hole.add_coord(-3,3);
    hole.add_coord(-7,3);
    hole.add_coord(-7,7);
    p0.add_hole(std::move(hole));

    // The encoder should remove repeated points, so the entire polygon should be
    // thrown out
    vector_tile::Tile_Feature feature = geometry_to_feature(p0);
    mapnik::vector_tile_impl::Geometry<double> geoms(feature,0.0,0.0,1.0,1.0);

    SECTION("VT Spec v1")
    {
        auto p1 = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 1);
        CHECK( p1.is<mapnik::geometry::geometry_empty>() );
    }
    
    SECTION("VT Spec v2")
    {
        auto p1 = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 2);
        CHECK( p1.is<mapnik::geometry::geometry_empty>() );
    }
}


TEST_CASE( "round trip multipolygon with hole" )
{
    // NOTE: this polygon should have correct winding order:
    // CCW for exterior, CW for interior
    mapnik::geometry::polygon<std::int64_t> p0;
    p0.exterior_ring.add_coord(0,0);
    p0.exterior_ring.add_coord(0,10);
    p0.exterior_ring.add_coord(-10,10);
    p0.exterior_ring.add_coord(-10,0);
    p0.exterior_ring.add_coord(0,0);
    mapnik::geometry::linear_ring<std::int64_t> hole;
    hole.add_coord(-7,7);
    hole.add_coord(-3,7);
    hole.add_coord(-3,3);
    hole.add_coord(-7,3);
    hole.add_coord(-7,7);
    p0.add_hole(std::move(hole));

    mapnik::box2d<double> extent = mapnik::geometry::envelope(p0);
    std::string expected_wkt0("POLYGON((0 0,0 10,-10 10,-10 0,0 0),(-7 7,-3 7,-3 3,-7 3,-7 7))");
    std::string wkt0;

    vector_tile::Tile_Feature feature = geometry_to_feature(p0);
    mapnik::vector_tile_impl::Geometry<double> geoms(feature,0.0,0.0,1.0,1.0);
    
    SECTION("VT Spec v1")
    {
        auto p1 = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 1);
        CHECK( p1.is<mapnik::geometry::polygon<double> >() );
        CHECK( extent == mapnik::geometry::envelope(p1) );
        CHECK( mapnik::util::to_wkt(wkt0,p1) );
        CHECK( wkt0 == expected_wkt0);
    }

    SECTION("VT Spec v2")
    {
        auto p1 = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(), 2);
        CHECK( p1.is<mapnik::geometry::polygon<double> >() );
        CHECK( extent == mapnik::geometry::envelope(p1) );
        CHECK( mapnik::util::to_wkt(wkt0,p1) );
        CHECK( wkt0 == expected_wkt0);
    }
}
