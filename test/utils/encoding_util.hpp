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

std::string compare(mapnik::geometry::geometry<std::int64_t> const& g, unsigned version);

std::string compare_pbf(mapnik::geometry::geometry<std::int64_t> const& g, unsigned version);

#endif // __MAPNIK_VECTOR_TILE_TEST_ENCODING_UTIL_H__
