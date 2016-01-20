// mapnik
#include <mapnik/geometry.hpp>
#include <mapnik/geometry_adapters.hpp>

// boost
#include <boost/geometry/algorithms/simplify.hpp>

namespace mapnik 
{

namespace vector_tile_impl 
{

geometry_simplifier::geometry_simplifier(mapnik::geometry::geometry<std::int64_t> & geom,
                                         double simplify_distance)
    : geom_(geom),
       simplify_distance_(simplify_distance) {}

MAPNIK_VECTOR_INLINE void geometry_simplifier::operator() (mapnik::geometry::geometry_empty &)
{
}

MAPNIK_VECTOR_INLINE void geometry_simplifier::operator() (mapnik::geometry::point<std::int64_t> &)
{
}

MAPNIK_VECTOR_INLINE void geometry_simplifier::operator() (mapnik::geometry::multi_point<std::int64_t> &)
{
}

MAPNIK_VECTOR_INLINE void geometry_simplifier::operator() (mapnik::geometry::line_string<std::int64_t> & geom)
{
    mapnik::geometry::line_string<std::int64_t> simplified;
    boost::geometry::simplify(geom, simplified, simplify_distance_);
    geom_ = mapnik::geometry::geometry<std::int64_t>(std::move(simplified));
}

MAPNIK_VECTOR_INLINE void geometry_simplifier::operator() (mapnik::geometry::multi_line_string<std::int64_t> & geom)
{
    mapnik::geometry::multi_line_string<std::int64_t> simplified;
    boost::geometry::simplify(geom, simplified, simplify_distance_);
    geom_ = mapnik::geometry::geometry<std::int64_t>(std::move(simplified));
}

MAPNIK_VECTOR_INLINE void geometry_simplifier::operator() (mapnik::geometry::polygon<std::int64_t> & geom)
{
    mapnik::geometry::polygon<std::int64_t> simplified;
    boost::geometry::simplify(geom, simplified, simplify_distance_);
    geom_ = mapnik::geometry::geometry<std::int64_t>(std::move(simplified));
}

MAPNIK_VECTOR_INLINE void geometry_simplifier::operator() (mapnik::geometry::multi_polygon<std::int64_t> & geom)
{
    mapnik::geometry::multi_polygon<std::int64_t> simplified;
    boost::geometry::simplify(geom, simplified, simplify_distance_);
    geom_ = mapnik::geometry::geometry<std::int64_t>(std::move(simplified));
}

MAPNIK_VECTOR_INLINE void geometry_simplifier::operator() (mapnik::geometry::geometry_collection<std::int64_t> & geom)
{
    for (auto & g : geom)
    {
        geometry_simplifier visitor(g, simplify_distance_);
        mapnik::util::apply_visitor(visitor, g);
    }
}

} // end ns vector_tile_impl

} // end ns mapnik
