#include "vector_tile_geometry_decoder.hpp"
#include "vector_tile_geometry_decoder.ipp"

namespace mapnik
{

namespace vector_tile_impl
{

// decode geometry
template mapnik::geometry::geometry<double> decode_geometry<double>(GeometryPBF & paths, 
                                                                    int32_t geom_type, 
                                                                    unsigned version,
                                                                    double tile_x,
                                                                    double tile_y,
                                                                    double scale_x, 
                                                                    double scale_y);
template mapnik::geometry::geometry<std::int64_t> decode_geometry<std::int64_t>(GeometryPBF & paths, 
                                                                                int32_t geom_type, 
                                                                                unsigned version,
                                                                                std::int64_t tile_x,
                                                                                std::int64_t tile_y,
                                                                                double scale_x, 
                                                                                double scale_y);
template mapnik::geometry::geometry<double> decode_geometry<double>(GeometryPBF & paths, 
                                                                    int32_t geom_type, 
                                                                    unsigned version,
                                                                    double tile_x,
                                                                    double tile_y,
                                                                    double scale_x, 
                                                                    double scale_y,
                                                                    mapnik::box2d<double> const& bbox);
template mapnik::geometry::geometry<std::int64_t> decode_geometry<std::int64_t>(GeometryPBF & paths, 
                                                                                int32_t geom_type, 
                                                                                unsigned version,
                                                                                std::int64_t tile_x,
                                                                                std::int64_t tile_y,
                                                                                double scale_x, 
                                                                                double scale_y,
                                                                                mapnik::box2d<double> const& bbox);

} // end ns vector_tile_impl

} // end ns mapnik
