// mapnik-vector-tile
#include "vector_tile_config.hpp"

// mapnik
#include <mapnik/box2d.hpp>
#include <mapnik/well_known_srs.hpp>

// std
#include <cmath>
#include <cstdint>

#ifndef M_PI
#define M_PI 3.141592653589793238462643
#endif

namespace mapnik 
{ 

namespace vector_tile_impl 
{

MAPNIK_VECTOR_INLINE void spherical_mercator::xyz(std::uint64_t x,
                                                  std::uint64_t y,
                                                  std::uint64_t z,
                                                  double & minx,
                                                  double & miny,
                                                  double & maxx,
                                                  double & maxy)
{
    const double half_of_equator = M_PI * EARTH_RADIUS;
    const double tile_size = 2.0 * half_of_equator / (1ul << z);
    minx = -half_of_equator + x * tile_size;
    miny = half_of_equator - (y + 1.0) * tile_size;
    maxx = -half_of_equator + (x + 1.0) * tile_size;
    maxy = half_of_equator - y * tile_size;
}

MAPNIK_VECTOR_INLINE mapnik::box2d<double> merc_extent(std::uint32_t tile_size, 
                                                       std::uint64_t x, 
                                                       std::uint64_t y, 
                                                       std::uint64_t z)
{
    spherical_mercator merc(tile_size);
    double minx,miny,maxx,maxy;
    merc.xyz(x, y, z, minx, miny, maxx, maxy);
    return mapnik::box2d<double>(minx,miny,maxx,maxy);
}

} // end vector_tile_impl ns

} // end mapnik ns
