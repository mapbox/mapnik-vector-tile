#include "catch.hpp"

#include <mapnik/projection.hpp>
#include <mapnik/geometry/box2d.hpp>
#include <mapnik/well_known_srs.hpp>
#include <mapnik/proj_transform.hpp>
#include "vector_tile_projection.hpp"

std::vector<double> pointToTile(double lon, double lat, unsigned z)
{
    double s = std::sin(lat * M_PI / 180.0);
    double z2 = std::pow(2.0,z);
    double x = z2 * (lon / 360.0 + 0.5);
    double y = z2 * (0.5 - 0.25 * std::log((1 + s) / (1 - s)) / M_PI) - 1;
    return { x, y };
}

int getBboxZoom(std::vector<double> const& bbox)
{
    int MAX_ZOOM = 32;
    for (int z = 0; z < MAX_ZOOM; z++) {
        int mask = (1 << (32 - (z + 1)));
        if (((static_cast<int>(bbox[0]) & mask) != (static_cast<int>(bbox[2]) & mask)) ||
            ((static_cast<int>(bbox[1]) & mask) != (static_cast<int>(bbox[3]) & mask))) {
            return z;
        }
    }

    return MAX_ZOOM;
}

std::vector<unsigned> bboxToXYZ(mapnik::box2d<double> const& bboxCoords)
{
    double minx = bboxCoords.minx();
    double miny = bboxCoords.miny();
    double maxx = bboxCoords.maxx();
    double maxy = bboxCoords.maxy();
    mapnik::merc2lonlat(&minx,&miny,1);
    mapnik::merc2lonlat(&maxx,&maxy,1);

    std::vector<double> ubbox = {
        minx,
        miny,
        maxx,
        maxy
    };
    unsigned z = getBboxZoom(ubbox);
    if (z == 0) return {0, 0, 0};
    minx = pointToTile(minx, miny, 32)[0];
    miny = pointToTile(minx, miny, 32)[1];
    unsigned x = static_cast<unsigned>(minx);
    unsigned y = static_cast<unsigned>(miny);
    return {x, y, z};
}

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
    auto xyz = bboxToXYZ(map_extent);
    /*
    CHECK(xyz[0] == 0);
    CHECK(xyz[1] == 0);
    CHECK(xyz[2] == 0);
    */
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
    auto xyz = bboxToXYZ(map_extent);
    /*
    CHECK(xyz[0] == x);
    CHECK(xyz[1] == y);
    CHECK(xyz[2] == z);
    */
}

TEST_CASE( "vector tile projection 3", "should support z/x/y to bbox conversion for z3" ) {
    mapnik::vector_tile_impl::spherical_mercator merc(256);
    int x = 3;
    int y = 3;
    int z = 3;
    double minx,miny,maxx,maxy;
    merc.xyz(x,y,z,minx,miny,maxx,maxy);
    mapnik::box2d<double> map_extent(minx,miny,maxx,maxy);
    mapnik::box2d<double> e(-5009377.085697311,0.0,0.0,5009377.085697311);
    double epsilon = 0.00000001;
    CHECK(std::fabs(map_extent.minx() - e.minx()) < epsilon);
    CHECK(std::fabs(map_extent.miny() - e.miny()) < epsilon);
    CHECK(std::fabs(map_extent.maxx() - e.maxx()) < epsilon);
    CHECK(std::fabs(map_extent.maxy() - e.maxy()) < epsilon);
    auto xyz = bboxToXYZ(map_extent);
    /*
    CHECK(xyz[0] == x);
    CHECK(xyz[1] == y);
    CHECK(xyz[2] == z);
    */
}
