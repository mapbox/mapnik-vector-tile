#ifndef __MAPNIK_VECTOR_TILE_TEST_DECODING_UTIL_H__
#define __MAPNIK_VECTOR_TILE_TEST_DECODING_UTIL_H__

// mapnik vector tile
#include "vector_tile_geometry_decoder.hpp"

mapnik::vector_tile_impl::GeometryPBF feature_to_pbf_geometry(std::string const& feature_string);

#endif // __MAPNIK_VECTOR_TILE_TEST_DECODING_UTIL_H__
