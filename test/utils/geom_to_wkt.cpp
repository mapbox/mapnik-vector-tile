// mapnik
#include <mapnik/util/geometry_to_wkt.hpp>

namespace test_utils
{

bool to_wkt(std::string & wkt,  mapnik::geometry::geometry<double> const& geom)
{
    return mapnik::util::to_wkt(wkt, geom);
}

bool to_wkt(std::string & wkt,  mapnik::geometry::geometry<std::int64_t> const& geom)
{
    return mapnik::util::to_wkt(wkt, geom);
}

} // end ns test_utils
