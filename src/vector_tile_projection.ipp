#include <mapnik/box2d.hpp>
#include <mapnik/well_known_srs.hpp>
#include <cmath>

#ifndef M_PI
#define M_PI 3.141592653589793238462643
#endif

namespace mapnik { namespace vector_tile_impl {

spherical_mercator::spherical_mercator(unsigned tile_size)
  : tile_size_(static_cast<double>(tile_size)) {}

void spherical_mercator::from_pixels(double shift, double & x, double & y)
{
    double b = shift/2.0;
    x = (x - b)/(shift/360.0);
    double g = (y - b)/-(shift/(2 * M_PI));
    y = R2D * (2.0 * std::atan(std::exp(g)) - M_PI_by2);
}

void spherical_mercator::xyz(int x,
         int y,
         int z,
         double & minx,
         double & miny,
         double & maxx,
         double & maxy)
{
    minx = x * tile_size_;
    miny = (y + 1.0) * tile_size_;
    maxx = (x + 1.0) * tile_size_;
    maxy = y * tile_size_;
    double shift = std::pow(2.0,z) * tile_size_;
    from_pixels(shift,minx,miny);
    from_pixels(shift,maxx,maxy);
    lonlat2merc(&minx,&miny,1);
    lonlat2merc(&maxx,&maxy,1);
}

}} // end ns