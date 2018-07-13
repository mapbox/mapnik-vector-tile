#pragma once

// mapnik-vector-tile
#include "vector_tile_config.hpp"

// mapbox
#include <mapbox/geometry/geometry.hpp>
#include <mapbox/geometry/wagyu/quick_clip.hpp>
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
double area(mapbox::geometry::linear_ring<T> const& poly) {
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
    // Added return here to make gcc happy
    return mapbox::geometry::wagyu::fill_type_even_odd;
}

template <typename T>
mapbox::geometry::linear_ring<T> box_to_ring(mapbox::geometry::box<T> const& box)
{
    mapbox::geometry::linear_ring<T> ring;
    ring.reserve(5);
    ring.emplace_back(box.min.x, box.min.y);
    ring.emplace_back(box.max.x, box.min.y);
    ring.emplace_back(box.max.x, box.max.y);
    ring.emplace_back(box.min.x, box.max.y);
    ring.emplace_back(box.min.x, box.min.y);
    return ring; // RVO
}

} // end ns detail

template <typename NextProcessor>
class geometry_clipper
{
private:
    NextProcessor & next_;
    mapbox::geometry::box<std::int64_t> const& tile_clipping_extent_;
    double area_threshold_;
    bool strictly_simple_;
    bool multi_polygon_union_;
    polygon_fill_type fill_type_;
    bool process_all_rings_;
public:
    geometry_clipper(mapbox::geometry::box<std::int64_t> const& tile_clipping_extent,
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

    void operator() (mapbox::geometry::point<std::int64_t> & geom)
    {
        next_(geom);
    }

    void operator() (mapbox::geometry::multi_point<std::int64_t> & geom)
    {
        // Here we remove repeated points from multi_point
        auto last = std::unique(geom.begin(), geom.end());
        geom.erase(last, geom.end());
        next_(geom);
    }

    void operator() (mapbox::geometry::geometry_collection<std::int64_t> & geom)
    {
        for (auto & g : geom)
        {
            mapbox::util::apply_visitor((*this), g);
        }
    }

    void operator() (mapbox::geometry::line_string<std::int64_t> & geom)
    {
        boost::geometry::unique(geom);
        if (geom.size() < 2)
        {
            return;
        }
        mapbox::geometry::multi_line_string<int64_t> result;
        auto clip_box = detail::box_to_ring(tile_clipping_extent_);
        boost::geometry::intersection(clip_box, geom, result);
        if (result.empty())
        {
            return;
        }
        next_(result);
    }

    void operator() (mapbox::geometry::multi_line_string<std::int64_t> & geom)
    {
        if (geom.empty())
        {
            return;
        }

        auto clip_box = detail::box_to_ring(tile_clipping_extent_);
        boost::geometry::unique(geom);
        mapbox::geometry::multi_line_string<int64_t> results;
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

    void operator() (mapbox::geometry::polygon<std::int64_t> & geom)
    {
        if (geom.empty() || ((geom.front().size() < 3) && !process_all_rings_))
        {
            return;
        }

        mapbox::geometry::wagyu::wagyu<std::int64_t> clipper;
        bool first = true;
        for (auto & ring : geom) {
            if (ring.size() < 3)
            {
                if (first) {
                    if (process_all_rings_) {
                        first = false;
                    } else {
                        return;
                    }
                }
                continue;
            }
            double area = detail::area(ring);
            if (first) {
                first = false;
                if ((std::abs(area) < area_threshold_)  && !process_all_rings_) {
                    return;
                }
                if (area < 0) {
                    std::reverse(ring.begin(), ring.end());
                }
                auto new_ring = mapbox::geometry::wagyu::quick_clip::quick_lr_clip(ring, tile_clipping_extent_);
                if (new_ring.empty()) {
                    if (process_all_rings_) {
                        continue;
                    }
                    return;
                }
                clipper.add_ring(new_ring);
            } else {
                if (std::abs(area) < area_threshold_) {
                    continue;
                }
                if (area > 0)
                {
                    std::reverse(ring.begin(), ring.end());
                }
                auto new_ring = mapbox::geometry::wagyu::quick_clip::quick_lr_clip(ring, tile_clipping_extent_);
                if (new_ring.empty()) {
                    continue;
                }
                clipper.add_ring(new_ring);
            }
        }

        mapbox::geometry::multi_polygon<std::int64_t> mp;

        clipper.execute(mapbox::geometry::wagyu::clip_type_union,
                        mp,
                        detail::get_wagyu_fill_type(fill_type_),
                        mapbox::geometry::wagyu::fill_type_even_odd);

        if (mp.empty())
        {
            return;
        }
        next_(mp);
    }

    void operator() (mapbox::geometry::multi_polygon<std::int64_t> & geom)
    {
        if (geom.empty())
        {
            return;
        }

        mapbox::geometry::multi_polygon<std::int64_t> mp;
        if (multi_polygon_union_)
        {
            mapbox::geometry::wagyu::wagyu<std::int64_t> clipper;
            for (auto & poly : geom)
            {
                bool first = true;
                for (auto & ring : poly) {
                    if (ring.size() < 3)
                    {
                        if (first) {
                            first = false;
                            if (!process_all_rings_) {
                                break;
                            }
                        }
                        continue;
                    }
                    double area = detail::area(ring);
                    if (first) {
                        first = false;
                        if ((std::abs(area) < area_threshold_)  && !process_all_rings_) {
                            break;
                        }
                        if (area < 0) {
                            std::reverse(ring.begin(), ring.end());
                        }
                        auto new_ring = mapbox::geometry::wagyu::quick_clip::quick_lr_clip(ring, tile_clipping_extent_);
                        if (new_ring.empty()) {
                            if (process_all_rings_) {
                                continue;
                            }
                            break;
                        }
                        clipper.add_ring(new_ring);
                    } else {
                        if (std::abs(area) < area_threshold_) {
                            continue;
                        }
                        if (area > 0)
                        {
                            std::reverse(ring.begin(), ring.end());
                        }
                        auto new_ring = mapbox::geometry::wagyu::quick_clip::quick_lr_clip(ring, tile_clipping_extent_);
                        if (new_ring.empty()) {
                            continue;
                        }
                        clipper.add_ring(new_ring);
                    }
                }
            }
            clipper.execute(mapbox::geometry::wagyu::clip_type_union,
                            mp,
                            detail::get_wagyu_fill_type(fill_type_),
                            mapbox::geometry::wagyu::fill_type_even_odd);
        }
        else
        {
            for (auto & poly : geom)
            {
                mapbox::geometry::wagyu::wagyu<std::int64_t> clipper;
                mapbox::geometry::multi_polygon<std::int64_t> tmp_mp;
                bool first = true;
                for (auto & ring : poly) {
                    if (ring.size() < 3)
                    {
                        if (first) {
                            first = false;
                            if (!process_all_rings_) {
                                break;
                            }
                        }
                        continue;
                    }
                    double area = detail::area(ring);
                    if (first) {
                        first = false;
                        if ((std::abs(area) < area_threshold_)  && !process_all_rings_) {
                            break;
                        }
                        if (area < 0) {
                            std::reverse(ring.begin(), ring.end());
                        }
                        auto new_ring = mapbox::geometry::wagyu::quick_clip::quick_lr_clip(ring, tile_clipping_extent_);
                        if (new_ring.empty()) {
                            if (process_all_rings_) {
                                continue;
                            }
                            break;
                        }
                        clipper.add_ring(new_ring);
                    } else {
                        if (std::abs(area) < area_threshold_) {
                            continue;
                        }
                        if (area > 0)
                        {
                            std::reverse(ring.begin(), ring.end());
                        }
                        auto new_ring = mapbox::geometry::wagyu::quick_clip::quick_lr_clip(ring, tile_clipping_extent_);
                        if (new_ring.empty()) {
                            continue;
                        }
                        clipper.add_ring(new_ring);
                    }
                }
                clipper.execute(mapbox::geometry::wagyu::clip_type_union,
                                tmp_mp,
                                detail::get_wagyu_fill_type(fill_type_),
                                mapbox::geometry::wagyu::fill_type_even_odd);
                mp.insert(mp.end(), tmp_mp.begin(), tmp_mp.end());
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
