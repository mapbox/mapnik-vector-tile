#ifndef __MAPNIK_VECTOR_TILE_GEOMETRY_ENCODER_H__
#define __MAPNIK_VECTOR_TILE_GEOMETRY_ENCODER_H__

// vector tile
#include "vector_tile.pb.h"
#include <mapnik/vertex.hpp>
#include "vector_tile_config.hpp"

#include <cstdlib>
#include <cmath>
#include <sstream>

namespace mapnik { namespace vector_tile_impl {

MAPNIK_VECTOR_INLINE void handle_skipped_last(vector_tile::Tile_Feature & current_feature,
                                int32_t skipped_index,
                                int32_t cur_x,
                                int32_t cur_y,
                                int32_t & x_,
                                int32_t & y_);

void rollback_geom(vector_tile::Tile_Feature & current_feature, unsigned idx)
{
   auto num_geometries = current_feature.geometry_size();
   if (num_geometries > 0)
   {
        unsigned num_geometries2 = static_cast<unsigned>(num_geometries);
        if (num_geometries2 > idx)
        {
            auto geom = current_feature.mutable_geometry();
            while (idx < static_cast<unsigned>(num_geometries2))
            {
                geom->RemoveLast();
                --num_geometries2;
            }
        }
   }
}

template <typename T>
inline unsigned encode_geometry(T & path,
                         vector_tile::Tile_Feature & current_feature,
                         int32_t & x_,
                         int32_t & y_,
                         unsigned tolerance,
                         unsigned path_multiplier)
{
    bool is_polygon = (current_feature.type() == vector_tile::Tile_GeomType_POLYGON);
    bool is_line_string = (current_feature.type() == vector_tile::Tile_GeomType_LINESTRING);
    bool working_on_exterior_ring = is_polygon;
    unsigned verts = 0;
    unsigned count = 0;
    path.rewind(0);
    unsigned first_geom_idx = current_feature.geometry_size();

    vertex2d vtx(vertex2d::no_init);
    int cmd = -1;
    int prev_cmd = -1;
    int cmd_idx = -1;
    const int cmd_bits = 3;
    unsigned length = 0;
    bool skipped_last = false;
    int32_t skipped_index = -1;
    int32_t cur_x = 0;
    int32_t cur_y = 0;

    // See vector_tile.proto for a description of how vertex command
    // encoding works.

    std::vector<vertex2d> output;
    const std::size_t buffer_size = 8;
    output.reserve(buffer_size);
    bool done = false;
    bool cache = true;
    while (true)
    {
        if (cache)
        {
            // read
            vertex2d v(vertex2d::no_init);
            while ((v.cmd = path.vertex(&v.x, &v.y)) != SEG_END)
            {
                output.push_back(std::move(v));
                if (output.size() == buffer_size) break;
            }
            cache = false;
            if (v.cmd == SEG_END)
            {
                done = true;
            }
        }
        else
        {
            if (done && output.empty()) break;
            // process
            vtx = output.front();
            {
                if (static_cast<int>(vtx.cmd) != cmd)
                {
                    if (cmd_idx >= 0)
                    {
                        // Encode the previous length/command value.
                        current_feature.set_geometry(cmd_idx, (length << cmd_bits) | (cmd & ((1 << cmd_bits) - 1)));
                        // abort on degenerate exterior ring of polygon
                        if (working_on_exterior_ring && cmd == SEG_CLOSE)
                        {
                            working_on_exterior_ring = false;
                            if (verts < 4)
                            {
                                rollback_geom(current_feature,first_geom_idx);
                                return 0;
                            }
                        }
                    }
                    cmd = static_cast<int>(vtx.cmd);
                    length = 0;
                    cmd_idx = current_feature.geometry_size();
                    current_feature.add_geometry(0); // placeholder added in first pass
                }

                switch (vtx.cmd)
                {
                case SEG_MOVETO:
                case SEG_LINETO:
                {
                    if (cmd == SEG_MOVETO && skipped_last && skipped_index > 1) // at least one vertex + cmd/length
                    {
                        // if we skipped previous vertex we just update it to the last one here.
                        handle_skipped_last(current_feature, skipped_index, cur_x, cur_y,  x_, y_);
                    }

                    // Compute delta to the previous coordinate.
                    cur_x = static_cast<int32_t>(std::floor((vtx.x * path_multiplier) + 0.5));
                    cur_y = static_cast<int32_t>(std::floor((vtx.y * path_multiplier) + 0.5));
                    int32_t dx = cur_x - x_;
                    int32_t dy = cur_y - y_;
                    // we try hard not to collapse corners that
                    // may have resulted from clipping
                    bool next_segment_axis_aligned = false;
                    auto output_size = output.size();
                    if (output_size > 1)
                    {
                        vertex2d const& next_vtx = output[1];
                        if (next_vtx.cmd == SEG_LINETO)
                        {
                            uint32_t next_dx = std::abs(cur_x - static_cast<int32_t>(std::floor((next_vtx.x * path_multiplier) + 0.5)));
                            uint32_t next_dy = std::abs(cur_y - static_cast<int32_t>(std::floor((next_vtx.y * path_multiplier) + 0.5)));
                            if ((next_dx == 0 && next_dy >= tolerance) || (next_dy == 0 && next_dx >= tolerance))
                            {
                                next_segment_axis_aligned = true;
                            }
                        }
                    }
                    // Keep all move_to commands, but omit other movements that are
                    // not >= the tolerance threshold and should be considered no-ops.
                    // NOTE: length == 0 indicates the command has changed and will
                    // preserve any non duplicate move_to or line_to
                    if ( length == 0 || next_segment_axis_aligned ||
                         (static_cast<unsigned>(std::abs(dx)) >= tolerance) ||
                         (static_cast<unsigned>(std::abs(dy)) >= tolerance)
                        )
                    {
                        // Manual zigzag encoding.
                        current_feature.add_geometry((dx << 1) ^ (dx >> 31));
                        current_feature.add_geometry((dy << 1) ^ (dy >> 31));
                        x_ = cur_x;
                        y_ = cur_y;
                        skipped_last = false;
                        ++length;
                    }
                    else
                    {
                        skipped_last = true;
                        skipped_index = current_feature.geometry_size();
                    }
                    break;
                }

                case SEG_CLOSE:
                {
                    if (prev_cmd != SEG_CLOSE) ++length;
                    break;
                }
                default:
                    std::stringstream msg;
                    msg << "Unknown command type (backend_pbf): "
                        << cmd;
                    throw std::runtime_error(msg.str());
                    break;
                }
            }
            ++count;
            ++verts;
            prev_cmd = cmd;
            output.erase(output.begin());
            if (output.size() < 2) cache = true;
        }
    }
    if (skipped_last && skipped_index > 1) // at least one vertex + cmd/length
    {
        // if we skipped previous vertex we just update it to the last one here.
        handle_skipped_last(current_feature, skipped_index, cur_x, cur_y, x_, y_);
    }
    // Update the last length/command value.
    if (cmd_idx >= 0)
    {
        current_feature.set_geometry(cmd_idx, (length << cmd_bits) | (cmd & ((1 << cmd_bits) - 1)));
        // abort on degenerate exterior ring of polygon
        if (working_on_exterior_ring && cmd == SEG_CLOSE)
        {
            working_on_exterior_ring = false;
            if (verts < 4)
            {
                rollback_geom(current_feature,first_geom_idx);
                return 0;
            }
        }
        // abort on degenerate line
        else if (is_line_string && verts < 2)
        {
            rollback_geom(current_feature,first_geom_idx);
            return 0;
        }
    }
    return count;
}

}} // end ns

#if !defined(MAPNIK_VECTOR_TILE_LIBRARY)
#include "vector_tile_geometry_encoder.ipp"
#endif

#endif // __MAPNIK_VECTOR_TILE_GEOMETRY_ENCODER_H__
