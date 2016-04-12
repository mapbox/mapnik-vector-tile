#ifndef __MAPNIK_VECTOR_TILE_GEOMETRY_SIMPLIFIER_H__
#define __MAPNIK_VECTOR_TILE_GEOMETRY_SIMPLIFIER_H__

// mapnik-vector-tile
#include "vector_tile_config.hpp"
#include "vector_tile_douglas_peucker.hpp"

// mapnik
#include <mapnik/geometry.hpp>

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
        if (geom.size() <= 2)
        {
            next_(geom);
        }
        else
        {
            mapnik::geometry::line_string<std::int64_t> simplified;
            douglas_peucker<std::int64_t>(geom, std::back_inserter(simplified), simplify_distance_);
            next_(simplified);
        }
    }

    void operator() (mapnik::geometry::multi_line_string<std::int64_t> & geom)
    {
        mapnik::geometry::multi_line_string<std::int64_t> simplified;
        for (auto const & g : geom)
        {
            if (g.size() <= 2)
            {
                simplified.push_back(g);
            }
            else
            {
                mapnik::geometry::line_string<std::int64_t> simplified_line;
                douglas_peucker<std::int64_t>(g, std::back_inserter(simplified_line), simplify_distance_);
                simplified.push_back(simplified_line);
            }
        }
        next_(simplified);
    }

    void operator() (mapnik::geometry::polygon<std::int64_t> & geom)
    {
        mapnik::geometry::polygon<std::int64_t> simplified;
        if (geom.exterior_ring.size() <= 4)
        {
            simplified.exterior_ring = geom.exterior_ring;
        }
        else
        {
            douglas_peucker<std::int64_t>(geom.exterior_ring, std::back_inserter(simplified.exterior_ring), simplify_distance_);
        }
        for (auto const & g : geom.interior_rings)
        {
            if (g.size() <= 4)
            {
                simplified.interior_rings.push_back(g);
            }
            else
            {
                mapnik::geometry::linear_ring<std::int64_t> simplified_ring;
                douglas_peucker<std::int64_t>(g, std::back_inserter(simplified_ring), simplify_distance_);
                simplified.interior_rings.push_back(simplified_ring);
            }
        }
        next_(simplified);
    }

    void operator() (mapnik::geometry::multi_polygon<std::int64_t> & multi_geom)
    {
        mapnik::geometry::multi_polygon<std::int64_t> simplified_multi;
        for (auto const & geom : multi_geom)
        {
            mapnik::geometry::polygon<std::int64_t> simplified;
            if (geom.exterior_ring.size() <= 4)
            {
                simplified.exterior_ring = geom.exterior_ring;
            }
            else
            {
                douglas_peucker<std::int64_t>(geom.exterior_ring, std::back_inserter(simplified.exterior_ring), simplify_distance_);
            }
            for (auto const & g : geom.interior_rings)
            {
                if (g.size() <= 4)
                {
                    simplified.interior_rings.push_back(g);
                }
                else
                {
                    mapnik::geometry::linear_ring<std::int64_t> simplified_ring;
                    douglas_peucker<std::int64_t>(g, std::back_inserter(simplified_ring), simplify_distance_);
                    simplified.interior_rings.push_back(simplified_ring);
                }
            }
            simplified_multi.push_back(simplified);
        }
        next_(simplified_multi);
    }

    void operator() (mapnik::geometry::geometry_collection<std::int64_t> & geom)
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
