#ifndef __MAPNIK_VECTOR_TILE_TEST_ENCODING_UTIL_H__
#define __MAPNIK_VECTOR_TILE_TEST_ENCODING_UTIL_H__

// mapbox
#include <mapbox/geometry.hpp>

// libprotobuf
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include "vector_tile.pb.h"
#pragma GCC diagnostic pop

std::string compare_pbf(mapbox::geometry::geometry<std::int64_t> const& g, unsigned version);

#endif // __MAPNIK_VECTOR_TILE_TEST_ENCODING_UTIL_H__
