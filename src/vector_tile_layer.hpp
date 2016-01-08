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

struct tile_layer
{
    typedef std::map<std::string, unsigned> keys_container;
    typedef std::unordered_map<mapnik::value, unsigned> values_container;

    tile_layer() = delete;
    tile_layer(std::string const & name, std::uint32_t extent);
    
    tile_layer(tile_layer && rhs)
        : layer_(std::move(rhs.layer_)),
          keys_(std::move(rhs.keys_)),
          values_(std::move(rhs.values_)),
          empty_(std::move(rhs.empty_)),
          painted_(std::move(rhs.painted_))
    {
    }

    tile_layer(tile_layer const& rhs)
        : layer_(new vector_tile::Tile_Layer(*rhs.layer_)),
          keys_(rhs.keys_),
          values_(rhs.values_),
          empty_(rhs.empty_),
          painted_(rhs.painted_)
    {
    }

    tile_layer& operator=(tile_layer rhs)
    {
        swap(rhs);
        return *this;
    }
    
    void swap(tile_layer & rhs)
    {
        std::swap(layer_, rhs.layer_);
        std::swap(keys_, rhs.keys_);
        std::swap(values_, rhs.values_);
        std::swap(empty_, rhs.empty_);
        std::swap(painted_, rhs.painted_);
    }

    MAPNIK_VECTOR_INLINE void add_feature(std::unique_ptr<vector_tile::Tile_Feature> & vt_feature, 
                                          mapnik::feature_impl const& mapnik_feature);

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

    std::unique_ptr<vector_tile::Tile_Layer> & get_layer()
    {
        return layer_;
    }

private:
    std::unique_ptr<vector_tile::Tile_Layer> layer_;
    keys_container keys_;
    values_container values_;
    bool empty_;
    bool painted_;
};

} // end ns vector_tile_impl

} // end ns mapnik

#if !defined(MAPNIK_VECTOR_TILE_LIBRARY)
#include "vector_tile_layer.ipp"
#endif

#endif // __MAPNIK_VECTOR_TILE_LAYER_H__
