#ifndef __MAPNIK_VECTOR_TILE_TILE_H__
#define __MAPNIK_VECTOR_TILE_TILE_H__

// mapnik-vector-tile
#include "vector_tile_config.hpp"

// mapnik
#include <mapnik/layer.hpp>

// libprotobuf
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include "vector_tile.pb.h"
#pragma GCC diagnostic pop

// std
#include <algorithm>

namespace mapnik
{

namespace vector_tile_impl
{

struct tile
{
public:
    tile();
    tile(std::string const& str);
    tile(tile const& rhs)
        : tile_(new vector_tile::Tile(*rhs.tile_)),
          empty_layers_(rhs.empty_layers_),
          painted_layers_(rhs.painted_layers_)
    {
    }

    tile(tile && rhs)
        : tile_(std::move(rhs.tile_)),
          empty_layers_(std::move(rhs.empty_layers_)),
          painted_layers_(std::move(rhs.painted_layers_))
    {
    }

    tile& operator=(tile rhs)
    {
        swap(rhs);
        return *this;
    }
    
    void swap(tile & rhs)
    {
        std::swap(tile_, rhs.tile_);
        std::swap(empty_layers_, rhs.empty_layers_);
        std::swap(painted_layers_, rhs.painted_layers_);
    }

    MAPNIK_VECTOR_INLINE bool add_layer(std::unique_ptr<vector_tile::Tile_Layer> & vt_layer, 
                                        bool layer_is_painted,
                                        bool layer_is_empty);
    
    MAPNIK_VECTOR_INLINE bool append_to_string(std::string & str);

    MAPNIK_VECTOR_INLINE bool serialize_to_string(std::string & str);

    vector_tile::Tile & get_tile()
    {
        return *tile_;
    }

    void clear()
    {
        tile_->Clear();
    }

    vector_tile::Tile const& get_tile() const
    {
        return *tile_;
    }

    bool is_painted() const
    {
        return !painted_layers_.empty();
    }

    bool is_empty() const
    {
        return tile_->layers_size() == 0;
    }

    std::vector<std::string> const& get_painted() const
    {
        return painted_layers_;
    }

    std::vector<std::string> const& get_empty() const
    {
        return empty_layers_;
    }

private:
    std::unique_ptr<vector_tile::Tile> tile_;
    std::vector<std::string> empty_layers_;
    std::vector<std::string> painted_layers_;
};

} // end ns vector_tile_impl

} // end ns mapnik

#if !defined(MAPNIK_VECTOR_TILE_LIBRARY)
#include "vector_tile_tile.ipp"
#endif

#endif // __MAPNIK_VECTOR_TILE_TILE_H__
