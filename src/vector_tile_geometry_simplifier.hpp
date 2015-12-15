#ifndef __MAPNIK_VECTOR_TILE_GEOMETRY_SIMPLIFIER_H__
#define __MAPNIK_VECTOR_TILE_GEOMETRY_SIMPLIFIER_H__

// mapnik-vector-tile
#include "vector_tile_geometry_clipper.hpp"

// mapnik
#include <mapnik/geometry.hpp>

namespace mapnik
{

namespace vector_tile_impl
{

template <typename T>
struct geometry_simplifier 
{
    typedef T backend_type;

    geometry_simplifier(double simplify_distance,
                        geometry_clipper<backend_type> & encoder);

    bool operator() (mapnik::geometry::geometry_empty const&)
    {
        return false;
    }

    bool operator() (mapnik::geometry::point<std::int64_t> const& geom);

    bool operator() (mapnik::geometry::multi_point<std::int64_t> const& geom);

    bool operator() (mapnik::geometry::line_string<std::int64_t> const& geom);

    bool operator() (mapnik::geometry::multi_line_string<std::int64_t> const& geom);

    bool operator() (mapnik::geometry::polygon<std::int64_t> const& geom);

    bool operator() (mapnik::geometry::multi_polygon<std::int64_t> const& geom);

    bool operator() (mapnik::geometry::geometry_collection<std::int64_t> const& geom);
    
    geometry_clipper<backend_type> & encoder_;
    unsigned simplify_distance_;
};

} // end ns vector_tile_impl

} // end ns mapnik

#if !defined(MAPNIK_VECTOR_TILE_LIBRARY)
#include "vector_tile_geometry_simplifier.ipp"
#endif

#endif // __MAPNIK_VECTOR_GEOMETRY_SIMPLIFIER_H__
