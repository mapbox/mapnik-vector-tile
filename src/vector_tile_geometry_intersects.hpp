#ifndef __MAPNIK_VECTOR_TILE_GEOMETRY_INTERSECTS_H__
#define __MAPNIK_VECTOR_TILE_GEOMETRY_INTERSECTS_H__

// mapnik
#include <mapnik/box2d.hpp>
#include <mapnik/geometry.hpp>

namespace mapnik
{

namespace vector_tile_impl
{

struct geometry_intersects 
{
    geometry_intersects(mapnik::box2d<double> const& extent);

    bool operator() (mapnik::geometry::geometry_empty const& geom);
    
    bool operator() (mapnik::geometry::point<double> const& geom);

    bool operator() (mapnik::geometry::multi_point<double> const& geom);

    bool operator() (mapnik::geometry::geometry_collection<double> const& geom);

    bool operator() (mapnik::geometry::line_string<double> const& geom);
    
    bool operator() (mapnik::geometry::multi_line_string<double> const& geom);

    bool operator() (mapnik::geometry::polygon<double> const& geom);

    bool operator() (mapnik::geometry::multi_polygon<double> const& geom);
    
    mapnik::geometry::linear_ring<double> extent_;
};

bool intersects(mapnik::box2d<double> const& extent, mapnik::geometry::geometry<double> const& geom);

} // end ns vector_tile_impl

} // end ns mapnik

#if !defined(MAPNIK_VECTOR_TILE_LIBRARY)
#include "vector_tile_geometry_intersects.ipp"
#endif

#endif // __MAPNIK_VECTOR_GEOMETRY_INTERSECTS_H__
