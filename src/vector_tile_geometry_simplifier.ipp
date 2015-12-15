// mapnik
#include <mapnik/geometry.hpp>
#include <mapnik/geometry_adapters.hpp>

// boost
#include <boost/geometry/algorithms/simplify.hpp>

namespace mapnik 
{

namespace vector_tile_impl 
{

template <typename T>
geometry_simplifier<T>::geometry_simplifier(double simplify_distance,
                        geometry_clipper<backend_type> & encoder) :
                        encoder_(encoder),
            simplify_distance_(simplify_distance) {}

template <typename T>
bool geometry_simplifier<T>::operator() (mapnik::geometry::point<std::int64_t> const& geom)
{
    return encoder_(geom);
}

template <typename T>
bool geometry_simplifier<T>::operator() (mapnik::geometry::multi_point<std::int64_t> const& geom)
{
    return encoder_(geom);
}

template <typename T>
bool geometry_simplifier<T>::operator() (mapnik::geometry::line_string<std::int64_t> const& geom)
{
    mapnik::geometry::line_string<std::int64_t> simplified;
    boost::geometry::simplify(geom,simplified,simplify_distance_);
    return encoder_(simplified);
}

template <typename T>
bool geometry_simplifier<T>::operator() (mapnik::geometry::multi_line_string<std::int64_t> const& geom)
{
    mapnik::geometry::multi_line_string<std::int64_t> simplified;
    boost::geometry::simplify(geom,simplified,simplify_distance_);
    return encoder_(simplified);
}

template <typename T>
bool geometry_simplifier<T>::operator() (mapnik::geometry::polygon<std::int64_t> const& geom)
{
    mapnik::geometry::polygon<std::int64_t> simplified;
    boost::geometry::simplify(geom,simplified,simplify_distance_);
    return encoder_(simplified);
}

template <typename T>
bool geometry_simplifier<T>::operator() (mapnik::geometry::multi_polygon<std::int64_t> const& geom)
{
    mapnik::geometry::multi_polygon<std::int64_t> simplified;
    boost::geometry::simplify(geom,simplified,simplify_distance_);
    return encoder_(simplified);
}

template <typename T>
bool geometry_simplifier<T>::operator() (mapnik::geometry::geometry_collection<std::int64_t> const& geom)
{
    bool painted = false;
    for (auto const& g : geom)
    {
        if (mapnik::util::apply_visitor((*this), g))
        {
            painted = true;
        }
    }
    return painted;
}

} // end ns vector_tile_impl

} // end ns mapnik
