#ifndef __MAPNIK_VECTOR_TILE_TEST_ENCODING_UTIL_H__
#define __MAPNIK_VECTOR_TILE_TEST_ENCODING_UTIL_H__

// mapnik
#include <mapnik/geometry.hpp>

// libprotobuf
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include "vector_tile.pb.h"
#pragma GCC diagnostic pop

struct show_path;

struct encoder_visitor;

vector_tile::Tile_Feature geometry_to_feature(mapnik::geometry::geometry<std::int64_t> const& g);

template <typename T>
std::string decode_to_path_string(mapnik::geometry::geometry<T> const& g);

std::string compare(mapnik::geometry::geometry<std::int64_t> const& g, unsigned version);

std::string compare_pbf(mapnik::geometry::geometry<std::int64_t> const& g, unsigned version);

#endif // __MAPNIK_VECTOR_TILE_TEST_ENCODING_UTIL_H__
