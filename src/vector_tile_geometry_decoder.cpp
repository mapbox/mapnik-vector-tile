#include "vector_tile_geometry_decoder.hpp"
#include "vector_tile_geometry_decoder.ipp"

namespace mapnik
{

namespace vector_tile_impl
{

// Geometry classes
template class GeometryPBF<double>;
template class GeometryPBF<std::int64_t>;

// decode geometry
template mapnik::geometry::geometry<double> decode_geometry(GeometryPBF<double> & paths, int32_t geom_type, unsigned version);
template mapnik::geometry::geometry<std::int64_t> decode_geometry(GeometryPBF<std::int64_t> & paths, int32_t geom_type, unsigned version);

} // end ns vector_tile_impl

} // end ns mapnik
