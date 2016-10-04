#ifndef __MAPNIK_VECTOR_TILE_GEOMETRY_CLIPPER_H__
#define __MAPNIK_VECTOR_TILE_GEOMETRY_CLIPPER_H__

// mapnik-vector-tile
#include "vector_tile_config.hpp"
#include "vector_tile_processor.hpp"
#include "convert_geometry_types.hpp"

// mapnik
#include <mapnik/box2d.hpp>
#include <mapnik/geometry.hpp>

// mapbox
#include <mapbox/geometry/wagyu/wagyu.hpp>

// boost
#pragma GCC diagnostic push
#include <mapnik/warning_ignore.hpp>
#include <boost/geometry/algorithms/intersection.hpp>
#include <boost/geometry/algorithms/unique.hpp>
#pragma GCC diagnostic pop

namespace mapnik
{

namespace vector_tile_impl
{

namespace detail
{

template <typename T>
double area(mapnik::geometry::linear_ring<T> const& poly) {
    std::size_t size = poly.size();
    if (size < 3) {
        return 0.0;
    }

    double a = 0.0;
    auto itr = poly.begin();
    auto itr_prev = poly.end();
    --itr_prev;
    a += static_cast<double>(itr_prev->x + itr->x) * static_cast<double>(itr_prev->y - itr->y);
    ++itr;
    itr_prev = poly.begin();
    for (; itr != poly.end(); ++itr, ++itr_prev) {
        a += static_cast<double>(itr_prev->x + itr->x) * static_cast<double>(itr_prev->y - itr->y);
    }
    return -a * 0.5;
}

inline mapbox::geometry::wagyu::fill_type get_wagyu_fill_type(polygon_fill_type type)
{
    switch (type) 
    {
    case polygon_fill_type_max:
    case even_odd_fill:
        return mapbox::geometry::wagyu::fill_type_even_odd;
    case non_zero_fill: 
        return mapbox::geometry::wagyu::fill_type_non_zero;
    case positive_fill:
        return mapbox::geometry::wagyu::fill_type_positive;
    case negative_fill:
        return mapbox::geometry::wagyu::fill_type_negative;
    }
}

} // end ns detail

template <typename NextProcessor>
class geometry_clipper 
{
private:
    NextProcessor & next_;
    mapnik::box2d<int> const& tile_clipping_extent_;
    double area_threshold_;
    bool strictly_simple_;
    bool multi_polygon_union_;
    polygon_fill_type fill_type_;
    bool process_all_rings_;
public:
    geometry_clipper(mapnik::box2d<int> const& tile_clipping_extent,
                     double area_threshold,
                     bool strictly_simple,
                     bool multi_polygon_union,
                     polygon_fill_type fill_type,
                     bool process_all_rings,
                     NextProcessor & next) :
              next_(next),
              tile_clipping_extent_(tile_clipping_extent),
              area_threshold_(area_threshold),
              strictly_simple_(strictly_simple),
              multi_polygon_union_(multi_polygon_union),
              fill_type_(fill_type),
              process_all_rings_(process_all_rings)
    {
    }

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
        // Here we remove repeated points from multi_point
        auto last = std::unique(geom.begin(), geom.end());
        geom.erase(last, geom.end());
        next_(geom);
    }

    void operator() (mapnik::geometry::geometry_collection<std::int64_t> & geom)
    {
        for (auto & g : geom)
        {
            mapnik::util::apply_visitor((*this), g);
        }
    }

    void operator() (mapnik::geometry::line_string<std::int64_t> & geom)
    {
        boost::geometry::unique(geom);
        if (geom.size() < 2)
        {
            return;
        }
        //std::deque<mapnik::geometry::line_string<int64_t>> result;
        mapnik::geometry::multi_line_string<int64_t> result;
        mapnik::geometry::linear_ring<std::int64_t> clip_box;
        clip_box.reserve(5);
        clip_box.emplace_back(tile_clipping_extent_.minx(),tile_clipping_extent_.miny());
        clip_box.emplace_back(tile_clipping_extent_.maxx(),tile_clipping_extent_.miny());
        clip_box.emplace_back(tile_clipping_extent_.maxx(),tile_clipping_extent_.maxy());
        clip_box.emplace_back(tile_clipping_extent_.minx(),tile_clipping_extent_.maxy());
        clip_box.emplace_back(tile_clipping_extent_.minx(),tile_clipping_extent_.miny());
        boost::geometry::intersection(clip_box, geom, result);
        if (result.empty())
        {
            return;
        }
        next_(result);
    }

    void operator() (mapnik::geometry::multi_line_string<std::int64_t> & geom)
    {
        if (geom.empty())
        {
            return;
        }

        mapnik::geometry::linear_ring<std::int64_t> clip_box;
        clip_box.reserve(5);
        clip_box.emplace_back(tile_clipping_extent_.minx(),tile_clipping_extent_.miny());
        clip_box.emplace_back(tile_clipping_extent_.maxx(),tile_clipping_extent_.miny());
        clip_box.emplace_back(tile_clipping_extent_.maxx(),tile_clipping_extent_.maxy());
        clip_box.emplace_back(tile_clipping_extent_.minx(),tile_clipping_extent_.maxy());
        clip_box.emplace_back(tile_clipping_extent_.minx(),tile_clipping_extent_.miny());
        boost::geometry::unique(geom);
        mapnik::geometry::multi_line_string<int64_t> results;
        for (auto const& line : geom)
        {
            if (line.size() < 2)
            {
               continue;
            }
            boost::geometry::intersection(clip_box, line, results);
        }
        if (results.empty())
        {
            return;
        }
        next_(results);
    }

    void operator() (mapnik::geometry::polygon<std::int64_t> & geom)
    {
        if ((geom.exterior_ring.size() < 3) && !process_all_rings_)
        {
            return;
        }
        
        mapbox::geometry::wagyu::wagyu<std::int64_t> clipper;
        
        // Start processing on exterior ring
        // if proces_all_rings is true even if the exterior
        // ring is invalid we will continue to insert all polygon
        // rings into the clipper
        double outer_area = detail::area(geom.exterior_ring);
        if ((std::abs(outer_area) < area_threshold_)  && !process_all_rings_)
        {
            return;
        }

        // The view transform inverts the y axis so this should be positive still despite now
        // being clockwise for the exterior ring. If it is not lets invert it.
        if (outer_area < 0)
        {   
            std::reverse(geom.exterior_ring.begin(), geom.exterior_ring.end());
        }

        if (!clipper.add_ring(mapnik_to_mapbox(geom.exterior_ring)) && !process_all_rings_)
        {
            return;
        }

        for (auto & ring : geom.interior_rings)
        {
            if (ring.size() < 3) 
            {
                continue;
            }
            double inner_area = detail::area(ring);
            if (std::abs(inner_area) < area_threshold_)
            {
                continue;
            }
            // This should be a negative area, the y axis is down, so the ring will be "CCW" rather
            // then "CW" after the view transform, but if it is not lets reverse it
            if (inner_area > 0)
            {
                std::reverse(ring.begin(), ring.end());
            }
            if (!clipper.add_ring(mapnik_to_mapbox(ring)))
            {
                continue;
            }
        }

        // Setup the box for clipping
        mapbox::geometry::linear_ring<std::int64_t> clip_box;
        clip_box.reserve(5);
        clip_box.emplace_back(tile_clipping_extent_.minx(), tile_clipping_extent_.miny());
        clip_box.emplace_back(tile_clipping_extent_.maxx(), tile_clipping_extent_.miny());
        clip_box.emplace_back(tile_clipping_extent_.maxx(), tile_clipping_extent_.maxy());
        clip_box.emplace_back(tile_clipping_extent_.minx(), tile_clipping_extent_.maxy());
        clip_box.emplace_back(tile_clipping_extent_.minx(), tile_clipping_extent_.miny());
        
        // Finally add the box we will be using for clipping
        if (!clipper.add_ring(clip_box, mapbox::geometry::wagyu::polygon_type_clip))
        {
            return;
        }

        mapbox::geometry::multi_polygon<std::int64_t> output;

        clipper.execute(mapbox::geometry::wagyu::clip_type_intersection, 
                        output, 
                        detail::get_wagyu_fill_type(fill_type_), 
                        mapbox::geometry::wagyu::fill_type_even_odd);
        
        mapnik::geometry::multi_polygon<std::int64_t> mp = mapbox_to_mapnik(output);

        if (mp.empty())
        {
            return;
        }
        next_(mp);
    }

    void operator() (mapnik::geometry::multi_polygon<std::int64_t> & geom)
    {
        if (geom.empty())
        {
            return;
        }
        
        mapbox::geometry::linear_ring<std::int64_t> clip_box;
        clip_box.reserve(5);
        clip_box.emplace_back(tile_clipping_extent_.minx(),tile_clipping_extent_.miny());
        clip_box.emplace_back(tile_clipping_extent_.maxx(),tile_clipping_extent_.miny());
        clip_box.emplace_back(tile_clipping_extent_.maxx(),tile_clipping_extent_.maxy());
        clip_box.emplace_back(tile_clipping_extent_.minx(),tile_clipping_extent_.maxy());
        clip_box.emplace_back(tile_clipping_extent_.minx(),tile_clipping_extent_.miny());
        
        mapnik::geometry::multi_polygon<std::int64_t> mp;
        
        if (multi_polygon_union_)
        {
            mapbox::geometry::wagyu::wagyu<std::int64_t> clipper;
            for (auto & poly : geom)
            {
                // Below we attempt to skip processing of all interior rings if the exterior
                // ring fails a variety of sanity checks for size and validity for AddPath
                // When `process_all_rings_=true` this optimization is disabled. This is needed when
                // the ring order of input polygons is potentially incorrect and where the
                // "exterior_ring" might actually be an incorrectly classified exterior ring.
                if (poly.exterior_ring.size() < 3 && !process_all_rings_)
                {
                    continue;
                }
                double outer_area = detail::area(poly.exterior_ring);
                if ((std::abs(outer_area) < area_threshold_) && !process_all_rings_)
                {
                    continue;
                }
                // The view transform inverts the y axis so this should be positive still despite now
                // being clockwise for the exterior ring. If it is not lets invert it.
                if (outer_area < 0)
                {
                    std::reverse(poly.exterior_ring.begin(), poly.exterior_ring.end());
                }
                if (!clipper.add_ring(mapnik_to_mapbox(poly.exterior_ring)) && !process_all_rings_)
                {
                    continue;
                }

                for (auto & ring : poly.interior_rings)
                {
                    if (ring.size() < 3)
                    {
                        continue;
                    }
                    double inner_area = detail::area(ring);
                    if (std::abs(inner_area) < area_threshold_)
                    {
                        continue;
                    }
                    // This should be a negative area, the y axis is down, so the ring will be "CCW" rather
                    // then "CW" after the view transform, but if it is not lets reverse it
                    if (inner_area > 0)
                    {
                        std::reverse(ring.begin(), ring.end());
                    }
                    clipper.add_ring(mapnik_to_mapbox(ring));
                }
            }
            if (!clipper.add_ring(clip_box, mapbox::geometry::wagyu::polygon_type_clip))
            {
                return;
            }
            mapbox::geometry::multi_polygon<std::int64_t> output;

            clipper.execute(mapbox::geometry::wagyu::clip_type_intersection, 
                            output, 
                            detail::get_wagyu_fill_type(fill_type_), 
                            mapbox::geometry::wagyu::fill_type_even_odd);
            
            mapnik::geometry::multi_polygon<std::int64_t> temp_mp = mapbox_to_mapnik(output);
            mp.insert(mp.end(), temp_mp.begin(), temp_mp.end());
        }
        else
        {
            for (auto & poly : geom)
            {
                mapbox::geometry::wagyu::wagyu<std::int64_t> clipper;
                // Below we attempt to skip processing of all interior rings if the exterior
                // ring fails a variety of sanity checks for size and validity for AddPath
                // When `process_all_rings_=true` this optimization is disabled. This is needed when
                // the ring order of input polygons is potentially incorrect and where the
                // "exterior_ring" might actually be an incorrectly classified exterior ring.
                if (poly.exterior_ring.size() < 3 && !process_all_rings_)
                {
                    continue;
                }
                double outer_area = detail::area(poly.exterior_ring);
                if ((std::abs(outer_area) < area_threshold_) && !process_all_rings_)
                {
                    continue;
                }
                // The view transform inverts the y axis so this should be positive still despite now
                // being clockwise for the exterior ring. If it is not lets invert it.
                if (outer_area < 0)
                {
                    std::reverse(poly.exterior_ring.begin(), poly.exterior_ring.end());
                }
                if (!clipper.add_ring(mapnik_to_mapbox(poly.exterior_ring)) && !process_all_rings_)
                {
                    continue;
                }
                for (auto & ring : poly.interior_rings)
                {
                    if (ring.size() < 3)
                    {
                        continue;
                    }
                    double inner_area = detail::area(ring);
                    if (std::abs(inner_area) < area_threshold_)
                    {
                        continue;
                    }
                    // This should be a negative area, the y axis is down, so the ring will be "CCW" rather
                    // then "CW" after the view transform, but if it is not lets reverse it
                    if (inner_area > 0)
                    {
                        std::reverse(ring.begin(), ring.end());
                    }
                    if (!clipper.add_ring(mapnik_to_mapbox(ring)))
                    {
                        continue;
                    }
                }
                if (!clipper.add_ring(clip_box, mapbox::geometry::wagyu::polygon_type_clip))
                {
                    continue;
                }
                mapbox::geometry::multi_polygon<std::int64_t> output;

                clipper.execute(mapbox::geometry::wagyu::clip_type_intersection, 
                                output, 
                                detail::get_wagyu_fill_type(fill_type_), 
                                mapbox::geometry::wagyu::fill_type_even_odd);
                
                mapnik::geometry::multi_polygon<std::int64_t> temp_mp = mapbox_to_mapnik(output);
                mp.insert(mp.end(), temp_mp.begin(), temp_mp.end());
            }
        }

        if (mp.empty())
        {
            return;
        }
        next_(mp);
    }
};

} // end ns vector_tile_impl

} // end ns mapnik

#endif // __MAPNIK_VECTOR_GEOMETRY_CLIPPER_H__
