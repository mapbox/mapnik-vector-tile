// mapnik
#include <mapnik/box2d.hpp>
#include <mapnik/geometry.hpp>
#include <mapnik/geometry_adapters.hpp>
#include <mapnik/util/variant.hpp>

// boost
#include <boost/geometry.hpp>

namespace mapnik 
{

namespace vector_tile_impl 
{

geometry_intersects::geometry_intersects(mapnik::box2d<double> const & extent)
    : extent_()
{
    extent_.reserve(5);
    extent_.emplace_back(extent.minx(),extent.miny());
    extent_.emplace_back(extent.maxx(),extent.miny());
    extent_.emplace_back(extent.maxx(),extent.maxy());
    extent_.emplace_back(extent.minx(),extent.maxy());
    extent_.emplace_back(extent.minx(),extent.miny());
}

bool geometry_intersects::operator() (mapnik::geometry::geometry_empty const& )
{
    return false;
}

bool geometry_intersects::operator() (mapnik::geometry::point<double> const& )
{
    // The query should gaurantee that a point is within the bounding box!
    return true;
}

bool geometry_intersects::operator() (mapnik::geometry::multi_point<double> const& geom)
{
    for (auto const& g : geom)
    {
        if (boost::geometry::intersects(g, extent_))
        {
            return true;
        }
    }
    return false;
}

bool geometry_intersects::operator() (mapnik::geometry::line_string<double> const& geom)
{
    return boost::geometry::intersects(geom, extent_);
}

bool geometry_intersects::operator() (mapnik::geometry::multi_line_string<double> const& geom)
{
    return boost::geometry::intersects(geom, extent_);
}

bool geometry_intersects::operator() (mapnik::geometry::polygon<double> const& geom)
{
    return boost::geometry::intersects(geom, extent_);
}

bool geometry_intersects::operator() (mapnik::geometry::multi_polygon<double> const& geom)
{
    return boost::geometry::intersects(geom, extent_);
}

bool geometry_intersects::operator() (mapnik::geometry::geometry_collection<double> const& geom)
{
    for (auto const& g : geom)
    {
        if (mapnik::util::apply_visitor(*this, g))
        {
            return true;
        }
    }
    return false;
}

bool intersects(mapnik::box2d<double> const& extent, mapnik::geometry::geometry<double> const& geom)
{
    return mapnik::util::apply_visitor(geometry_intersects(extent), geom); 
}

} // end ns vector_tile_impl

} // end ns mapnik

