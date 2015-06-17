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
#include <cstdlib>
#include <cmath>
#include <sstream>
#include <iostream>

namespace mapnik { namespace vector_tile_impl {

inline unsigned encode_geometry(mapnik::geometry::point<std::int64_t> const& pt,
                        vector_tile::Tile_Feature & current_feature,
                        int32_t & start_x,
                        int32_t & start_y)
{
    current_feature.add_geometry(9); // 1 | (move_to << 3)
    int32_t dx = pt.x - start_x;
    int32_t dy = pt.y - start_y;
    // Manual zigzag encoding.
    current_feature.add_geometry((static_cast<unsigned>(dx) << 1) ^ (dx >> 31));
    current_feature.add_geometry((static_cast<unsigned>(dy) << 1) ^ (dy >> 31));
    start_x = pt.x;
    start_y = pt.y;
    return 1;
}

inline unsigned encode_geometry(mapnik::geometry::line_string<std::int64_t> const& line,
                        vector_tile::Tile_Feature & current_feature,
                        int32_t & start_x,
                        int32_t & start_y)
{
    const int cmd_bits = 3;
    int32_t line_to_length = static_cast<int32_t>(line.size()) - 1;

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
            current_feature.add_geometry((line_to_length << cmd_bits) | 2); // len | (line_to << 3)
        }
        int32_t dx = pt.x - start_x;
        int32_t dy = pt.y - start_y;
        // Manual zigzag encoding.
        current_feature.add_geometry((static_cast<unsigned>(dx) << 1) ^ (dx >> 31));
        current_feature.add_geometry((static_cast<unsigned>(dy) << 1) ^ (dy >> 31));
        start_x = pt.x;
        start_y = pt.y;
    }
    return line.size();
}

inline unsigned encode_geometry(mapnik::geometry::linear_ring<std::int64_t> const& ring,
                        vector_tile::Tile_Feature & current_feature,
                        int32_t & start_x,
                        int32_t & start_y)
{
    if (ring.size() < 3) return 0;
    const int cmd_bits = 3;
    int32_t line_to_length = static_cast<int32_t>(ring.size()) - 1;
    int count = 0;
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
            return 0;
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
            current_feature.add_geometry((line_to_length<< cmd_bits) | 2); // len | (line_to << 3)
        }
        else if (drop_last && count == line_to_length + 1)
        {
            continue;
        }
        int32_t dx = pt.x - start_x;
        int32_t dy = pt.y - start_y;
        // Manual zigzag encoding.
        current_feature.add_geometry((static_cast<unsigned>(dx) << 1) ^ (dx >> 31));
        current_feature.add_geometry((static_cast<unsigned>(dy) << 1) ^ (dy >> 31));
        start_x = pt.x;
        start_y = pt.y;
        ++count;
    }
    current_feature.add_geometry(15); // close_path
    return line_to_length;
}

inline unsigned encode_geometry(mapnik::geometry::polygon<std::int64_t> const& poly,
                                vector_tile::Tile_Feature & current_feature,
                                int32_t & x_,
                                int32_t & y_)
{
    unsigned count = 0;
    count += encode_geometry(poly.exterior_ring, current_feature, x_, y_);
    if (count == 0)
    {
        return count;
    }
    for (auto const& ring : poly.interior_rings)
    {
        count += encode_geometry(ring, current_feature, x_, y_);
    }
    return count;
}


}} // end ns

#endif // __MAPNIK_VECTOR_TILE_GEOMETRY_ENCODER_H__
