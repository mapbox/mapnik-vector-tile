// libprotobuf
#include "vector_tile.pb.h"

// mapnik vector tile
#include "vector_tile_geometry_decoder.hpp"

template <typename T>
mapnik::vector_tile_impl::GeometryPBF<T> feature_to_pbf_geometry(std::string const& feature_string);
