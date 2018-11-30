#ifndef __MAPNIK_VECTOR_TILE_GEOMETRY_ENCODER_PBF_H__
#define __MAPNIK_VECTOR_TILE_GEOMETRY_ENCODER_PBF_H__

// mapnik-vector-tile
#include "vector_tile_config.hpp"

// mapbox
#include <mapbox/geometry/geometry.hpp>

// protozero
#include <protozero/varint.hpp>
#include <protozero/pbf_writer.hpp>

// std
#include <cstdlib>
#include <cmath>

namespace mapnik
{

namespace vector_tile_impl
{

template <typename T>
MAPNIK_VECTOR_INLINE bool encode_geometry_pbf(T const& pt,
                                              protozero::pbf_writer & current_feature,
                                              int32_t & start_x,
                                              int32_t & start_y);
template <>
MAPNIK_VECTOR_INLINE bool encode_geometry_pbf(mapbox::geometry::point<std::int64_t> const& pt,
                                              protozero::pbf_writer & current_feature,
                                              int32_t & start_x,
                                              int32_t & start_y);

template <>
MAPNIK_VECTOR_INLINE bool encode_geometry_pbf(mapbox::geometry::multi_point<std::int64_t> const& geom,
                                              protozero::pbf_writer & current_feature,
                                              int32_t & start_x,
                                              int32_t & start_y);

template <>
MAPNIK_VECTOR_INLINE bool encode_geometry_pbf(mapbox::geometry::line_string<std::int64_t> const& line,
                                              protozero::pbf_writer & current_feature,
                                              int32_t & start_x,
                                              int32_t & start_y);
template <>
MAPNIK_VECTOR_INLINE bool encode_geometry_pbf(mapbox::geometry::multi_line_string<std::int64_t> const& geom,
                                              protozero::pbf_writer & current_feature,
                                              int32_t & start_x,
                                              int32_t & start_y);

template <>
MAPNIK_VECTOR_INLINE bool encode_geometry_pbf(mapbox::geometry::polygon<std::int64_t> const& poly,
                                              protozero::pbf_writer & current_feature,
                                              int32_t & start_x,
                                              int32_t & start_y);
template <>
MAPNIK_VECTOR_INLINE bool encode_geometry_pbf(mapbox::geometry::multi_polygon<std::int64_t> const& poly,
                                              protozero::pbf_writer & current_feature,
                                              int32_t & start_x,
                                              int32_t & start_y);
template <>
MAPNIK_VECTOR_INLINE bool encode_geometry_pbf(mapbox::geometry::geometry<std::int64_t> const& geom,
                                              protozero::pbf_writer & current_feature,
                                              int32_t & start_x,
                                              int32_t & start_y);

} // end ns vector_tile_impl

} // end ns mapnik

#if !defined(MAPNIK_VECTOR_TILE_LIBRARY)
#include "vector_tile_geometry_encoder_pbf.ipp"
#endif

#endif // __MAPNIK_VECTOR_TILE_GEOMETRY_ENCODER_PBF_H__
