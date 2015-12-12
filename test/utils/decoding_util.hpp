// mapnik vector tile
#include "vector_tile_geometry_decoder.hpp"

template <typename T>
mapnik::vector_tile_impl::GeometryPBF<T> feature_to_pbf_geometry(std::string const& feature_string, double scale_x = 1.0, double scale_y = 1.0);
