#ifndef __MAPNIK_VECTOR_TILE_PROJECTION_H__
#define __MAPNIK_VECTOR_TILE_PROJECTION_H__

#include <mapnik/box2d.hpp>
#include <mapnik/well_known_srs.hpp>
#include <cmath>

#ifndef M_PI
#define M_PI 3.141592653589793238462643
#endif

namespace mapnik { namespace vector {

    class spherical_mercator
    {
    private:
        double tile_size_;
    public:
        spherical_mercator(unsigned tile_size)
          : tile_size_(static_cast<double>(tile_size)) {}

        void to_pixels(double shift, double &x, double &y)
        {
            double d = shift/2.0;
            double f = minmax(std::sin(D2R * y),-0.9999,0.9999);
            x = round(d + x * (shift/360.0));
            y = round(d + 0.5*std::log((1+f)/(1-f))*-(shift/(2 * M_PI)));
        }

        void from_pixels(double shift, double & x, double & y)
        {
            double b = shift/2.0;
            x = (x - b)/(shift/360.0);
            double g = (y - b)/-(shift/(2 * M_PI));
            y = R2D * (2.0 * std::atan(std::exp(g)) - M_PI_by2);
        }

        void xyz(int x,
                 int y,
                 int z,
                 double & minx,
                 double & miny,
                 double & maxx,
                 double & maxy,
                 bool wgs84 = false)
        {
            minx = x * tile_size_;
            miny = (y + 1.0) * tile_size_;
            maxx = (x + 1.0) * tile_size_;
            maxy = y * tile_size_;
            double shift = std::pow(2.0,z) * tile_size_;
            from_pixels(shift,minx,miny);
            from_pixels(shift,maxx,maxy);
            if (!wgs84) {
                lonlat2merc(&minx,&miny,1);
                lonlat2merc(&maxx,&maxy,1);
            }
        }
    private:
        double minmax(double a, double b, double c)
        {
#ifndef MIN
#define MIN(x,y) ((x)<(y)?(x):(y))
#endif
#ifndef MAX
#define MAX(x,y) ((x)>(y)?(x):(y))
#endif
            a = MAX(a,b);
            a = MIN(a,c);
            return a;
        }
    };
    }} // end ns

#endif // __MAPNIK_VECTOR_TILE_PROJECTION_H__
