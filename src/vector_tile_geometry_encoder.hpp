#ifndef __MAPNIK_VECTOR_TILE_GEOMETRY_ENCODER_H__
#define __MAPNIK_VECTOR_TILE_GEOMETRY_ENCODER_H__

// vector tile
#include "vector_tile.pb.h"

namespace mapnik { namespace vector_tile_impl {

inline void handle_skipped_last(vector_tile::Tile_Feature & current_feature,
                                int32_t skipped_index,
                                int32_t cur_x,
                                int32_t cur_y,
                                int32_t & x_,
                                int32_t & y_);

template <typename T>
unsigned encode_geometry(T & path,
                         vector_tile::Tile_GeomType type,
                         vector_tile::Tile_Feature & current_feature,
                         int32_t & x_,
                         int32_t & y_,
                         unsigned tolerance,
                         unsigned path_multiplier);

}} // end ns

#if !defined(MAPNIK_VECTOR_TILE_LIBRARY)
#include "vector_tile_geometry_encoder.ipp"
#endif

#endif // __MAPNIK_VECTOR_TILE_GEOMETRY_ENCODER_H__
