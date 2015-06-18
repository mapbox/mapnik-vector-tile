#include <limits>
#include <iostream>
#include <mapnik/projection.hpp>
#include <mapnik/geometry_transform.hpp>
//#include <mapnik/util/geometry_to_geojson.hpp>

#include "vector_tile_strategy.hpp"
#include "vector_tile_projection.hpp"

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
        mapnik::vector_tile_impl::vector_tile_strategy vs(prj_trans, tr, 16);
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
        mapnik::vector_tile_impl::vector_tile_strategy vs(prj_trans, tr, path_multiplier);
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
        mapnik::vector_tile_impl::vector_tile_strategy vs(prj_trans, tr, path_multiplier);
        mapnik::geometry::geometry<std::int64_t> new_geom = mapnik::geometry::transform<std::int64_t>(g, vs);
        REQUIRE( new_geom.is<mapnik::geometry::polygon<std::int64_t>>() );
        auto const& poly = mapnik::util::get<mapnik::geometry::polygon<std::int64_t>>(new_geom);
        for (auto const& pt : poly.exterior_ring)
        {
            INFO( pt.x )
            INFO( ClipperLib::hiRange )
            REQUIRE(( (pt.x > ClipperLib::hiRange) ||
                       (pt.y > ClipperLib::hiRange) ||
                       (-pt.x < ClipperLib::hiRange) ||
                       (-pt.y < ClipperLib::hiRange)
                    ));
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

