#ifndef __MAPNIK_VECTOR_TILE_GEOMETRY_CLIPPER_H__
#define __MAPNIK_VECTOR_TILE_GEOMETRY_CLIPPER_H__

// mapnik
#include <mapnik/box2d.hpp>
#include <mapnik/feature.hpp>
#include <mapnik/geometry.hpp>

// angus clipper
// http://www.angusj.com/delphi/clipper.php
#include "clipper.hpp"

namespace mapnik
{

namespace vector_tile_impl
{

template <typename T>
struct geometry_clipper 
{
    typedef T backend_type;

    geometry_clipper(backend_type & backend,
                     mapnik::feature_impl const& feature,
                     mapnik::box2d<int> const& tile_clipping_extent,
                     double area_threshold,
                     bool strictly_simple,
                     bool multi_polygon_union,
                     ClipperLib::PolyFillType fill_type,
                     bool process_all_rings);

    bool operator() (mapnik::geometry::geometry_empty const&)
    {
        return false;
    }

    bool operator() (mapnik::geometry::geometry_collection<std::int64_t> & geom);

    bool operator() (mapnik::geometry::point<std::int64_t> const& geom);

    bool operator() (mapnik::geometry::multi_point<std::int64_t> const& geom);

    bool operator() (mapnik::geometry::line_string<std::int64_t> & geom);
    
    bool operator() (mapnik::geometry::multi_line_string<std::int64_t> & geom);

    bool operator() (mapnik::geometry::polygon<std::int64_t> & geom);

    bool operator() (mapnik::geometry::multi_polygon<std::int64_t> & geom);

    backend_type & backend_;
    mapnik::feature_impl const& feature_;
    mapnik::box2d<int> const& tile_clipping_extent_;
    double area_threshold_;
    bool strictly_simple_;
    bool multi_polygon_union_;
    ClipperLib::PolyFillType fill_type_;
    bool process_all_rings_;
};

} // end ns vector_tile_impl

} // end ns mapnik

#if !defined(MAPNIK_VECTOR_TILE_LIBRARY)
#include "vector_tile_geometry_clipper.ipp"
#endif

#endif // __MAPNIK_VECTOR_GEOMETRY_CLIPPER_H__
