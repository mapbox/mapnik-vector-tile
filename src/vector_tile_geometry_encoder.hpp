#ifndef __MAPNIK_VECTOR_TILE_GEOMETRY_ENCODER_H__
#define __MAPNIK_VECTOR_TILE_GEOMETRY_ENCODER_H__

// mapnik-vector-tile
#include "vector_tile_config.hpp"

// vector tile
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include "vector_tile.pb.h"
#pragma GCC diagnostic pop

// mapnik
#include <mapnik/geometry.hpp>

// protozero
#include <protozero/varint.hpp>

// std
#include <cstdlib>
#include <cmath>

namespace mapnik
{
    
namespace vector_tile_impl
{

MAPNIK_VECTOR_INLINE bool encode_geometry(mapnik::geometry::point<std::int64_t> const& pt,
                                          vector_tile::Tile_Feature & current_feature,
                                          int32_t & start_x,
                                          int32_t & start_y);

MAPNIK_VECTOR_INLINE bool encode_geometry(mapnik::geometry::multi_point<std::int64_t> const& geom,
                                          vector_tile::Tile_Feature & current_feature,
                                          int32_t & start_x,
                                          int32_t & start_y);

MAPNIK_VECTOR_INLINE bool encode_geometry(mapnik::geometry::line_string<std::int64_t> const& line,
                                          vector_tile::Tile_Feature & current_feature,
                                          int32_t & start_x,
                                          int32_t & start_y);

MAPNIK_VECTOR_INLINE bool encode_geometry(mapnik::geometry::multi_line_string<std::int64_t> const& geom,
                                          vector_tile::Tile_Feature & current_feature,
                                          int32_t & start_x,
                                          int32_t & start_y);

MAPNIK_VECTOR_INLINE bool encode_geometry(mapnik::geometry::polygon<std::int64_t> const& poly,
                                          vector_tile::Tile_Feature & current_feature,
                                          int32_t & start_x,
                                          int32_t & start_y);

MAPNIK_VECTOR_INLINE bool encode_geometry(mapnik::geometry::multi_polygon<std::int64_t> const& poly,
                                          vector_tile::Tile_Feature & current_feature,
                                          int32_t & start_x,
                                          int32_t & start_y);

MAPNIK_VECTOR_INLINE bool encode_geometry(mapnik::geometry::geometry<std::int64_t> const& geom,
                                          vector_tile::Tile_Feature & current_feature,
                                          int32_t & start_x,
                                          int32_t & start_y);

} // end ns vector_tile_impl

} // end ns mapnik

#endif // __MAPNIK_VECTOR_TILE_GEOMETRY_ENCODER_H__
