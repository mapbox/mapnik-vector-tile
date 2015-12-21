// mapnik
#include <mapnik/geometry.hpp>
#include <mapnik/geometry_adapters.hpp>

// boost
#include <boost/geometry/algorithms/simplify.hpp>

namespace mapnik 
{

namespace vector_tile_impl 
{

geometry_simplifier::geometry_simplifier(double simplify_distance,
                                         geometry_clipper & clipper) 
    : clipper_(clipper),
       simplify_distance_(simplify_distance) {}

mapnik::geometry::geometry<int64_t> geometry_simplifier::operator() (mapnik::geometry::geometry_empty & geom)
{
    return clipper_(geom);
}

mapnik::geometry::geometry<int64_t> geometry_simplifier::operator() (mapnik::geometry::point<std::int64_t> & geom)
{
    return clipper_(geom);
}

mapnik::geometry::geometry<int64_t> geometry_simplifier::operator() (mapnik::geometry::multi_point<std::int64_t> & geom)
{
    return clipper_(geom);
}

mapnik::geometry::geometry<int64_t> geometry_simplifier::operator() (mapnik::geometry::line_string<std::int64_t> const& geom)
{
    mapnik::geometry::line_string<std::int64_t> simplified;
    boost::geometry::simplify(geom, simplified, simplify_distance_);
    return clipper_(simplified);
}

mapnik::geometry::geometry<int64_t> geometry_simplifier::operator() (mapnik::geometry::multi_line_string<std::int64_t> const& geom)
{
    mapnik::geometry::multi_line_string<std::int64_t> simplified;
    boost::geometry::simplify(geom, simplified, simplify_distance_);
    return clipper_(simplified);
}

mapnik::geometry::geometry<int64_t> geometry_simplifier::operator() (mapnik::geometry::polygon<std::int64_t> const& geom)
{
    mapnik::geometry::polygon<std::int64_t> simplified;
    boost::geometry::simplify(geom, simplified, simplify_distance_);
    return clipper_(simplified);
}

mapnik::geometry::geometry<int64_t> geometry_simplifier::operator() (mapnik::geometry::multi_polygon<std::int64_t> const& geom)
{
    mapnik::geometry::multi_polygon<std::int64_t> simplified;
    boost::geometry::simplify(geom, simplified, simplify_distance_);
    return clipper_(simplified);
}

mapnik::geometry::geometry<int64_t> geometry_simplifier::operator() (mapnik::geometry::geometry_collection<std::int64_t> & geom)
{
    mapnik::geometry::geometry_collection<std::int64_t> out_geom;
    out_geom.reserve(geom.size());
    for (auto & g : geom)
    {
        out_geom.emplace_back(mapnik::util::apply_visitor((*this), g));
    }
    return out_geom;
}

} // end ns vector_tile_impl

} // end ns mapnik
