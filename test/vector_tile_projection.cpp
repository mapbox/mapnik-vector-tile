#include "catch.hpp"

#include <mapnik/projection.hpp>
#include <mapnik/box2d.hpp>
#include <mapnik/proj_transform.hpp>
#include "vector_tile_projection.hpp"

TEST_CASE( "vector tile projection 1", "should support z/x/y to bbox conversion at 0/0/0" ) {
    mapnik::vector_tile_impl::spherical_mercator merc(256);
    double minx,miny,maxx,maxy;
    merc.xyz(0,0,0,minx,miny,maxx,maxy);
    mapnik::box2d<double> map_extent(minx,miny,maxx,maxy);
    mapnik::box2d<double> e(-20037508.342789,-20037508.342789,20037508.342789,20037508.342789);
    double epsilon = 0.000001;
    CHECK(std::fabs(map_extent.minx() - e.minx()) < epsilon);
    CHECK(std::fabs(map_extent.miny() - e.miny()) < epsilon);
    CHECK(std::fabs(map_extent.maxx() - e.maxx()) < epsilon);
    CHECK(std::fabs(map_extent.maxy() - e.maxy()) < epsilon);
}

TEST_CASE( "vector tile projection 2", "should support z/x/y to bbox conversion up to z33" ) {
    mapnik::vector_tile_impl::spherical_mercator merc(256);
    int x = 2145960701;
    int y = 1428172928;
    int z = 32;
    double minx,miny,maxx,maxy;
    merc.xyz(x,y,z,minx,miny,maxx,maxy);
    mapnik::box2d<double> map_extent(minx,miny,maxx,maxy);
    mapnik::box2d<double> e(-14210.1492817168364127,6711666.7204630710184574,-14210.1399510249066225,6711666.7297937674447894);
    double epsilon = 0.00000001;
    CHECK(std::fabs(map_extent.minx() - e.minx()) < epsilon);
    CHECK(std::fabs(map_extent.miny() - e.miny()) < epsilon);
    CHECK(std::fabs(map_extent.maxx() - e.maxx()) < epsilon);
    CHECK(std::fabs(map_extent.maxy() - e.maxy()) < epsilon);
}
