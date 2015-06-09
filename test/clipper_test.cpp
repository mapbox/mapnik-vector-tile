#include "catch.hpp"
#include "clipper.hpp"
#include <limits>
#include <iostream>

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
    auto x0 = std::numeric_limits<std::int64_t>::min();
    auto y0 = std::numeric_limits<std::int64_t>::min();
    auto x1 = std::numeric_limits<std::int64_t>::max();
    auto y1 = std::numeric_limits<std::int64_t>::max();
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
      REQUIRE(std::string(ex.what()) == "Coordinate outside allowed range: -9223372036854775808 -9223372036854775808 -9223372036854775808 -9223372036854775808");
    }        
}

