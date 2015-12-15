// mapnik
#include <mapnik/box2d.hpp>
#include <mapnik/feature.hpp>
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

template <typename T>
geometry_clipper<T>::geometry_clipper(backend_type & backend,
                                      mapnik::feature_impl const& feature,
                                      mapnik::box2d<int> const& tile_clipping_extent,
                                      double area_threshold,
                                      bool strictly_simple,
                                      bool multi_polygon_union,
                                      ClipperLib::PolyFillType fill_type,
                                      bool process_all_rings) :
          backend_(backend),
          feature_(feature),
          tile_clipping_extent_(tile_clipping_extent),
          area_threshold_(area_threshold),
          strictly_simple_(strictly_simple),
          multi_polygon_union_(multi_polygon_union),
          fill_type_(fill_type),
          process_all_rings_(process_all_rings)
{
}

template <typename T>
bool geometry_clipper<T>::operator() (mapnik::geometry::geometry_collection<std::int64_t> & geom)
{
    bool painted = false;
    for (auto & g : geom)
    {
        if (mapnik::util::apply_visitor((*this), g))
        {
            painted = true;
        }
    }
    return painted;
}

template <typename T>
bool geometry_clipper<T>::operator() (mapnik::geometry::point<std::int64_t> const& geom)
{
    backend_.start_tile_feature(feature_);
    bool painted = backend_.add_path(geom);
    backend_.stop_tile_feature();
    return painted;
}

template <typename T>
bool geometry_clipper<T>::operator() (mapnik::geometry::multi_point<std::int64_t> const& geom)
{
    bool painted = false;
    if (!geom.empty())
    {
        backend_.start_tile_feature(feature_);
        painted = backend_.add_path(geom);
        backend_.stop_tile_feature();
    }
    return painted;
}

template <typename T>
bool geometry_clipper<T>::operator() (mapnik::geometry::line_string<std::int64_t> & geom)
{
    bool painted = false;
    boost::geometry::unique(geom);
    if (geom.size() < 2)
    {
        // This is false because it means the original data was invalid
        return false;
    }
    std::deque<mapnik::geometry::line_string<int64_t>> result;
    mapnik::geometry::linear_ring<std::int64_t> clip_box;
    clip_box.emplace_back(tile_clipping_extent_.minx(),tile_clipping_extent_.miny());
    clip_box.emplace_back(tile_clipping_extent_.maxx(),tile_clipping_extent_.miny());
    clip_box.emplace_back(tile_clipping_extent_.maxx(),tile_clipping_extent_.maxy());
    clip_box.emplace_back(tile_clipping_extent_.minx(),tile_clipping_extent_.maxy());
    clip_box.emplace_back(tile_clipping_extent_.minx(),tile_clipping_extent_.miny());
    boost::geometry::intersection(clip_box,geom,result);
    if (!result.empty())
    {
        // Found some data in tile so painted is now true
        painted = true;
        // Add the data to the tile
        backend_.start_tile_feature(feature_);
        for (auto const& ls : result)
        {
            backend_.add_path(ls);
        }
        backend_.stop_tile_feature();
    }
    return painted;
}

template <typename T>
bool geometry_clipper<T>::operator() (mapnik::geometry::multi_line_string<std::int64_t> & geom)
{
    bool painted = false;
    mapnik::geometry::linear_ring<std::int64_t> clip_box;
    clip_box.emplace_back(tile_clipping_extent_.minx(),tile_clipping_extent_.miny());
    clip_box.emplace_back(tile_clipping_extent_.maxx(),tile_clipping_extent_.miny());
    clip_box.emplace_back(tile_clipping_extent_.maxx(),tile_clipping_extent_.maxy());
    clip_box.emplace_back(tile_clipping_extent_.minx(),tile_clipping_extent_.maxy());
    clip_box.emplace_back(tile_clipping_extent_.minx(),tile_clipping_extent_.miny());
    bool first = true;
    boost::geometry::unique(geom);
    for (auto const& line : geom)
    {
        if (line.size() < 2)
        {
           continue;
        }
        // If any line reaches here painted is now true because
        std::deque<mapnik::geometry::line_string<int64_t>> result;
        boost::geometry::intersection(clip_box,line,result);
        if (!result.empty())
        {
            if (first)
            {
                painted = true;
                first = false;
                backend_.start_tile_feature(feature_);
            }
            for (auto const& ls : result)
            {
                backend_.add_path(ls);
            }
        }
    }
    if (!first)
    {
        backend_.stop_tile_feature();
    }
    return painted;
}

template <typename T>
bool geometry_clipper<T>::operator() (mapnik::geometry::polygon<std::int64_t> & geom)
{
    bool painted = false;
    if ((geom.exterior_ring.size() < 3) && !process_all_rings_)
    {
        // Invalid geometry so will be false
        return false;
    }
    // Because of geometry cleaning and other methods
    // we automatically call this tile painted even if there is no intersections.
    painted = true;
    double clean_distance = 1.415;
    mapnik::geometry::linear_ring<std::int64_t> clip_box;
    clip_box.emplace_back(tile_clipping_extent_.minx(),tile_clipping_extent_.miny());
    clip_box.emplace_back(tile_clipping_extent_.maxx(),tile_clipping_extent_.miny());
    clip_box.emplace_back(tile_clipping_extent_.maxx(),tile_clipping_extent_.maxy());
    clip_box.emplace_back(tile_clipping_extent_.minx(),tile_clipping_extent_.maxy());
    clip_box.emplace_back(tile_clipping_extent_.minx(),tile_clipping_extent_.miny());
    ClipperLib::Clipper clipper;
    ClipperLib::CleanPolygon(geom.exterior_ring, clean_distance);
    double outer_area = ClipperLib::Area(geom.exterior_ring);
    if ((std::abs(outer_area) < area_threshold_)  && !process_all_rings_)
    {
        return painted;
    }
    // The view transform inverts the y axis so this should be positive still despite now
    // being clockwise for the exterior ring. If it is not lets invert it.
    if (outer_area < 0)
    {   
        std::reverse(geom.exterior_ring.begin(), geom.exterior_ring.end());
    }
    ClipperLib::Clipper poly_clipper;
    
    if (strictly_simple_) 
    {
        poly_clipper.StrictlySimple(true);
    }
    if (!poly_clipper.AddPath(geom.exterior_ring, ClipperLib::ptSubject, true) && !process_all_rings_)
    {
        return painted;
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
        if (!poly_clipper.AddPath(ring, ClipperLib::ptSubject, true))
        {
            continue;
        }
    }
    if (!poly_clipper.AddPath( clip_box, ClipperLib::ptClip, true ))
    {
        return painted;
    }
    ClipperLib::PolyTree polygons;
    poly_clipper.Execute(ClipperLib::ctIntersection, polygons, fill_type_, fill_type_);
    poly_clipper.Clear();
    
    mapnik::geometry::multi_polygon<std::int64_t> mp;
    
    for (auto * polynode : polygons.Childs)
    {
        detail::process_polynode_branch(polynode, mp, area_threshold_); 
    }

    if (mp.empty())
    {
        return painted;
    }

    backend_.start_tile_feature(feature_);
    
    for (auto const& poly : mp)
    {
        backend_.add_path(poly);
    }
    backend_.stop_tile_feature();
    return painted;
}

template <typename T>
bool geometry_clipper<T>::operator() (mapnik::geometry::multi_polygon<std::int64_t> & geom)
{
    bool painted = false;
    //mapnik::box2d<std::int64_t> bbox = mapnik::geometry::envelope(geom);
    if (geom.empty())
    {
        return painted;
    }
    // From this point on due to polygon cleaning etc, we just assume that the tile has some sort
    // of intersection and is painted.
    painted = true;   
    double clean_distance = 1.415;
    mapnik::geometry::linear_ring<std::int64_t> clip_box;
    clip_box.emplace_back(tile_clipping_extent_.minx(),tile_clipping_extent_.miny());
    clip_box.emplace_back(tile_clipping_extent_.maxx(),tile_clipping_extent_.miny());
    clip_box.emplace_back(tile_clipping_extent_.maxx(),tile_clipping_extent_.maxy());
    clip_box.emplace_back(tile_clipping_extent_.minx(),tile_clipping_extent_.maxy());
    clip_box.emplace_back(tile_clipping_extent_.minx(),tile_clipping_extent_.miny());
    mapnik::geometry::multi_polygon<std::int64_t> mp;
    
    ClipperLib::Clipper clipper;
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
            return painted;
        }
        ClipperLib::PolyTree polygons;
        if (strictly_simple_) 
        {
            clipper.StrictlySimple(true);
        }
        clipper.Execute(ClipperLib::ctIntersection, polygons, fill_type_, fill_type_);
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
                return painted;
            }
            ClipperLib::PolyTree polygons;
            if (strictly_simple_) 
            {
                clipper.StrictlySimple(true);
            }
            clipper.Execute(ClipperLib::ctIntersection, polygons, fill_type_, fill_type_);
            clipper.Clear();
            
            for (auto * polynode : polygons.Childs)
            {
                detail::process_polynode_branch(polynode, mp, area_threshold_); 
            }
        }
    }

    if (mp.empty())
    {
        return painted;
    }

    backend_.start_tile_feature(feature_);
    
    for (auto const& poly : mp)
    {
        backend_.add_path(poly);
    }
    backend_.stop_tile_feature();
    return painted;
}

} // end ns vector_tile_impl

} // end ns mapnik
