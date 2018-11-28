#ifndef __MAPNIK_VECTOR_TILE_MERC_TILE_H__
#define __MAPNIK_VECTOR_TILE_MERC_TILE_H__

// mapnik-vector-tile
#include "vector_tile_tile.hpp"
#include "vector_tile_projection.hpp"

namespace mapnik
{

namespace vector_tile_impl
{

class merc_tile : public tile
{
private:
    std::uint64_t x_;
    std::uint64_t y_;
    std::uint64_t z_;
public:
    merc_tile(std::uint64_t x,
              std::uint64_t y,
              std::uint64_t z,
              std::uint32_t tile_size = 4096,
              std::int32_t buffer_size = 128)
        : tile(tile_mercator_bbox(x, y, z), tile_size, buffer_size),
          x_(x),
          y_(y),
          z_(z) {}

    merc_tile(merc_tile && rhs) = default;

    bool same_extent(merc_tile const& other)
    {
        return x_ == other.x_ &&
               y_ == other.y_ &&
               z_ == other.z_;
    }

    std::uint64_t x() const
    {
        return x_;
    }

    void x(std::uint64_t x)
    {
        x_ = x;
        extent_ = tile_mercator_bbox(x_, y_, z_);
    }

    std::uint64_t y() const
    {
        return y_;
    }

    void y(std::uint64_t y)
    {
        y_ = y;
        extent_ = tile_mercator_bbox(x_, y_, z_);
    }

    std::uint64_t z() const
    {
        return z_;
    }

    void z(std::uint64_t z)
    {
        z_ = z;
        extent_ = tile_mercator_bbox(x_, y_, z_);
    }
};

typedef std::shared_ptr<merc_tile> merc_tile_ptr;

} // end ns vector_tile_impl

} // end ns mapnik

#endif // __MAPNIK_VECTOR_TILE_MERC_TILE_H__
