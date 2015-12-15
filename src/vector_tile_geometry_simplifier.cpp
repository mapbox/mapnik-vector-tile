#include "vector_tile_geometry_simplifier.hpp"
#include "vector_tile_geometry_simplifier.ipp"
#include "vector_tile_backend_pbf.hpp"
template struct mapnik::vector_tile_impl::geometry_simplifier<mapnik::vector_tile_impl::backend_pbf>;
