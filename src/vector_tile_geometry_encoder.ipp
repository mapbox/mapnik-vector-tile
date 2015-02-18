// vector tile
#include "vector_tile.pb.h"

namespace mapnik { namespace vector_tile_impl {

void handle_skipped_last(vector_tile::Tile_Feature & current_feature,
                                int32_t skipped_index,
                                int32_t cur_x,
                                int32_t cur_y,
                                int32_t & x_,
                                int32_t & y_)
{
    uint32_t last_x = current_feature.geometry(skipped_index - 2);
    uint32_t last_y = current_feature.geometry(skipped_index - 1);
    int32_t last_dx = ((last_x >> 1) ^ (-(last_x & 1)));
    int32_t last_dy = ((last_y >> 1) ^ (-(last_y & 1)));
    int32_t dx = cur_x - x_ + last_dx;
    int32_t dy = cur_y - y_ + last_dy;
    x_ = cur_x;
    y_ = cur_y;
    current_feature.set_geometry(skipped_index - 2, ((dx << 1) ^ (dx >> 31)));
    current_feature.set_geometry(skipped_index - 1, ((dy << 1) ^ (dy >> 31)));
}


}} // end ns
