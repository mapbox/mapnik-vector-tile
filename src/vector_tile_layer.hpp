#ifndef __MAPNIK_VECTOR_TILE_LAYER_H__
#define __MAPNIK_VECTOR_TILE_LAYER_H__

// mapnik-vector-tile
#include "vector_tile_config.hpp"

// mapnik
#include <mapnik/feature.hpp>
#include <mapnik/value.hpp>

// libprotobuf
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include "vector_tile.pb.h"
#pragma GCC diagnostic pop

// std
#include <map>
#include <unordered_map>

namespace mapnik
{

namespace vector_tile_impl
{

struct layer_builder
{
    typedef std::map<std::string, unsigned> keys_container;
    typedef std::unordered_map<mapnik::value, unsigned> values_container;

    vector_tile::Tile tile;
    vector_tile::Tile_Layer * layer;
    keys_container keys;
    values_container values;
    bool empty;
    bool painted;

    layer_builder(std::string const& name, std::uint32_t extent);

    void make_painted()
    {
        painted = true;
    }

    MAPNIK_VECTOR_INLINE void add_feature(std::unique_ptr<vector_tile::Tile_Feature> & vt_feature, 
                                          mapnik::feature_impl const& mapnik_feature);
};

class tile_layer
{

private:
    std::string buffer_;
    std::string name_;
    bool empty_;
    bool painted_;

public:
    tile_layer()
        : buffer_(),
          name_(),
          empty_(true),
          painted_(false) {}

    tile_layer(tile_layer && rhs) = default;

    tile_layer(tile_layer const& rhs) = default;

    tile_layer& operator=(tile_layer rhs)
    {
        swap(rhs);
        return *this;
    }
    
    void swap(tile_layer & rhs)
    {
        std::swap(buffer_, rhs.buffer_);
        std::swap(name_, rhs.name_);
        std::swap(empty_, rhs.empty_);
        std::swap(painted_, rhs.painted_);
    }

    std::string const& name() const
    {
        return name_;
    }
    
    void name(std::string const& val)
    {
        name_ = val;
    }

    bool is_empty() const
    {
        return empty_;
    }
     
    bool is_painted() const
    {
        return painted_;
    }
    
    void make_painted()
    {
        painted_ = true;
    }

    std::string const& get_data() const
    {
        return buffer_;
    }

    void build(layer_builder & builder)
    {
        empty_ = builder.empty;
        painted_ = builder.painted;
        builder.tile.SerializeToString(&buffer_);
    }

};

} // end ns vector_tile_impl

} // end ns mapnik

#if !defined(MAPNIK_VECTOR_TILE_LIBRARY)
#include "vector_tile_layer.ipp"
#endif

#endif // __MAPNIK_VECTOR_TILE_LAYER_H__
