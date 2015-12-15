#include "vector_tile_geometry_clipper.hpp"
#include "vector_tile_geometry_clipper.ipp"
#include "vector_tile_backend_pbf.hpp"
template struct mapnik::vector_tile_impl::geometry_clipper<mapnik::vector_tile_impl::backend_pbf>;
