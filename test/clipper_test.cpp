#include <limits>
#include <iostream>
#include <mapnik/projection.hpp>
#include <mapnik/geometry_transform.hpp>
//#include <mapnik/util/geometry_to_geojson.hpp>

#include "vector_tile_strategy.hpp"
#include "vector_tile_projection.hpp"

#include "test_utils.hpp"
#include "catch.hpp"
#include "clipper.hpp"

TEST_CASE( "vector_tile_strategy", "should not overflow" ) {
    mapnik::projection merc("+init=epsg:3857",true);
    mapnik::proj_transform prj_trans(merc,merc); // no-op
    unsigned tile_size = 256;
    mapnik::vector_tile_impl::spherical_mercator merc_tiler(tile_size);
    double minx,miny,maxx,maxy;
    merc_tiler.xyz(9664,20435,15,minx,miny,maxx,maxy);
    mapnik::box2d<double> z15_extent(minx,miny,maxx,maxy);
    mapnik::view_transform tr(tile_size,tile_size,z15_extent,0,0);
    {
        mapnik::vector_tile_impl::vector_tile_strategy_proj vs(prj_trans, tr, 16);
        // even an invalid point is not expected to result in values beyond hirange
        mapnik::geometry::point<double> g(-20037508.342789*2.0,-20037508.342789*2.0);
        mapnik::geometry::geometry<std::int64_t> new_geom = mapnik::geometry::transform<std::int64_t>(g, vs);
        REQUIRE( new_geom.is<mapnik::geometry::point<std::int64_t>>() );
        auto const& pt = mapnik::util::get<mapnik::geometry::point<std::int64_t>>(new_geom);
        REQUIRE( (pt.x < ClipperLib::hiRange) );
        REQUIRE( (pt.y < ClipperLib::hiRange) );
        REQUIRE( (-pt.x < ClipperLib::hiRange) );
        REQUIRE( (-pt.y < ClipperLib::hiRange) );
    }
    merc_tiler.xyz(0,0,0,minx,miny,maxx,maxy);
    mapnik::geometry::polygon<double> g;
    g.exterior_ring.add_coord(minx,miny);
    g.exterior_ring.add_coord(maxx,miny);
    g.exterior_ring.add_coord(maxx,maxy);
    g.exterior_ring.add_coord(minx,maxy);
    g.exterior_ring.add_coord(minx,miny);
    {
        // absurdly large but still should not result in values beyond hirange
        double path_multiplier = 100000000000.0;
        mapnik::vector_tile_impl::vector_tile_strategy_proj vs(prj_trans, tr, path_multiplier);
        mapnik::geometry::geometry<std::int64_t> new_geom = mapnik::geometry::transform<std::int64_t>(g, vs);
        REQUIRE( new_geom.is<mapnik::geometry::polygon<std::int64_t>>() );
        auto const& poly = mapnik::util::get<mapnik::geometry::polygon<std::int64_t>>(new_geom);
        for (auto const& pt : poly.exterior_ring)
        {
            INFO( pt.x )
            INFO( ClipperLib::hiRange )
            REQUIRE( (pt.x < ClipperLib::hiRange) );
            REQUIRE( (pt.y < ClipperLib::hiRange) );
            REQUIRE( (-pt.x < ClipperLib::hiRange) );
            REQUIRE( (-pt.y < ClipperLib::hiRange) );
        }
    }
    {
        // expected to trigger values above hirange
        double path_multiplier = 1000000000000.0;
        mapnik::vector_tile_impl::vector_tile_strategy_proj vs(prj_trans, tr, path_multiplier);
        CHECK_THROWS( mapnik::geometry::transform<std::int64_t>(g, vs) );
        mapnik::box2d<double> clip_extent(std::numeric_limits<double>::min(),
                                       std::numeric_limits<double>::min(),
                                       std::numeric_limits<double>::max(),
                                       std::numeric_limits<double>::max());
        mapnik::vector_tile_impl::transform_visitor<mapnik::vector_tile_impl::vector_tile_strategy_proj> skipping_transformer(vs, clip_extent);
        mapnik::geometry::geometry<std::int64_t> new_geom = skipping_transformer(g);
        REQUIRE( new_geom.is<mapnik::geometry::polygon<std::int64_t>>() );
        auto const& poly = mapnik::util::get<mapnik::geometry::polygon<std::int64_t>>(new_geom);
        for (auto const& pt : poly.exterior_ring)
        {
            INFO( pt.x )
            INFO( ClipperLib::hiRange )
            REQUIRE( (pt.x < ClipperLib::hiRange) );
            REQUIRE( (pt.y < ClipperLib::hiRange) );
            REQUIRE( (-pt.x < ClipperLib::hiRange) );
            REQUIRE( (-pt.y < ClipperLib::hiRange) );
        }
    }
}

TEST_CASE( "vector_tile_strategy2", "invalid mercator coord in interior ring" ) {
    mapnik::geometry::geometry<double> geom = testing::read_geojson("./test/data/invalid-interior-ring.json");
    mapnik::projection longlat("+init=epsg:4326",true);
    mapnik::proj_transform prj_trans(longlat,longlat); // no-op
    unsigned tile_size = 256;
    mapnik::vector_tile_impl::spherical_mercator merc_tiler(tile_size);
    double minx,miny,maxx,maxy;
    merc_tiler.xyz(9664,20435,15,minx,miny,maxx,maxy);
    mapnik::box2d<double> z15_extent(minx,miny,maxx,maxy);
    mapnik::view_transform tr(tile_size,tile_size,z15_extent,0,0);
    mapnik::vector_tile_impl::vector_tile_strategy_proj vs(prj_trans, tr, 16);
    CHECK_THROWS( mapnik::geometry::transform<std::int64_t>(geom, vs) );
    mapnik::box2d<double> clip_extent(std::numeric_limits<double>::min(),
                                   std::numeric_limits<double>::min(),
                                   std::numeric_limits<double>::max(),
                                   std::numeric_limits<double>::max());
    mapnik::vector_tile_impl::transform_visitor<mapnik::vector_tile_impl::vector_tile_strategy_proj> skipping_transformer(vs, clip_extent);
    mapnik::geometry::geometry<std::int64_t> new_geom = mapnik::util::apply_visitor(skipping_transformer,geom);
    REQUIRE( new_geom.is<mapnik::geometry::polygon<std::int64_t>>() );
    auto const& poly = mapnik::util::get<mapnik::geometry::polygon<std::int64_t>>(new_geom);
    for (auto const& pt : poly.exterior_ring)
    {
        INFO( pt.x )
        INFO( ClipperLib::hiRange )
        REQUIRE( (pt.x < ClipperLib::hiRange) );
        REQUIRE( (pt.y < ClipperLib::hiRange) );
        REQUIRE( (-pt.x < ClipperLib::hiRange) );
        REQUIRE( (-pt.y < ClipperLib::hiRange) );
    }
    for (auto const& ring : poly.interior_rings)
    {
        for (auto const& pt : ring)
        {
            INFO( pt.x )
            INFO( ClipperLib::hiRange )
            REQUIRE( (pt.x < ClipperLib::hiRange) );
            REQUIRE( (pt.y < ClipperLib::hiRange) );
            REQUIRE( (-pt.x < ClipperLib::hiRange) );
            REQUIRE( (-pt.y < ClipperLib::hiRange) );
        }
    }
}

TEST_CASE( "clipper IntPoint", "should accept 64bit values" ) {
    std::int64_t x = 4611686018427387903;
    std::int64_t y = 4611686018427387903;
    auto x0 = std::numeric_limits<std::int64_t>::max();
    auto y0 = std::numeric_limits<std::int64_t>::max();
    REQUIRE( x == ClipperLib::hiRange );
    REQUIRE( y == ClipperLib::hiRange );
    REQUIRE( (x0/2) == ClipperLib::hiRange );
    REQUIRE( (y0/2) == ClipperLib::hiRange );
    auto pt = ClipperLib::IntPoint(x,y);
    CHECK( pt.x == x );
    CHECK( pt.y == y );
    auto pt2 = ClipperLib::IntPoint(x,y);
    CHECK( (pt == pt2) );
    CHECK( !(pt != pt2) );
    // this is invalid when passed to RangeTest but should
    // still be able to be created
    auto pt3 = ClipperLib::IntPoint(x0,y0);
    REQUIRE( pt3.x == x0 );
    REQUIRE( pt3.y == y0 );
    REQUIRE( (pt3.x/2) == ClipperLib::hiRange );
    REQUIRE( (pt3.y/2) == ClipperLib::hiRange );
    CHECK( (pt != pt3) );
}

TEST_CASE( "clipper AddPath 1", "should not throw within range coords" ) {
    ClipperLib::Clipper clipper;
    ClipperLib::Path clip_box; // actually mapnik::geometry::line_string<std::int64_t>
    // values that should just barely work since they are one below the
    // threshold
    auto x0 = (std::numeric_limits<std::int64_t>::min()/2)+1;
    auto y0 = (std::numeric_limits<std::int64_t>::min()/2)+1;
    auto x1 = (std::numeric_limits<std::int64_t>::max()/2);
    auto y1 = (std::numeric_limits<std::int64_t>::max()/2);
    clip_box.emplace_back(x0,y0);
    clip_box.emplace_back(x1,y0);
    clip_box.emplace_back(x1,y1);
    clip_box.emplace_back(x0,y1);
    clip_box.emplace_back(x0,y0);
    CHECK( clipper.AddPath(clip_box,ClipperLib::ptClip,true) );
}

TEST_CASE( "clipper AddPath 2", "should throw on out of range coords" ) {
    ClipperLib::Clipper clipper;
    ClipperLib::Path clip_box; // actually mapnik::geometry::line_string<std::int64_t>
    auto x0 = std::numeric_limits<std::int64_t>::min()+1;
    auto y0 = std::numeric_limits<std::int64_t>::min()+1;
    auto x1 = std::numeric_limits<std::int64_t>::max()-1;
    auto y1 = std::numeric_limits<std::int64_t>::max()-1;
    clip_box.emplace_back(x0,y0);
    clip_box.emplace_back(x1,y0);
    clip_box.emplace_back(x1,y1);
    clip_box.emplace_back(x0,y1);
    clip_box.emplace_back(x0,y0);
    try
    {
      clipper.AddPath(clip_box,ClipperLib::ptClip,true);
      FAIL( "expected exception" );
    }
    catch(std::exception const& ex)
    {
      REQUIRE(std::string(ex.what()) == "Coordinate outside allowed range: -9223372036854775807 -9223372036854775807 9223372036854775807 9223372036854775807");
    }        
}

TEST_CASE( "clipper polytree error" ) {

    // http://sourceforge.net/p/polyclipping/bugs/132/
    ClipperLib::Clipper clipper;
    ClipperLib::Paths polygons;
    ClipperLib::Path shape1;
    ClipperLib::Path shape2;
    ClipperLib::Path shape3;
    ClipperLib::Path shape4;
    // shape 1
    shape1.emplace_back(280000, 240000),
    shape1.emplace_back(180000, 240000),
    shape1.emplace_back(180000, 200000),
    shape1.emplace_back(240000, 200000),
    shape1.emplace_back(240000, 90000),
    shape1.emplace_back(180000, 90000),
    shape1.emplace_back(180000, 50000),
    shape1.emplace_back(280000, 50000),
    // shape 2
    shape2.emplace_back(230000, 120000),
    shape2.emplace_back(230000, 160000),
    shape2.emplace_back(180000, 160000),
    shape2.emplace_back(180000, 120000),
    // shape 3
    shape3.emplace_back(180000, 50000),
    shape3.emplace_back(180000, 90000),
    shape3.emplace_back(100000, 90001), // Problematic point
    //shape3.emplace_back(100000, 90000), // This point is okay
    shape3.emplace_back(100000, 200000),
    shape3.emplace_back(180000, 200000),
    shape3.emplace_back(180000, 240000),
    shape3.emplace_back(60000, 240000),
    shape3.emplace_back(60000, 50000),
    // shape 4
    shape4.emplace_back(180000, 120000),
    shape4.emplace_back(180000, 160000),
    shape4.emplace_back(120000, 160000),
    shape4.emplace_back(120000, 120000),
    polygons.emplace_back(shape1);
    polygons.emplace_back(shape2);
    polygons.emplace_back(shape3);
    polygons.emplace_back(shape4);

    clipper.AddPaths(polygons, ClipperLib::ptSubject, true);
    ClipperLib::PolyTree solution;
    clipper.Execute(ClipperLib::ctUnion, solution, ClipperLib::pftNonZero, ClipperLib::pftNonZero);

    // Check that the polytree is correct
    REQUIRE(solution.Childs.size() == 1);
    REQUIRE(solution.Childs[0]->Childs.size() == 1);
    REQUIRE(solution.Childs[0]->Childs[0]->Childs.size() == 1);
    REQUIRE(solution.Childs[0]->Childs[0]->Childs[0]->Childs.size() == 0);

}

