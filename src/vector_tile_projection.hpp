#ifndef __MAPNIK_VECTOR_TILE_PROJECTION_H__
#define __MAPNIK_VECTOR_TILE_PROJECTION_H__

// mapnik-vector-tile
#include "vector_tile_config.hpp"

// mapnik
#include <mapnik/geometry/box2d.hpp>
#include <mapnik/well_known_srs.hpp>

namespace mapnik
{

namespace vector_tile_impl
{

inline mapnik::box2d<double> tile_mercator_bbox(std::uint64_t x,
                                                std::uint64_t y,
                                                std::uint64_t z)
{
    const double half_of_equator = M_PI * EARTH_RADIUS;
    const double tile_size = 2.0 * half_of_equator / (1ull << z);
    double minx = -half_of_equator + x * tile_size;
    double miny = half_of_equator - (y + 1.0) * tile_size;
    double maxx = -half_of_equator + (x + 1.0) * tile_size;
    double maxy = half_of_equator - y * tile_size;
    return mapnik::box2d<double>(minx,miny,maxx,maxy);
}

} // end vector_tile_impl ns

} // end mapnik ns

#endif // __MAPNIK_VECTOR_TILE_PROJECTION_H__
