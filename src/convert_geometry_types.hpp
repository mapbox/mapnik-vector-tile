#pragma once

// Mapnik
#include <mapnik/geometry.hpp>

// Mapbox
#include <mapbox/geometry/geometry.hpp>

namespace mapnik
{

namespace vector_tile_impl
{

template <typename T>
mapbox::geometry::linear_ring<T> mapnik_to_mapbox(mapnik::geometry::linear_ring<T> const& input)
{
    mapbox::geometry::linear_ring<T> r;
    r.reserve(input.size());
    for (auto const& pt : input) {
        r.emplace_back(pt.x, pt.y);
    }
    return r;
}

template <typename T>
mapbox::geometry::polygon<T> mapnik_to_mapbox(mapnik::geometry::polygon<T> const& input)
{
    mapbox::geometry::polygon<T> out;
    if (input.exterior_ring.empty())
    {
        return out;
    }

    out.push_back(mapnik_to_mapbox(input.exterior_ring));

    for (auto const& in_r : input.interior_rings) {
        out.push_back(mapnik_to_mapbox(in_r));
    }

    return out;
}

template <typename T>
mapbox::geometry::multi_polygon<T> mapnik_to_mapbox(mapnik::geometry::multi_polygon<T> const& input)
{
    mapbox::geometry::multi_polygon<T> out;
    for (auto const& poly : input) {
        out.push_back(mapnik_to_mapbox(poly));
    }
    return out;
}

template <typename T>
mapnik::geometry::linear_ring<T> mapbox_to_mapnik(mapbox::geometry::linear_ring<T> const& input)
{
    mapnik::geometry::linear_ring<T> r;
    r.reserve(input.size());
    for (auto const& pt : input) {
        r.emplace_back(pt.x, pt.y);
    }
    return r;
}

template <typename T>
mapnik::geometry::polygon<T> mapbox_to_mapnik(mapbox::geometry::polygon<T> const& input)
{
    mapnik::geometry::polygon<T> out;

    bool first = true;
    for (auto const& in_r : input) {
        if (first) {
            out.exterior_ring = mapbox_to_mapnik(in_r);
            first = false;
        } else {
            out.interior_rings.push_back(mapbox_to_mapnik(in_r));
        }
    }
    return out;
}

template <typename T>
mapnik::geometry::multi_polygon<T> mapbox_to_mapnik(mapbox::geometry::multi_polygon<T> const& input)
{
    mapnik::geometry::multi_polygon<T> out;
    for (auto const& poly : input) {
        out.push_back(mapbox_to_mapnik(poly));
    }
    return out;
}

} // end ns vector_tile_impl

} // end ns mapnik
