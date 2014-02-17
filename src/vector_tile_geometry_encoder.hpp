#ifndef __MAPNIK_VECTOR_TILE_GEOMETRY_ENCODER_H__
#define __MAPNIK_VECTOR_TILE_GEOMETRY_ENCODER_H__

// vector tile
#include "vector_tile.pb.h"
#include <mapnik/vertex.hpp>

namespace mapnik { namespace vector {

        template <typename T>
        unsigned encode_geometry(T & path,
                                 tile_GeomType type,
                                 tile_feature & current_feature,
                                 int32_t & x_,
                                 int32_t & y_,
                                 unsigned tolerance,
                                 unsigned path_multiplier)
        {
            unsigned count = 0;
            path.rewind(0);
            current_feature.set_type(type);

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
            while ((vtx.cmd = path.vertex(&vtx.x, &vtx.y)) != SEG_END)
            {
                if (static_cast<int>(vtx.cmd) != cmd)
                {
                    if (cmd_idx >= 0)
                    {
                        // Encode the previous length/command value.
                        current_feature.set_geometry(cmd_idx, (length << cmd_bits) | (cmd & ((1 << cmd_bits) - 1)));
                    }
                    cmd = static_cast<int>(vtx.cmd);
                    length = 0;
                    cmd_idx = current_feature.geometry_size();
                    current_feature.add_geometry(0); // placeholder added in first pass
                }

                if (cmd == SEG_MOVETO || cmd == SEG_LINETO)
                {
                    // Compute delta to the previous coordinate.
                    cur_x = static_cast<int32_t>(std::floor((vtx.x * path_multiplier) + 0.5));
                    cur_y = static_cast<int32_t>(std::floor((vtx.y * path_multiplier) + 0.5));
                    int32_t dx = cur_x - x_;
                    int32_t dy = cur_y - y_;
                    // Keep all move_to commands, but omit other movements that are
                    // not >= the tolerance threshold and should be considered no-ops.
                    // NOTE: length == 0 indicates the command has changed and will
                    // preserve any non duplicate move_to or line_to
                    if ( length == 0 ||
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
                }
                else if (cmd == SEG_CLOSE)
                {
                    if (prev_cmd != SEG_CLOSE) ++length;
                }
                else
                {
                    std::stringstream msg;
                    msg << "Unknown command type (backend_pbf): "
                        << cmd;
                    throw std::runtime_error(msg.str());
                }

                ++count;
                prev_cmd = cmd;
            }

            if (skipped_last && skipped_index > 1) // at least one vertex + cmd/length
            {
                // if we skipped previous vertex we just update it to the last one here.
                uint32_t last_x = current_feature.geometry(skipped_index - 2);
                uint32_t last_y = current_feature.geometry(skipped_index - 1);
                int32_t last_dx = ((last_x >> 1) ^ (-(last_x & 1)));
                int32_t last_dy = ((last_y >> 1) ^ (-(last_y & 1)));
                int32_t dx = cur_x - x_ + last_dx;
                int32_t dy = cur_y - y_ + last_dy;
                // FIXME : add tolerance check here to discard short segments
                current_feature.set_geometry(skipped_index - 2, ((dx << 1) ^ (dx >> 31)));
                current_feature.set_geometry(skipped_index - 1, ((dy << 1) ^ (dy >> 31)));
            }

            // Update the last length/command value.
            if (cmd_idx >= 0)
            {
                current_feature.set_geometry(cmd_idx, (length << cmd_bits) | (cmd & ((1 << cmd_bits) - 1)));
            }
            return count;
        }

}} // end ns

#endif // __MAPNIK_VECTOR_TILE_GEOMETRY_ENCODER_H__
