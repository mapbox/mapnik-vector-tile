#ifndef __MAPNIK_VECTOR_TILE_PROJECTION_H__
#define __MAPNIK_VECTOR_TILE_PROJECTION_H__

#include <cstdint>

#include "vector_tile_config.hpp"

namespace mapnik { namespace vector_tile_impl {

    class spherical_mercator
    {
    private:
        double tile_size_;
    public:
        MAPNIK_VECTOR_INLINE spherical_mercator(unsigned tile_size);

        MAPNIK_VECTOR_INLINE void from_pixels(double shift, double & x, double & y);

        MAPNIK_VECTOR_INLINE void xyz(int x,
                 int y,
                 int z,
                 double & minx,
                 double & miny,
                 double & maxx,
                 double & maxy);
    };

}} // end ns

#if !defined(MAPNIK_VECTOR_TILE_LIBRARY)
#include "vector_tile_projection.ipp"
#endif


#endif // __MAPNIK_VECTOR_TILE_PROJECTION_H__
