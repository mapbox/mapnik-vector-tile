#ifndef __MAPNIK_VECTOR_TILE_GEOMETRY_ENCODER_H__
#define __MAPNIK_VECTOR_TILE_GEOMETRY_ENCODER_H__

// vector tile
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include "vector_tile.pb.h"
#pragma GCC diagnostic pop

#include <mapnik/geometry.hpp>
#include "vector_tile_config.hpp"
#include <protozero/varint.hpp>

#include <cstdlib>
#include <cmath>
#include <sstream>
#include <iostream>

namespace mapnik { namespace vector_tile_impl {

inline bool encode_geometry(mapnik::geometry::point<std::int64_t> const& pt,
                        vector_tile::Tile_Feature & current_feature,
                        int32_t & start_x,
                        int32_t & start_y)
{
    current_feature.add_geometry(9); // 1 | (move_to << 3)
    int32_t dx = pt.x - start_x;
    int32_t dy = pt.y - start_y;
    // Manual zigzag encoding.
    current_feature.add_geometry(protozero::encode_zigzag32(dx));
    current_feature.add_geometry(protozero::encode_zigzag32(dy));
    start_x = pt.x;
    start_y = pt.y;
    return true;
}

inline unsigned encode_length(unsigned len)
{
    return (len << 3u) | 2u;
}

inline bool encode_geometry(mapnik::geometry::line_string<std::int64_t> const& line,
                        vector_tile::Tile_Feature & current_feature,
                        int32_t & start_x,
                        int32_t & start_y)
{
    std::size_t line_size = line.size();
    if (line_size <= 0)
    {
        return false;
    }
    unsigned line_to_length = static_cast<unsigned>(line_size) - 1;

    enum {
        move_to = 1,
        line_to = 2,
        coords = 3
    } status = move_to;

    for (auto const& pt : line)
    {
        if (status == move_to)
        {
            status = line_to;
            current_feature.add_geometry(9); // 1 | (move_to << 3)
        }
        else if (status == line_to)
        {
            status = coords;
            current_feature.add_geometry(encode_length(line_to_length));
        }
        int32_t dx = pt.x - start_x;
        int32_t dy = pt.y - start_y;
        // Manual zigzag encoding.
        current_feature.add_geometry(protozero::encode_zigzag32(dx));
        current_feature.add_geometry(protozero::encode_zigzag32(dy));
        start_x = pt.x;
        start_y = pt.y;
    }
    return true;
}

inline bool encode_geometry(mapnik::geometry::linear_ring<std::int64_t> const& ring,
                        vector_tile::Tile_Feature & current_feature,
                        int32_t & start_x,
                        int32_t & start_y)
{
    std::size_t ring_size = ring.size();
    if (ring_size < 3)
    {
        return false;
    }
    unsigned line_to_length = static_cast<unsigned>(ring_size) - 1;
    unsigned count = 0;
    enum {
        move_to = 1,
        line_to = 2,
        coords = 3
    } status = move_to;
    bool drop_last = false;
    if (ring.size() > 2 && ring.front() == ring.back())
    {
        drop_last = true;
        line_to_length -= 1;
        if (line_to_length < 2)
        {
            return false;
        }
    }

    for (auto const& pt : ring)
    {
        if (status == move_to)
        {
            status = line_to;
            current_feature.add_geometry(9); // 1 | (move_to << 3)
        }
        else if (status == line_to)
        {
            status = coords;
            current_feature.add_geometry(encode_length(line_to_length));
        }
        else if (drop_last && count == line_to_length + 1)
        {
            continue;
        }
        int32_t dx = pt.x - start_x;
        int32_t dy = pt.y - start_y;
        // Manual zigzag encoding.
        current_feature.add_geometry(protozero::encode_zigzag32(dx));
        current_feature.add_geometry(protozero::encode_zigzag32(dy));
        start_x = pt.x;
        start_y = pt.y;
        ++count;
    }
    current_feature.add_geometry(15); // close_path
    return true;
}

inline bool encode_geometry(mapnik::geometry::polygon<std::int64_t> const& poly,
                            vector_tile::Tile_Feature & current_feature,
                            int32_t & start_x,
                            int32_t & start_y)
{
    if (!encode_geometry(poly.exterior_ring, current_feature, start_x, start_y))
    {
        return false;
    }
    for (auto const& ring : poly.interior_rings)
    {
        encode_geometry(ring, current_feature, start_x, start_y);
    }
    return true;
}

inline bool encode_geometry(mapnik::geometry::multi_point<std::int64_t> const& geom,
                            vector_tile::Tile_Feature & current_feature,
                            int32_t & start_x,
                            int32_t & start_y)
{
    std::size_t geom_size = geom.size();
    if (geom_size <= 0)
    {
        return false;
    }
    current_feature.add_geometry(1u | (geom_size << 3)); // move_to | (len << 3)
    for (auto const& pt : geom)
    {
        int32_t dx = pt.x - start_x;
        int32_t dy = pt.y - start_y;
        // Manual zigzag encoding.
        current_feature.add_geometry(protozero::encode_zigzag32(dx));
        current_feature.add_geometry(protozero::encode_zigzag32(dy));
        start_x = pt.x;
        start_y = pt.y;
    }
    return true;
}

inline bool encode_geometry(mapnik::geometry::multi_line_string<std::int64_t> const& geom,
                            vector_tile::Tile_Feature & current_feature,
                            int32_t & start_x,
                            int32_t & start_y)
{
    bool success = false;
    for (auto const& poly : geom)
    {
        if (encode_geometry(poly, current_feature, start_x, start_y))
        {
            success = true;
        }
    }
    return success;
}

inline bool encode_geometry(mapnik::geometry::multi_polygon<std::int64_t> const& geom,
                            vector_tile::Tile_Feature & current_feature,
                            int32_t & start_x,
                            int32_t & start_y)
{
    bool success = false;
    for (auto const& poly : geom)
    {
        if (encode_geometry(poly, current_feature, start_x, start_y))
        {
            success = true;
        }
    }
    return success;
}

}} // end ns

#endif // __MAPNIK_VECTOR_TILE_GEOMETRY_ENCODER_H__
