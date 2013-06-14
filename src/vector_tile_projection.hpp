#ifndef __MAPNIK_VECTOR_TILE_PROJECTION_H__
#define __MAPNIK_VECTOR_TILE_PROJECTION_H__

#include <mapnik/version.hpp>
#include <mapnik/projection.hpp>
#include <mapnik/proj_transform.hpp>
#include <cmath>

#ifndef M_PI
#define M_PI 3.141592653589793238462643
#endif

#ifndef RAD_TO_DEG
#define RAD_TO_DEG (180/M_PI)
#endif

#ifndef DEG_TO_RAD
#define DEG_TO_RAD (M_PI/180)
#endif

#include <cmath>

namespace mapnik { namespace vector {

    template <unsigned levels=19>
    class spherical_mercator
    {
        double Ac[levels];
        double Bc[levels];
        double Cc[levels];
        double zc[levels];
        unsigned tile_size_;

    public:
        spherical_mercator(unsigned tile_size) :
            tile_size_(tile_size)
        {
            unsigned d;
            unsigned c = tile_size_;
            for (d=0; d<levels; d++) {
                unsigned e = static_cast<unsigned>(c/2.0);
                Bc[d] = c/360.0;
                Cc[d] = c/(2 * M_PI);
                zc[d] = e;
                Ac[d] = c;
                c *=2;
            }
        }

        void to_pixels(double &x, double &y, int zoom)
        {
            double d = zc[zoom];
            double f = minmax(std::sin(DEG_TO_RAD * y),-0.9999,0.9999);
            x = round(d + x * Bc[zoom]);
            y = round(d + 0.5*std::log((1+f)/(1-f))*-Cc[zoom]);
        }

        void from_pixels(double &x, double &y, int zoom)
        {
            double e = zc[zoom];
            double g = (y - e)/-Cc[zoom];
            x = (x - e)/Bc[zoom];
            y = RAD_TO_DEG * ( 2 * std::atan(std::exp(g)) - 0.5 * M_PI);
        }

        void xyz(mapnik::box2d<double> & bbox,
                 unsigned x,
                 unsigned y,
                 unsigned z)
        {
            if (z > levels) throw std::runtime_error("invalid z value");
            mapnik::projection src("+init=epsg:4326",true);
            mapnik::projection dest("+init=epsg:3857",true);
            mapnik::proj_transform tr(src,dest);
            double minx = x * tile_size_;
            double miny = (y + 1) * tile_size_;
            double maxx = (x + 1) * tile_size_;
            double maxy = y * tile_size_;
            from_pixels(minx,miny,z);
            from_pixels(maxx,maxy,z);
            bbox.init(minx,miny,maxx,maxy);
            tr.forward(bbox);
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
