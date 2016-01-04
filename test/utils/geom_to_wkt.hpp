// mapnik
#include <mapnik/geometry.hpp>

namespace test_utils
{

bool to_wkt(std::string & wkt,  mapnik::geometry::geometry<double> const& geom);
bool to_wkt(std::string & wkt,  mapnik::geometry::geometry<std::int64_t> const& geom);

} // end ns test_utils
