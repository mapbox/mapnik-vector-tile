#ifndef __MAPNIK_VECTOR_TILE_TILE_H__
#define __MAPNIK_VECTOR_TILE_TILE_H__

// mapnik-vector-tile
#include "vector_tile_layer.hpp"

// std
#include <set>
#include <string>

namespace mapnik
{

namespace vector_tile_impl
{

class tile
{
private:
    std::string buffer_;
    std::set<std::string> painted_layers_;
    std::set<std::string> empty_layers_;
    std::set<std::string> layers_;
    mapnik::box2d<double> extent_;
    std::uint32_t tile_size_;
    std::uint32_t buffer_size_;
    std::uint32_t path_multiplier_;

public:
    tile(mapnik::box2d<double> const& extent,
         std::uint32_t tile_size = 256,
         std::uint32_t buffer_size = 0,
         std::uint32_t path_multiplier = 16)
        : buffer_(),
          painted_layers_(),
          empty_layers_(),
          layers_(),
          extent_(extent),
          tile_size_(tile_size),
          buffer_size_(buffer_size),
          path_multiplier_(path_multiplier) {}

    tile(tile const& rhs) = default;

    tile(tile && rhs) = default;

    bool add_layer(tile_layer & layer)
    {
        std::string const& new_name = layer.name();
        if (layer.is_empty())
        {
            empty_layers_.insert(new_name);
            if (layer.is_painted())
            {
                painted_layers_.insert(new_name);
            }
        }
        else
        {
            painted_layers_.insert(new_name);
            auto p = layers_.insert(new_name);
            if (!p.second)
            {
                // Layer already in tile
                return false;
            }
            buffer_.append(layer.get_data());
            auto itr = empty_layers_.find(new_name);
            if (itr != empty_layers_.end())
            {
                empty_layers_.erase(itr);
            }
        }
        return true;
    }
    
    std::size_t size() const
    {
        return buffer_.size();
    }

    double scale() const
    {
        if (tile_size_ > 0)
        {
            return extent_.width()/tile_size_;
        }
        return extent_.width();
    }

    box2d<double> get_buffered_extent() const
    {
        double extra = 2.0 * scale() * buffer_size_;
        box2d<double> ext(extent_);
        ext.width(extent_.width() + extra);
        ext.height(extent_.height() + extra);
        return ext;
    }
    
    std::uint32_t tile_extent() const
    {
        return tile_size_ * path_multiplier_;
    }

    void append_to_string(std::string & str) const
    {
        str.append(buffer_);
    }

    void serialize_to_string(std::string & str) const
    {
        str = buffer_;
    }

    vector_tile::Tile get_tile() const
    {
        vector_tile::Tile t;
        t.ParseFromString(buffer_);
        return t;
    }

    bool is_painted() const
    {
        return !painted_layers_.empty();
    }

    bool is_empty() const
    {
        return layers_.empty();
    }

    box2d<double> const& extent() const
    {
        return extent_;
    }

    std::uint32_t tile_size() const
    {
        return tile_size_;
    }

    void tile_size(std::uint32_t val)
    {
        tile_size_ = val;
    }

    std::uint32_t path_multiplier() const
    {
        return path_multiplier_;
    }

    void path_multiplier(std::uint32_t val)
    {
        path_multiplier_ = val;
    }

    std::uint32_t buffer_size() const
    {
        return buffer_size_;
    }

    void buffer_size(std::uint32_t val)
    {
        buffer_size_ = val;
    }

    std::set<std::string> const& get_painted_layers() const
    {
        return painted_layers_;
    }

    std::set<std::string> const& get_empty_layers() const
    {
        return empty_layers_;
    }

};

} // end ns vector_tile_impl

} // end ns mapnik

#endif // __MAPNIK_VECTOR_TILE_TILE_H__
