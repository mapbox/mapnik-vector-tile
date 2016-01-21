#ifndef __MAPNIK_VECTOR_TILE_GEOMETRY_SIMPLIFIER_H__
#define __MAPNIK_VECTOR_TILE_GEOMETRY_SIMPLIFIER_H__

// mapnik-vector-tile
#include "vector_tile_config.hpp"

// mapnik
#include <mapnik/geometry.hpp>

namespace mapnik
{

namespace vector_tile_impl
{

struct geometry_simplifier 
{
    geometry_simplifier(mapnik::geometry::geometry<std::int64_t> & geom,
                        double simplify_distance)
        : geom_(geom),
          simplify_distance_(simplify_distance) {}

    MAPNIK_VECTOR_INLINE void operator() (mapnik::geometry::geometry_empty & geom);
    
    MAPNIK_VECTOR_INLINE void operator() (mapnik::geometry::point<std::int64_t> & geom);

    MAPNIK_VECTOR_INLINE void operator() (mapnik::geometry::multi_point<std::int64_t> & geom);

    MAPNIK_VECTOR_INLINE void operator() (mapnik::geometry::geometry_collection<std::int64_t> & geom);

    MAPNIK_VECTOR_INLINE void operator() (mapnik::geometry::line_string<std::int64_t> & geom);
    
    MAPNIK_VECTOR_INLINE void operator() (mapnik::geometry::multi_line_string<std::int64_t> & geom);

    MAPNIK_VECTOR_INLINE void operator() (mapnik::geometry::polygon<std::int64_t> & geom);

    MAPNIK_VECTOR_INLINE void operator() (mapnik::geometry::multi_polygon<std::int64_t> & geom);
    
    mapnik::geometry::geometry<std::int64_t> & geom_;
    unsigned simplify_distance_;
};

} // end ns vector_tile_impl

} // end ns mapnik

#if !defined(MAPNIK_VECTOR_TILE_LIBRARY)
#include "vector_tile_geometry_simplifier.ipp"
#endif

#endif // __MAPNIK_VECTOR_GEOMETRY_SIMPLIFIER_H__
