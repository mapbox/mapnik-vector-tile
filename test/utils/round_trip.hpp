// mapnik-vector-tile
#include "vector_tile_processor.hpp"

// mapnik
#include <mapnik/geometry.hpp>

namespace test_utils
{

mapnik::geometry::geometry<double> round_trip(mapnik::geometry::geometry<double> const& geom,
                                      double simplify_distance=0.0,
                                      mapnik::vector_tile_impl::polygon_fill_type fill_type = mapnik::vector_tile_impl::non_zero_fill,
                                      bool mpu = false);

} // end ns
