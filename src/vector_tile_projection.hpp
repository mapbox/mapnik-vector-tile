#ifndef __MAPNIK_VECTOR_TILE_PROJECTION_H__
#define __MAPNIK_VECTOR_TILE_PROJECTION_H__

#include <cstdint>


namespace mapnik { namespace vector_tile_impl {

    class spherical_mercator
    {
    private:
        double tile_size_;
    public:
        spherical_mercator(unsigned tile_size);

        void from_pixels(double shift, double & x, double & y);

        void xyz(int x,
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
