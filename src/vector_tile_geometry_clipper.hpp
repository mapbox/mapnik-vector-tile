#ifndef __MAPNIK_VECTOR_TILE_GEOMETRY_CLIPPER_H__
#define __MAPNIK_VECTOR_TILE_GEOMETRY_CLIPPER_H__

// mapnik-vector-tile
#include "vector_tile_config.hpp"

// mapnik
#include <mapnik/box2d.hpp>
#include <mapnik/geometry.hpp>

namespace mapnik
{

namespace vector_tile_impl
{

enum polygon_fill_type : std::uint8_t {
    even_odd_fill = 0, 
    non_zero_fill, 
    positive_fill, 
    negative_fill,
    polygon_fill_type_max
};

struct geometry_clipper 
{
    geometry_clipper(mapnik::geometry::geometry<std::int64_t> & geom,
                     mapnik::box2d<int> const& tile_clipping_extent,
                     double area_threshold,
                     bool strictly_simple,
                     bool multi_polygon_union,
                     polygon_fill_type fill_type,
                     bool process_all_rings) :
              geom_(geom),
              tile_clipping_extent_(tile_clipping_extent),
              area_threshold_(area_threshold),
              strictly_simple_(strictly_simple),
              multi_polygon_union_(multi_polygon_union),
              fill_type_(fill_type),
              process_all_rings_(process_all_rings)
    {
    }

    MAPNIK_VECTOR_INLINE void operator() (mapnik::geometry::geometry_empty & geom);
    
    MAPNIK_VECTOR_INLINE void operator() (mapnik::geometry::point<std::int64_t> & geom);
    
    MAPNIK_VECTOR_INLINE void operator() (mapnik::geometry::multi_point<std::int64_t> & geom);

    MAPNIK_VECTOR_INLINE void operator() (mapnik::geometry::line_string<std::int64_t> & geom);
    
    MAPNIK_VECTOR_INLINE void operator() (mapnik::geometry::multi_line_string<std::int64_t> & geom);

    MAPNIK_VECTOR_INLINE void operator() (mapnik::geometry::polygon<std::int64_t> & geom);

    MAPNIK_VECTOR_INLINE void operator() (mapnik::geometry::multi_polygon<std::int64_t> & geom);

    MAPNIK_VECTOR_INLINE void operator() (mapnik::geometry::geometry_collection<std::int64_t> & geom);

    mapnik::geometry::geometry<std::int64_t> & geom_;
    mapnik::box2d<int> const& tile_clipping_extent_;
    double area_threshold_;
    bool strictly_simple_;
    bool multi_polygon_union_;
    polygon_fill_type fill_type_;
    bool process_all_rings_;
};

} // end ns vector_tile_impl

} // end ns mapnik

#if !defined(MAPNIK_VECTOR_TILE_LIBRARY)
#include "vector_tile_geometry_clipper.ipp"
#endif

#endif // __MAPNIK_VECTOR_GEOMETRY_CLIPPER_H__
