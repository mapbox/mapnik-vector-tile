#include "vector_tile_raster_clipper.hpp"
#include "vector_tile_raster_clipper.ipp"
#include "vector_tile_backend_pbf.hpp"
template struct mapnik::vector_tile_impl::raster_clipper<mapnik::vector_tile_impl::backend_pbf>;
