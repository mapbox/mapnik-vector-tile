#include "catch.hpp"

#include "encoding_util.hpp"
#include <mapnik/geometry_is_valid.hpp>
#include <mapnik/geometry_is_simple.hpp>
#include <mapnik/geometry_correct.hpp>
#include <mapnik/geometry_envelope.hpp>
#include <mapnik/util/geometry_to_wkt.hpp>

//#include <mapnik/geometry_unique.hpp>

/*

low level encoding and decoding that skips clipping

*/

TEST_CASE( "point", "should round trip without changes" ) {
    mapnik::geometry::point<std::int64_t> g(0,0);
    std::string expected(
    "move_to(0,0)\n"
    );
    CHECK(compare<std::int64_t>(g) == expected);
}

TEST_CASE( "multi_point", "should round trip without changes" ) {
    mapnik::geometry::multi_point<std::int64_t> g;
    g.add_coord(0,0);
    g.add_coord(1,1);
    g.add_coord(2,2);
    std::string expected(
    "move_to(0,0)\n"
    "move_to(1,1)\n"
    "move_to(2,2)\n"
    );
    CHECK(compare<std::int64_t>(g) == expected);
}

TEST_CASE( "line_string", "should round trip without changes" ) {
    mapnik::geometry::line_string<std::int64_t> g;
    g.add_coord(0,0);
    g.add_coord(1,1);
    g.add_coord(100,100);
    std::string expected(
    "move_to(0,0)\n"
    "line_to(1,1)\n"
    "line_to(100,100)\n"
    );
    CHECK(compare<std::int64_t>(g) == expected);
}

TEST_CASE( "multi_line_string", "should round trip without changes" ) {
    mapnik::geometry::multi_line_string<std::int64_t> g;
    {
        mapnik::geometry::line_string<std::int64_t> line;
        line.add_coord(0,0);
        line.add_coord(1,1);
        line.add_coord(100,100);
        g.emplace_back(std::move(line));
    }
    {
        mapnik::geometry::line_string<std::int64_t> line;
        line.add_coord(-10,-10);
        line.add_coord(-20,-20);
        line.add_coord(-100,-100);
        g.emplace_back(std::move(line));
    }
    std::string expected(
    "move_to(0,0)\n"
    "line_to(1,1)\n"
    "line_to(100,100)\n"
    "move_to(-10,-10)\n"
    "line_to(-20,-20)\n"
    "line_to(-100,-100)\n"
    );
    CHECK(compare<std::int64_t>(g) == expected);
}

/*TEST_CASE( "degenerate line_string", "should be culled" ) {
    mapnik::geometry::line_string<std::int64_t> line;
    line.add_coord(10,10);

    std::string wkt0;
    CHECK( mapnik::util::to_wkt(wkt0,line) );
    // wkt writer copes with busted line_string
    std::string expected_wkt0("LINESTRING(10 10)");
    CHECK( wkt0 == expected_wkt0);

    vector_tile::Tile_Feature feature = geometry_to_feature<std::int64_t>(line);
    CHECK( feature.geometry_size() == 0 );
    auto geom = mapnik::vector_tile_impl::decode_geometry(feature,0.0,0.0,1.0,1.0);
    CHECK( geom.is<mapnik::geometry::geometry_empty>() );
}*/

/*
TEST_CASE( "multi_line_string with degenerate first part", "should be culled" ) {
    mapnik::geometry::multi_line_string<std::int64_t> g;
    mapnik::geometry::line_string<std::int64_t> l1;
    l1.add_coord(0,0);
    g.push_back(std::move(l1));
    mapnik::geometry::line_string<std::int64_t> l2;
    l2.add_coord(2,2);
    l2.add_coord(3,3);
    g.push_back(std::move(l2));

    std::string wkt0;
    CHECK( mapnik::util::to_wkt(wkt0,g) );
    // wkt writer copes with busted line_string
    std::string expected_wkt0("MULTILINESTRING((0 0),(2 2,3 3))");
    CHECK( wkt0 == expected_wkt0);

    vector_tile::Tile_Feature feature = geometry_to_feature<std::int64_t>(g);
    CHECK( feature.geometry_size() == 6 );
    auto geom = mapnik::vector_tile_impl::decode_geometry(feature,0.0,0.0,1.0,1.0);

    wkt0.clear();
    CHECK( mapnik::util::to_wkt(wkt0,geom) );
    CHECK( wkt0 == "LINESTRING(2 2,3 3)");
    CHECK( geom.is<mapnik::geometry::line_string<std::int64_t> >() );
}*/

/*TEST_CASE( "multi_line_string with degenerate second part", "should be culled" ) {
    mapnik::geometry::multi_line_string<std::int64_t> g;
    {
        mapnik::geometry::line_string<std::int64_t> line;
        line.add_coord(0,0);
        line.add_coord(1,1);
        line.add_coord(100,100);
        g.emplace_back(std::move(line));
    }
    {
        mapnik::geometry::line_string<std::int64_t> line;
        line.add_coord(-10,-10);
        g.emplace_back(std::move(line));
    }

    std::string wkt0;
    CHECK( mapnik::util::to_wkt(wkt0,g) );
    // wkt writer copes with busted line_string
    std::string expected_wkt0("MULTILINESTRING((0 0,1 1,100 100),(-10 -10))");
    CHECK( wkt0 == expected_wkt0);

    vector_tile::Tile_Feature feature = geometry_to_feature<std::int64_t>(g);
    CHECK( feature.type() == vector_tile::Tile_GeomType_LINESTRING );
    CHECK( feature.geometry_size() == 8 );
    auto geom = mapnik::vector_tile_impl::decode_geometry(feature,0.0,0.0,1.0,1.0);

    wkt0.clear();
    CHECK( mapnik::util::to_wkt(wkt0,geom) );
    CHECK( wkt0 == "LINESTRING(0 0,1 1,100 100)");
    CHECK( geom.is<mapnik::geometry::line_string<std::int64_t> >() );
}
*/
TEST_CASE( "polygon", "should round trip without changes" ) {
    mapnik::geometry::polygon<std::int64_t> g;
    g.exterior_ring.add_coord(0,0);
    g.exterior_ring.add_coord(1,1);
    g.exterior_ring.add_coord(100,100);
    g.exterior_ring.add_coord(0,0);
    std::string expected(
    "move_to(0,0)\n"
    "line_to(1,1)\n"
    "line_to(100,100)\n"
    "close_path(0,0)\n"
    );
    CHECK(compare<std::int64_t>(g) == expected);
}

TEST_CASE( "polygon with degenerate exterior ring ", "should be culled" ) {
    mapnik::geometry::polygon<std::int64_t> p0;
    // invalid exterior ring
    p0.exterior_ring.add_coord(0,0);
    p0.exterior_ring.add_coord(0,10);

    std::string wkt0;
    CHECK( mapnik::util::to_wkt(wkt0,mapnik::geometry::geometry<std::int64_t>(p0)) );
    // wkt writer copes with busted polygon
    std::string expected_wkt0("POLYGON((0 0,0 10))");
    CHECK( wkt0 == expected_wkt0);

    vector_tile::Tile_Feature feature = geometry_to_feature<std::int64_t>(p0);
    // since first ring is degenerate the whole polygon should be culled
    mapnik::vector_tile_impl::Geometry geoms(feature,0.0,0.0,1.0,1.0);
    auto p1 = mapnik::vector_tile_impl::decode_geometry(geoms, feature.type());
    CHECK( p1.is<mapnik::geometry::geometry_empty>() );
}

/*TEST_CASE( "polygon with degenerate exterior ring will drop valid interior ring", "should be culled" ) {
    mapnik::geometry::polygon<std::int64_t> p0;
    // invalid exterior ring
    p0.exterior_ring.add_coord(0,0);
    p0.exterior_ring.add_coord(0,10);
    // valid interior ring
    mapnik::geometry::linear_ring<std::int64_t> hole;
    hole.add_coord(-7,7);
    hole.add_coord(-3,7);
    hole.add_coord(-3,3);
    hole.add_coord(-7,3);
    hole.add_coord(-7,7);
    p0.add_hole(std::move(hole));

    std::string wkt0;
    CHECK( mapnik::util::to_wkt(wkt0,p0) );
    // wkt writer copes with busted polygon
    std::string expected_wkt0("POLYGON((0 0,0 10),(-7 7,-3 7,-3 3,-7 3,-7 7))");
    CHECK( wkt0 == expected_wkt0);

    vector_tile::Tile_Feature feature = geometry_to_feature<std::int64_t>(p0);
    // since first ring is degenerate the whole polygon should be culled
    mapnik::vector_tile_impl::Geometry geoms(feature,0.0,0.0,1.0,1.0);
    auto p1 = mapnik::vector_tile_impl::decode_geometry(geoms, feature.type());
    CHECK( p1.is<mapnik::geometry::geometry_empty>() );
}*/

TEST_CASE( "polygon with valid exterior ring but degenerate interior ring", "should be culled" ) {
    mapnik::geometry::polygon<std::int64_t> p0;
    p0.exterior_ring.add_coord(0,0);
    p0.exterior_ring.add_coord(0,10);
    p0.exterior_ring.add_coord(-10,10);
    p0.exterior_ring.add_coord(-10,0);
    p0.exterior_ring.add_coord(0,0);
    // invalid interior ring
    mapnik::geometry::linear_ring<std::int64_t> hole;
    hole.add_coord(-7,7);
    hole.add_coord(-3,7);
    p0.add_hole(std::move(hole));

    std::string wkt0;
    CHECK( mapnik::util::to_wkt(wkt0,mapnik::geometry::geometry<std::int64_t>(p0)) );
    // wkt writer copes with busted polygon
    std::string expected_wkt0("POLYGON((0 0,0 10,-10 10,-10 0,0 0),(-7 7,-3 7))");
    CHECK( wkt0 == expected_wkt0);

    vector_tile::Tile_Feature feature = geometry_to_feature<std::int64_t>(p0);
    mapnik::vector_tile_impl::Geometry geoms(feature,0.0,0.0,1.0,1.0);
    auto p1 = mapnik::vector_tile_impl::decode_geometry(geoms, feature.type());
    CHECK( p1.is<mapnik::geometry::polygon<double> >() );
    auto const& poly = mapnik::util::get<mapnik::geometry::polygon<double> >(p1);
    // since interior ring is degenerate it should have been culled when decoded
    auto const& holes = poly.interior_rings;
    CHECK( holes.empty() == true );
}

TEST_CASE( "polygon with valid exterior ring but one degenerate interior ring of two", "should be culled" ) {
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

    std::string wkt0;
    CHECK( mapnik::util::to_wkt(wkt0,mapnik::geometry::geometry<std::int64_t>(p0)) );
    // wkt writer copes with busted polygon
    std::string expected_wkt0("POLYGON((0 0,0 10,-10 10,-10 0,0 0),(-7 7,-3 7),(-6 4,-6 6,-4 6,-4 4,-6 4))");
    CHECK( wkt0 == expected_wkt0);

    vector_tile::Tile_Feature feature = geometry_to_feature<std::int64_t>(p0);
    mapnik::vector_tile_impl::Geometry geoms(feature,0.0,0.0,1.0,1.0);
    auto p1 = mapnik::vector_tile_impl::decode_geometry(geoms, feature.type());
    CHECK( p1.is<mapnik::geometry::polygon<double> >() );
    auto const& poly = mapnik::util::get<mapnik::geometry::polygon<double> >(p1);
    // since first interior ring is degenerate it should have been culled when decoded
    auto const& holes = poly.interior_rings;
    // the second one is kept: somewhat dubious since it is actually a hole in a hole
    // but this is probably the best we can do
    CHECK( holes.size() == 1 );
}

TEST_CASE( "(multi)polygon with hole", "should round trip without changes" ) {
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

    std::string wkt0;
    CHECK( mapnik::util::to_wkt(wkt0,mapnik::geometry::geometry<std::int64_t>(p0) ) );
    std::string expected_wkt0("POLYGON((0 0,0 10,-10 10,-10 0,0 0),(-7 7,-3 7,-3 3,-7 3,-7 7))");
    CHECK( wkt0 == expected_wkt0);
    // ensure correcting geometry has no effect
    // as a way of confirming the original was correct
    mapnik::geometry::correct(p0);
    wkt0.clear();
    CHECK( mapnik::util::to_wkt(wkt0,mapnik::geometry::geometry<std::int64_t>(p0)) );
    CHECK( wkt0 == expected_wkt0);

    vector_tile::Tile_Feature feature = geometry_to_feature<std::int64_t>(p0);
    mapnik::vector_tile_impl::Geometry geoms(feature,0.0,0.0,1.0,1.0);
    auto p1 = mapnik::vector_tile_impl::decode_geometry(geoms, feature.type());
    CHECK( p1.is<mapnik::geometry::polygon<double> >() );
    CHECK( extent == mapnik::geometry::envelope(p1) );

    wkt0.clear();
    CHECK( mapnik::util::to_wkt(wkt0,p1) );
    CHECK( wkt0 == expected_wkt0);

    // now test back compatibility mode where we decode all rings into exterior rings
    // for polygons rings that were encoded correctly in vtiles (CCW exterior, CW interior)
    // then this should be unneeded, but for rings with incorrect order then this style of
    // decoding should allow them still to be queried correctly using the current mapnik hit_test algos
    // Note, we need a new Geometry here, the old object can't be rewound.
    mapnik::vector_tile_impl::Geometry geoms2(feature,0.0,0.0,1.0,1.0);
    auto _p1 = mapnik::vector_tile_impl::decode_geometry(geoms2, feature.type(), true);
    wkt0.clear();
    CHECK( mapnik::util::to_wkt(wkt0,_p1) );
    CHECK( _p1.is<mapnik::geometry::multi_polygon<double> >() );
    std::string expected_wkt2("MULTIPOLYGON(((0 0,0 10,-10 10,-10 0,0 0)),((-7 7,-7 3,-3 3,-3 7,-7 7)))");
    CHECK( wkt0 ==  expected_wkt2 );
    mapnik::geometry::correct(_p1);
    wkt0.clear();
    CHECK( mapnik::util::to_wkt(wkt0,_p1) );
    CHECK( wkt0 ==  expected_wkt2 );

    std::string expected_p0(
    "move_to(0,0)\n"
    "line_to(0,10)\n"
    "line_to(-10,10)\n"
    "line_to(-10,0)\n"
    "close_path(0,0)\n"
    "move_to(-7,7)\n"
    "line_to(-3,7)\n"
    "line_to(-3,3)\n"
    "line_to(-7,3)\n"
    "close_path(0,0)\n"
    );

    CHECK(decode_to_path_string(p1) == expected_p0);

    std::string expected_p1(
    "move_to(0,0)\n"
    "line_to(0,10)\n"
    "line_to(-10,10)\n"
    "line_to(-10,0)\n"
    "close_path(0,0)\n"
    "move_to(-7,7)\n"
    "line_to(-7,3)\n"
    "line_to(-3,3)\n"
    "line_to(-3,7)\n"
    "close_path(0,0)\n"
    );

    CHECK(decode_to_path_string(_p1) == expected_p1);

    // make into multi_polygon
    mapnik::geometry::multi_polygon<std::int64_t> multi_poly;
    multi_poly.push_back(std::move(p0));
    mapnik::geometry::polygon<std::int64_t> p2;
    p2.exterior_ring.add_coord(-6,4);
    p2.exterior_ring.add_coord(-4,4);
    p2.exterior_ring.add_coord(-4,6);
    p2.exterior_ring.add_coord(-6,6);
    p2.exterior_ring.add_coord(-6,4);
    multi_poly.push_back(std::move(p2));

    mapnik::box2d<double> multi_extent = mapnik::geometry::envelope(multi_poly);

    vector_tile::Tile_Feature feature1 = geometry_to_feature<std::int64_t>(multi_poly);
    mapnik::vector_tile_impl::Geometry geoms1(feature1,0.0,0.0,1.0,1.0);
    auto mp = mapnik::vector_tile_impl::decode_geometry(geoms1, feature1.type());
    CHECK( mp.is<mapnik::geometry::multi_polygon<double> >() );

    CHECK( multi_extent == mapnik::geometry::envelope(mp) );

    wkt0.clear();
    CHECK( mapnik::util::to_wkt(wkt0,mp) );
    std::string expected_wkt3("MULTIPOLYGON(((0 0,0 10,-10 10,-10 0,0 0),(-7 7,-3 7,-3 3,-7 3,-7 7)),((-6 4,-4 4,-4 6,-6 6,-6 4)))");
    CHECK( wkt0 == expected_wkt3);
    // ensure correcting geometry has no effect
    // as a way of confirming the original was correct
    mapnik::geometry::correct(mp);
    wkt0.clear();
    CHECK( mapnik::util::to_wkt(wkt0,mp) );
    CHECK( wkt0 == expected_wkt3);

    std::string expected_multi = expected_p0 += std::string(
    "move_to(-6,4)\n"
    "line_to(-4,4)\n"
    "line_to(-4,6)\n"
    "line_to(-6,6)\n"
    "close_path(0,0)\n"
    );

    CHECK(decode_to_path_string(mp) == expected_multi);
    CHECK(mapnik::geometry::is_valid(mp));
    CHECK(mapnik::geometry::is_simple(mp));

}

// We no longer drop coincidental points in the encoder it should be
// done prior to reaching encoder.
/*TEST_CASE( "test 2", "should drop coincident line_to commands" ) {
    mapnik::geometry::line_string<std::int64_t> g;
    g.add_coord(0,0);
    g.add_coord(3,3);
    g.add_coord(3,3);
    g.add_coord(3,3);
    g.add_coord(3,3);
    g.add_coord(4,4);
    std::string expected(
    "move_to(0,0)\n"
    "line_to(3,3)\n"
    "line_to(4,4)\n"
    );
    CHECK( compare<std::int64_t>(g) == expected);
}*/

/*
TEST_CASE( "test 2b", "should drop vertices" ) {
    mapnik::geometry::line_string<std::int64_t> g;
    g.add_coord(0,0);
    g.add_coord(0,0);
    g.add_coord(1,1);
    std::string expected(
    "move_to(0,0)\n"
    "line_to(0,0)\n" // TODO - should we try to drop this?
    "line_to(1,1)\n"
    );
    CHECK(compare<std::int64_t>(g) == expected);
}*/

TEST_CASE( "test 3", "should not drop first move_to or last vertex in line" ) {
    mapnik::geometry::multi_line_string<std::int64_t> g;
    mapnik::geometry::line_string<std::int64_t> l1;
    l1.add_coord(0,0);
    l1.add_coord(1,1);
    g.push_back(std::move(l1));
    mapnik::geometry::line_string<std::int64_t> l2;
    l2.add_coord(2,2);
    l2.add_coord(3,3);
    g.push_back(std::move(l2));

    std::string expected(
    "move_to(0,0)\n"
    "line_to(1,1)\n"
    "move_to(2,2)\n"
    "line_to(3,3)\n"
    );
    CHECK(compare<std::int64_t>(g) == expected);
}

/*

TEST_CASE( "test 4", "should not drop first move_to or last vertex in polygon" ) {
    mapnik::geometry::multi_polygon g;
    mapnik::geometry::polygon p0;
    p0.exterior_ring.add_coord(0,0);
    p0.exterior_ring.add_coord(1,0);
    g.push_back(std::move(p0));

    mapnik::geometry::polygon p1;
    p1.exterior_ring.add_coord(1,1);
    p1.exterior_ring.add_coord(0,1);
    p1.exterior_ring.add_coord(1,1);
    g.push_back(std::move(p1));

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

    mapnik::geometry::path p(mapnik::path::LineString);
    g.move_to(0,0);
    g.move_to(1,1); // skipped
    g.line_to(4,4); // skipped
    g.line_to(5,5);

    std::string expected(
    "move_to(0,0)\n" // TODO - should we keep move_to(1,1) instead?
    "line_to(5,5)\n"
    );
    CHECK(compare(g,2) == expected);
}


TEST_CASE( "test 5b", "can drop duplicate move_to" ) {
    mapnik::geometry::line_string g;
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
    mapnik::geometry::line_string g;
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

TEST_CASE( "test 6", "should not drop last line_to if repeated" ) {
    mapnik::geometry::line_string g;
    g.move_to(0,0);
    g.line_to(2,2);
    g.line_to(1000,1000); // skipped
    g.line_to(1001,1001); // skipped
    g.line_to(1001,1001);
    std::string expected(
    "move_to(0,0)\n"
    "line_to(2,2)\n"
    "line_to(1001,1001)\n"
    );
    CHECK(compare(g,2) == expected);
}

TEST_CASE( "test 7", "ensure proper handling of skipping + close commands" ) {
    mapnik::geometry_type g(mapnik::geometry_type::types::Polygon);
    g.move_to(0,0);
    g.line_to(2,2);
    g.close_path();
    g.move_to(5,5);
    g.line_to(10,10); // skipped
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
    mapnik::geometry_type g(mapnik::geometry_type::types::Polygon);
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
    mapnik::geometry::line_string g;
    g.move_to(0,0);
    g.line_to(9,0); // skipped
    g.line_to(0,10);
    std::string expected(
    "move_to(0,0)\n"
    "line_to(0,10)\n"
    );
    CHECK(compare(g,11) == expected);
}

TEST_CASE( "test 9b", "should not drop last vertex" ) {
    mapnik::geometry_type g(mapnik::geometry_type::types::Polygon);
    g.move_to(0,0);
    g.line_to(10,0); // skipped
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
    mapnik::geometry_type g(mapnik::geometry_type::types::Polygon);
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
    mapnik::geometry_type g(mapnik::geometry_type::types::Polygon);
    g.move_to(0,0);
    g.line_to(10,10);
    g.line_to(10,10); // skipped
    g.line_to(20,20);
    g.line_to(20,20); // skipped, but added back and replaces previous
    g.close_path();
    g.close_path(); // skipped
    g.close_path(); // skipped
    g.close_path(); // skipped
    g.move_to(0,0);
    g.line_to(10,10);
    g.line_to(20,20);
    g.close_path();
    g.close_path(); // skipped
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
    using namespace mapnik::vector_tile_impl;
    vector_tile::Tile_Feature feature0;
    int32_t x = 0;
    int32_t y = 0;
    unsigned path_multiplier = 1;
    unsigned tolerance = 10000;
    mapnik::geometry_type g0(mapnik::geometry_type::types::Polygon);
    g0.move_to(0,0);
    g0.line_to(-10,-10);
    g0.line_to(-20,-20);
    g0.close_path();
    mapnik::vertex_adapter va(g0);
    encode_geometry(va,(vector_tile::Tile_GeomType)g0.type(),feature0,x,y,tolerance,path_multiplier);
    CHECK(x == -20);
    CHECK(y == -20);
    mapnik::geometry_type g1(mapnik::geometry_type::types::Polygon);
    g1.move_to(1000,1000);
    g1.line_to(1010,1010);
    g1.line_to(1020,1020);
    g1.close_path();
    mapnik::vertex_adapter va1(g1);
    encode_geometry(va1,(vector_tile::Tile_GeomType)g1.type(),feature0,x,y,tolerance,path_multiplier);
    CHECK(x == 1020);
    CHECK(y == 1020);
    mapnik::geometry_type g2(mapnik::geometry_type::types::Polygon);
    double x0 = 0;
    double y0 = 0;
    decode_geometry(feature0,g2,x0,y0,path_multiplier);
    mapnik::vertex_adapter va2(g2);
    std::string actual = show_path(va2);
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

TEST_CASE( "test 12", "should correctly encode multiple paths" ) {
    using namespace mapnik::vector_tile_impl;
    vector_tile::Tile_Feature feature0;
    int32_t x = 0;
    int32_t y = 0;
    unsigned path_multiplier = 1;
    unsigned tolerance = 10;
    mapnik::geometry_type g(mapnik::geometry_type::types::Polygon);
    g.move_to(0,0);
    g.line_to(100,0);
    g.line_to(100,100);
    g.line_to(0,100);
    g.line_to(0,5);
    g.line_to(0,0);
    g.close_path();
    g.move_to(20,20);
    g.line_to(20,60);
    g.line_to(60,60);
    g.line_to(60,20);
    g.line_to(25,20);
    g.line_to(20,20);
    g.close_path();
    mapnik::vertex_adapter va(g);
    encode_geometry(va,(vector_tile::Tile_GeomType)g.type(),feature0,x,y,tolerance,path_multiplier);

    mapnik::geometry_type g2(mapnik::geometry_type::types::Polygon);
    double x0 = 0;
    double y0 = 0;
    decode_geometry(feature0,g2,x0,y0,path_multiplier);
    mapnik::vertex_adapter va2(g2);
    std::string actual = show_path(va2);
    std::string expected(
        "move_to(0,0)\n"
        "line_to(100,0)\n"
        "line_to(100,100)\n"
        "line_to(0,100)\n"
        "line_to(0,0)\n"
        "close_path(0,0)\n"
        "move_to(20,20)\n"
        "line_to(20,60)\n"
        "line_to(60,60)\n"
        "line_to(60,20)\n"
        "line_to(20,20)\n"
        "close_path(0,0)\n"
        );
    CHECK(actual == expected);
}
*/
