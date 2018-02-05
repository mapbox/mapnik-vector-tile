#ifndef __MAPNIK_VECTOR_TILE_GEOMETRY_SIMPLIFIER_H__
#define __MAPNIK_VECTOR_TILE_GEOMETRY_SIMPLIFIER_H__

// mapnik-vector-tile
#include "vector_tile_config.hpp"
#include "vector_tile_douglas_peucker.hpp"

// mapbox
#include <mapbox/geometry/geometry.hpp>

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

    void operator() (mapbox::geometry::point<std::int64_t> & geom)
    {
        next_(geom);
    }

    void operator() (mapbox::geometry::multi_point<std::int64_t> & geom)
    {
        next_(geom);
    }

    void operator() (mapbox::geometry::line_string<std::int64_t> & geom)
    {
        if (geom.size() <= 2)
        {
            next_(geom);
        }
        else
        {
            mapbox::geometry::line_string<std::int64_t> simplified;
            douglas_peucker(geom, std::back_inserter(simplified), simplify_distance_);
            next_(simplified);
        }
    }

    void operator() (mapbox::geometry::multi_line_string<std::int64_t> & geom)
    {
        mapbox::geometry::multi_line_string<std::int64_t> simplified;
        for (auto const & g : geom)
        {
            if (g.size() <= 2)
            {
                simplified.push_back(g);
            }
            else
            {
                mapbox::geometry::line_string<std::int64_t> simplified_line;
                douglas_peucker(g, std::back_inserter(simplified_line), simplify_distance_);
                simplified.push_back(simplified_line);
            }
        }
        next_(simplified);
    }

    void operator() (mapbox::geometry::polygon<std::int64_t> & geom)
    {
        mapbox::geometry::polygon<std::int64_t> simplified;
        for (auto const & g : geom)
        {
            if (g.size() <= 4)
            {
                simplified.push_back(g);
            }
            else
            {
                mapbox::geometry::linear_ring<std::int64_t> simplified_ring;
                douglas_peucker(g, std::back_inserter(simplified_ring), simplify_distance_);
                simplified.push_back(simplified_ring);
            }
        }
        next_(simplified);
    }

    void operator() (mapbox::geometry::multi_polygon<std::int64_t> & multi_geom)
    {
        mapbox::geometry::multi_polygon<std::int64_t> simplified_multi;
        for (auto const & geom : multi_geom)
        {
            mapbox::geometry::polygon<std::int64_t> simplified;
            for (auto const & g : geom)
            {
                if (g.size() <= 4)
                {
                    simplified.push_back(g);
                }
                else
                {
                    mapbox::geometry::linear_ring<std::int64_t> simplified_ring;
                    douglas_peucker(g, std::back_inserter(simplified_ring), simplify_distance_);
                    simplified.push_back(simplified_ring);
                }
            }
            simplified_multi.push_back(simplified);
        }
        next_(simplified_multi);
    }

    void operator() (mapbox::geometry::geometry_collection<std::int64_t> & geom)
    {
        for (auto & g : geom)
        {
            mapnik::util::apply_visitor((*this), g);
        }
    }
        
    NextProcessor & next_;
    double simplify_distance_;
};

} // end ns vector_tile_impl

} // end ns mapnik

#endif // __MAPNIK_VECTOR_GEOMETRY_SIMPLIFIER_H__
