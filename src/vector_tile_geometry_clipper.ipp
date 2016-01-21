// mapnik
#include <mapnik/box2d.hpp>
#include <mapnik/geometry.hpp>
#include <mapnik/geometry_adapters.hpp>

// angus clipper
// http://www.angusj.com/delphi/clipper.php
#include "clipper.hpp"

// libprotobuf
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include "vector_tile.pb.h"
#pragma GCC diagnostic pop

// boost
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wshadow"
#include <boost/geometry/algorithms/intersection.hpp>
#include <boost/geometry/algorithms/unique.hpp>
#pragma GCC diagnostic pop

namespace mapnik
{

namespace vector_tile_impl
{

namespace detail
{

ClipperLib::PolyFillType get_angus_fill_type(polygon_fill_type type)
{
    switch (type) 
    {
    case polygon_fill_type_max:
    case even_odd_fill:
        return ClipperLib::pftEvenOdd;
    case non_zero_fill: 
        return ClipperLib::pftNonZero;
    case positive_fill:
        return ClipperLib::pftPositive;
    case negative_fill:
        return ClipperLib::pftNegative;
    }
}

inline void process_polynode_branch(ClipperLib::PolyNode* polynode, 
                                    mapnik::geometry::multi_polygon<std::int64_t> & mp,
                                    double area_threshold)
{
    mapnik::geometry::polygon<std::int64_t> polygon;
    polygon.set_exterior_ring(std::move(polynode->Contour));
    if (polygon.exterior_ring.size() > 2) // Throw out invalid polygons
    {
        double outer_area = ClipperLib::Area(polygon.exterior_ring);
        if (std::abs(outer_area) >= area_threshold)
        {
            // The view transform inverts the y axis so this should be positive still despite now
            // being clockwise for the exterior ring. If it is not lets invert it.
            if (outer_area < 0)
            {   
                std::reverse(polygon.exterior_ring.begin(), polygon.exterior_ring.end());
            }
            
            // children of exterior ring are always interior rings
            for (auto * ring : polynode->Childs)
            {
                if (ring->Contour.size() < 3)
                {
                    continue; // Throw out invalid holes
                }
                double inner_area = ClipperLib::Area(ring->Contour);
                if (std::abs(inner_area) < area_threshold)
                {
                    continue;
                }
                
                if (inner_area > 0)
                {
                    std::reverse(ring->Contour.begin(), ring->Contour.end());
                }
                polygon.add_hole(std::move(ring->Contour));
            }
            mp.emplace_back(std::move(polygon));
        }
    }
    for (auto * ring : polynode->Childs)
    {
        for (auto * sub_ring : ring->Childs)
        {
            process_polynode_branch(sub_ring, mp, area_threshold);
        }
    }
}

} // end ns detail

geometry_clipper::geometry_clipper(mapnik::geometry::geometry<std::int64_t> & geom,
                                   mapnik::box2d<int> const& tile_clipping_extent,
                                   double area_threshold,
                                   bool strictly_simple,
                                   bool multi_polygon_union,
                                   polygon_fill_type fill_type,
                                   bool process_all_rings) :
          geom_(geom),
          tile_clipping_extent_(tile_clipping_extent),
          area_threshold_(area_threshold),
          strictly_simple_(strictly_simple),
          multi_polygon_union_(multi_polygon_union),
          fill_type_(fill_type),
          process_all_rings_(process_all_rings)
{
}

MAPNIK_VECTOR_INLINE void geometry_clipper::operator() (mapnik::geometry::geometry_empty &)
{
}

MAPNIK_VECTOR_INLINE void geometry_clipper::operator() (mapnik::geometry::point<std::int64_t> &)
{
}

MAPNIK_VECTOR_INLINE void geometry_clipper::operator() (mapnik::geometry::multi_point<std::int64_t> &)
{
}

MAPNIK_VECTOR_INLINE void geometry_clipper::operator() (mapnik::geometry::geometry_collection<std::int64_t> & geom)
{
    for (auto & g : geom)
    {
        geometry_clipper clipper(g,
                                 tile_clipping_extent_,
                                 area_threshold_,
                                 strictly_simple_,
                                 multi_polygon_union_,
                                 fill_type_,
                                 process_all_rings_);
                           
        mapnik::util::apply_visitor(clipper, g);
    }
}

MAPNIK_VECTOR_INLINE void geometry_clipper::operator() (mapnik::geometry::line_string<std::int64_t> & geom)
{
    boost::geometry::unique(geom);
    if (geom.size() < 2)
    {
        geom_ = mapnik::geometry::geometry_empty();
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
        geom_ = mapnik::geometry::geometry_empty();
        return;
    }
    geom_ = std::move(result);
}

MAPNIK_VECTOR_INLINE void geometry_clipper::operator() (mapnik::geometry::multi_line_string<std::int64_t> & geom)
{
    if (geom.empty())
    {
        geom_ = mapnik::geometry::geometry_empty();
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
        geom_ = mapnik::geometry::geometry_empty();
        return;
    }
    geom_ = std::move(results);
}

MAPNIK_VECTOR_INLINE void geometry_clipper::operator() (mapnik::geometry::polygon<std::int64_t> & geom)
{
    if ((geom.exterior_ring.size() < 3) && !process_all_rings_)
    {
        geom_ = mapnik::geometry::geometry_empty();
        return;
    }
    
    double clean_distance = 1.415;
    
    // Prepare the clipper object
    ClipperLib::Clipper clipper;
    
    if (strictly_simple_) 
    {
        clipper.StrictlySimple(true);
    }
    
    // Start processing on exterior ring
    // if proces_all_rings is true even if the exterior
    // ring is invalid we will continue to insert all polygon
    // rings into the clipper
    ClipperLib::CleanPolygon(geom.exterior_ring, clean_distance);
    double outer_area = ClipperLib::Area(geom.exterior_ring);
    if ((std::abs(outer_area) < area_threshold_)  && !process_all_rings_)
    {
        geom_ = mapnik::geometry::geometry_empty();
        return;
    }

    // The view transform inverts the y axis so this should be positive still despite now
    // being clockwise for the exterior ring. If it is not lets invert it.
    if (outer_area < 0)
    {   
        std::reverse(geom.exterior_ring.begin(), geom.exterior_ring.end());
    }

    if (!clipper.AddPath(geom.exterior_ring, ClipperLib::ptSubject, true) && !process_all_rings_)
    {
        geom_ = mapnik::geometry::geometry_empty();
        return;
    }

    for (auto & ring : geom.interior_rings)
    {
        if (ring.size() < 3) 
        {
            continue;
        }
        ClipperLib::CleanPolygon(ring, clean_distance);
        double inner_area = ClipperLib::Area(ring);
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
        if (!clipper.AddPath(ring, ClipperLib::ptSubject, true))
        {
            continue;
        }
    }

    // Setup the box for clipping
    mapnik::geometry::linear_ring<std::int64_t> clip_box;
    clip_box.reserve(5);
    clip_box.emplace_back(tile_clipping_extent_.minx(), tile_clipping_extent_.miny());
    clip_box.emplace_back(tile_clipping_extent_.maxx(), tile_clipping_extent_.miny());
    clip_box.emplace_back(tile_clipping_extent_.maxx(), tile_clipping_extent_.maxy());
    clip_box.emplace_back(tile_clipping_extent_.minx(), tile_clipping_extent_.maxy());
    clip_box.emplace_back(tile_clipping_extent_.minx(), tile_clipping_extent_.miny());
    
    // Finally add the box we will be using for clipping
    if (!clipper.AddPath( clip_box, ClipperLib::ptClip, true ))
    {
        geom_ = mapnik::geometry::geometry_empty();
        return;
    }

    ClipperLib::PolyTree polygons;
    ClipperLib::PolyFillType fill_type = detail::get_angus_fill_type(fill_type_);
    clipper.Execute(ClipperLib::ctIntersection, polygons, fill_type, fill_type);
    clipper.Clear();
    
    mapnik::geometry::multi_polygon<std::int64_t> mp;
    
    for (auto * polynode : polygons.Childs)
    {
        detail::process_polynode_branch(polynode, mp, area_threshold_); 
    }

    if (mp.empty())
    {
        geom_ = mapnik::geometry::geometry_empty();
        return;
    }
    geom_ = std::move(mp);
}

MAPNIK_VECTOR_INLINE void geometry_clipper::operator() (mapnik::geometry::multi_polygon<std::int64_t> & geom)
{
    if (geom.empty())
    {
        geom_ = mapnik::geometry::geometry_empty();
        return;
    }
    
    double clean_distance = 1.415;
    mapnik::geometry::linear_ring<std::int64_t> clip_box;
    clip_box.reserve(5);
    clip_box.emplace_back(tile_clipping_extent_.minx(),tile_clipping_extent_.miny());
    clip_box.emplace_back(tile_clipping_extent_.maxx(),tile_clipping_extent_.miny());
    clip_box.emplace_back(tile_clipping_extent_.maxx(),tile_clipping_extent_.maxy());
    clip_box.emplace_back(tile_clipping_extent_.minx(),tile_clipping_extent_.maxy());
    clip_box.emplace_back(tile_clipping_extent_.minx(),tile_clipping_extent_.miny());
    
    mapnik::geometry::multi_polygon<std::int64_t> mp;
    
    ClipperLib::Clipper clipper;
    
    if (strictly_simple_) 
    {
        clipper.StrictlySimple(true);
    }

    ClipperLib::PolyFillType fill_type = detail::get_angus_fill_type(fill_type_);

    if (multi_polygon_union_)
    {
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
            ClipperLib::CleanPolygon(poly.exterior_ring, clean_distance);
            double outer_area = ClipperLib::Area(poly.exterior_ring);
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
            if (!clipper.AddPath(poly.exterior_ring, ClipperLib::ptSubject, true) && !process_all_rings_)
            {
                continue;
            }

            for (auto & ring : poly.interior_rings)
            {
                if (ring.size() < 3)
                {
                    continue;
                }
                ClipperLib::CleanPolygon(ring, clean_distance);
                double inner_area = ClipperLib::Area(ring);
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
                clipper.AddPath(ring, ClipperLib::ptSubject, true);
            }
        }
        if (!clipper.AddPath( clip_box, ClipperLib::ptClip, true ))
        {
            geom_ = mapnik::geometry::geometry_empty();
            return;
        }
        ClipperLib::PolyTree polygons;
        clipper.Execute(ClipperLib::ctIntersection, polygons, fill_type, fill_type);
        clipper.Clear();
        
        for (auto * polynode : polygons.Childs)
        {
            detail::process_polynode_branch(polynode, mp, area_threshold_); 
        }
    }
    else
    {
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
            ClipperLib::CleanPolygon(poly.exterior_ring, clean_distance);
            double outer_area = ClipperLib::Area(poly.exterior_ring);
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
            if (!clipper.AddPath(poly.exterior_ring, ClipperLib::ptSubject, true) && !process_all_rings_)
            {
                continue;
            }
            for (auto & ring : poly.interior_rings)
            {
                if (ring.size() < 3)
                {
                    continue;
                }
                ClipperLib::CleanPolygon(ring, clean_distance);
                double inner_area = ClipperLib::Area(ring);
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
                if (!clipper.AddPath(ring, ClipperLib::ptSubject, true))
                {
                    continue;
                }
            }
            if (!clipper.AddPath( clip_box, ClipperLib::ptClip, true ))
            {
                geom_ = mapnik::geometry::geometry_empty();
                return;
            }
            ClipperLib::PolyTree polygons;
            clipper.Execute(ClipperLib::ctIntersection, polygons, fill_type, fill_type);
            clipper.Clear();
            
            for (auto * polynode : polygons.Childs)
            {
                detail::process_polynode_branch(polynode, mp, area_threshold_); 
            }
        }
    }

    if (mp.empty())
    {
        geom_ = mapnik::geometry::geometry_empty();
        return;
    }
    geom_ = std::move(mp);
}

} // end ns vector_tile_impl

} // end ns mapnik
