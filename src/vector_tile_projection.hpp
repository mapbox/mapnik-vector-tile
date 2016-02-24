#ifndef __MAPNIK_VECTOR_TILE_PROJECTION_H__
#define __MAPNIK_VECTOR_TILE_PROJECTION_H__

// mapnik-vector-tile
#include "vector_tile_config.hpp"

// mapnik
#include <mapnik/box2d.hpp>

namespace mapnik 
{ 

namespace vector_tile_impl 
{

class spherical_mercator
{
private:
    double tile_size_;
public:
    spherical_mercator(unsigned tile_size)
      : tile_size_(static_cast<double>(tile_size)) {}

    MAPNIK_VECTOR_INLINE void from_pixels(double shift, double & x, double & y);

    MAPNIK_VECTOR_INLINE void xyz(int x,
                                  int y,
                                  int z,
                                  double & minx,
                                  double & miny,
                                  double & maxx,
                                  double & maxy);
};

MAPNIK_VECTOR_INLINE mapnik::box2d<double> merc_extent(std::uint32_t tile_size, int x, int y, int z);

} // end vector_tile_impl ns

} // end mapnik ns

#if !defined(MAPNIK_VECTOR_TILE_LIBRARY)
#include "vector_tile_projection.ipp"
#endif

#endif // __MAPNIK_VECTOR_TILE_PROJECTION_H__
