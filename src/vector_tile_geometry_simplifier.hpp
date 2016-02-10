#ifndef __MAPNIK_VECTOR_TILE_GEOMETRY_SIMPLIFIER_H__
#define __MAPNIK_VECTOR_TILE_GEOMETRY_SIMPLIFIER_H__

// mapnik-vector-tile
#include "vector_tile_config.hpp"

// mapnik
#include <mapnik/geometry.hpp>
#include <mapnik/geometry_adapters.hpp>

// boost
#include <boost/geometry/algorithms/simplify.hpp>

namespace mapnik
{

namespace vector_tile_impl
{

template <typename NextProcessor>
struct geometry_simplifier 
{
    geometry_simplifier(unsigned simplify_distance,
                        NextProcessor & next)
        : next_(next),
          simplify_distance_(simplify_distance) {}

    void operator() (mapnik::geometry::geometry_empty &)
    {
        return;
    }

    void operator() (mapnik::geometry::point<std::int64_t> & geom)
    {
        next_(geom);
    }

    void operator() (mapnik::geometry::multi_point<std::int64_t> & geom)
    {
        next_(geom);
    }

    void operator() (mapnik::geometry::line_string<std::int64_t> & geom)
    {
        mapnik::geometry::line_string<std::int64_t> simplified;
        boost::geometry::simplify<
                mapnik::geometry::line_string<std::int64_t>,
                std::int64_t>
                (geom, simplified, simplify_distance_);
        next_(simplified);
    }

    void operator() (mapnik::geometry::multi_line_string<std::int64_t> & geom)
    {
        mapnik::geometry::multi_line_string<std::int64_t> simplified;
        boost::geometry::simplify<
                mapnik::geometry::multi_line_string<std::int64_t>,
                std::int64_t>
                (geom, simplified, simplify_distance_);
        next_(simplified);
    }

    void operator() (mapnik::geometry::polygon<std::int64_t> & geom)
    {
        mapnik::geometry::polygon<std::int64_t> simplified;
        boost::geometry::simplify<
                mapnik::geometry::polygon<std::int64_t>,
                std::int64_t>
                (geom, simplified, simplify_distance_);
        next_(simplified);
    }

    void operator() (mapnik::geometry::multi_polygon<std::int64_t> & geom)
    {
        mapnik::geometry::multi_polygon<std::int64_t> simplified;
        boost::geometry::simplify<
                mapnik::geometry::multi_polygon<std::int64_t>,
                std::int64_t>
                (geom, simplified, simplify_distance_);
        next_(simplified);
    }

    void operator() (mapnik::geometry::geometry_collection<std::int64_t> & geom)
    {
        for (auto & g : geom)
        {
            mapnik::util::apply_visitor((*this), g);
        }
    }
        
    NextProcessor & next_;
    unsigned simplify_distance_;
};

} // end ns vector_tile_impl

} // end ns mapnik

#endif // __MAPNIK_VECTOR_GEOMETRY_SIMPLIFIER_H__
