#ifndef __MAPNIK_VECTOR_TILE_MERC_TILE_H__
#define __MAPNIK_VECTOR_TILE_MERC_TILE_H__

// mapnik-vector-tile
#include "vector_tile_tile.hpp"
#include "vector_tile_projection.hpp"

namespace mapnik
{

namespace vector_tile_impl
{

struct merc_tile : public tile
{
private:
    std::uint32_t x_;
    std::uint32_t y_;
    std::uint32_t z_;
public:
    merc_tile(std::uint32_t x,
              std::uint32_t y,
              std::uint32_t z,
              std::uint32_t tile_size = 256,
              std::uint32_t buffer_size = 0,
              std::uint32_t path_multiplier = 16)
        : tile(merc_extent(tile_size, x, y, z), tile_size, buffer_size, path_multiplier),
          x_(x),
          y_(y),
          z_(z) {}

    merc_tile(merc_tile const& rhs) = default;

    merc_tile(merc_tile && rhs) = default;

};

} // end ns vector_tile_impl

} // end ns mapnik

#endif // __MAPNIK_VECTOR_TILE_MERC_TILE_H__

