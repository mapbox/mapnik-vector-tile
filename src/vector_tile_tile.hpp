#ifndef __MAPNIK_VECTOR_TILE_TILE_H__
#define __MAPNIK_VECTOR_TILE_TILE_H__

// mapnik-vector-tile
#include "vector_tile_config.hpp"

//protozero
#include <protozero/pbf_reader.hpp>

// mapnik
#include <mapnik/geometry/box2d.hpp>

// std
#include <set>
#include <string>
#include <vector>
#include <memory>

namespace mapnik
{

namespace vector_tile_impl
{

// fwd declare
class tile_layer;

class tile
{
protected:
    std::unique_ptr<std::string> buffer_;
    std::set<std::string> painted_layers_;
    std::set<std::string> empty_layers_;
    std::set<std::string> layers_set_;
    std::vector<std::string> layers_;
    mapnik::box2d<double> extent_;
    std::uint32_t tile_size_;
    std::int32_t buffer_size_;

public:
    tile(mapnik::box2d<double> const& extent,
         std::uint32_t tile_size = 4096,
         std::int32_t buffer_size = 128)
        : buffer_(std::make_unique<std::string>()),
          painted_layers_(),
          empty_layers_(),
          layers_set_(),
          layers_(),
          extent_(extent),
          tile_size_(tile_size),
          buffer_size_(buffer_size) {}

    tile(tile && rhs) = default;

    MAPNIK_VECTOR_INLINE bool add_layer(tile_layer const& layer);

    void add_empty_layer(std::string const& name)
    {
        empty_layers_.insert(name);
    }

    const char * data() const
    {
        return buffer_->data();
    }

    std::size_t size() const
    {
        return buffer_->size();
    }

    std::string const& get_buffer() const
    {
        return *buffer_;
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
        double extra_width = extent_.width() + extra;
        double extra_height = extent_.height() + extra;
        if (extra_width < 0.0)
        {
            extra_width = 0.0;
        }
        if (extra_height < 0.0)
        {
            extra_height = 0.0;
        }
        ext.width(extra_width);
        ext.height(extra_height);
        return ext;
    }

    void append_to_string(std::string & str) const
    {
        str.append(*buffer_);
    }

    void serialize_to_string(std::string & str) const
    {
        str = *buffer_;
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

    std::int32_t buffer_size() const
    {
        return buffer_size_;
    }

    void buffer_size(std::int32_t val)
    {
        buffer_size_ = val;
    }

    MAPNIK_VECTOR_INLINE bool append_layer_buffer(const char * data, std::size_t size, std::string const& name);

    std::set<std::string> const& get_painted_layers() const
    {
        return painted_layers_;
    }

    std::set<std::string> const& get_empty_layers() const
    {
        return empty_layers_;
    }

    std::vector<std::string> const& get_layers() const
    {
        return layers_;
    }

    std::set<std::string> const& get_layers_set() const
    {
        return layers_set_;
    }

    bool same_extent(tile const& other) const
    {
        return extent_ == other.extent_;
    }

    void clear()
    {
        buffer_->clear();
        empty_layers_.clear();
        layers_.clear();
        layers_set_.clear();
        painted_layers_.clear();
    }

    bool has_layer(std::string const& name)
    {
        auto itr = layers_set_.find(name);
        return itr != layers_set_.end();
    }

    protozero::pbf_reader get_reader() const
    {
        return protozero::pbf_reader(buffer_->data(), buffer_->size());
    }

    std::unique_ptr<std::string> release_buffer()
    {
        std::unique_ptr<std::string> out = std::move(buffer_);
        buffer_ = std::make_unique<std::string>();
        clear();
        return out;
    }

    MAPNIK_VECTOR_INLINE bool layer_reader(std::string const& name, protozero::pbf_reader & layer_msg) const;

    MAPNIK_VECTOR_INLINE bool layer_reader(std::size_t index, protozero::pbf_reader & layer_msg) const;
};

} // end ns vector_tile_impl

} // end ns mapnik

#if !defined(MAPNIK_VECTOR_TILE_LIBRARY)
#include "vector_tile_tile.ipp"
#endif

#endif // __MAPNIK_VECTOR_TILE_TILE_H__
