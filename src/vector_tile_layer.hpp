#ifndef __MAPNIK_VECTOR_TILE_LAYER_H__
#define __MAPNIK_VECTOR_TILE_LAYER_H__

// mapnik-vector-tile
#include "vector_tile_config.hpp"

// mapnik
#include <mapnik/feature.hpp>
#include <mapnik/value.hpp>

// protozero
#include <protozero/pbf_writer.hpp>

// std
#include <map>
#include <unordered_map>

namespace mapnik
{

namespace vector_tile_impl
{

struct layer_builder_pbf
{
    typedef std::map<std::string, unsigned> keys_container;
    typedef std::unordered_map<mapnik::value, unsigned> values_container;

    std::string feature_str;
    keys_container keys;
    values_container values;
    protozero::pbf_writer layer_writer;
    bool empty;
    bool painted;

    layer_builder_pbf(std::string const & name, std::uint32_t extent, std::string & layer_buffer)
        : feature_str(),
          keys(),
          values(),
          layer_writer(layer_buffer),
          empty(true),
          painted(false)
    {
        layer_writer.add_uint32(Layer_Encoding::VERSION, 2);
        layer_writer.add_string(Layer_Encoding::NAME, name);
        layer_writer.add_uint32(Layer_Encoding::EXTENT, extent);
    }

    void make_painted()
    {
        painted = true;
    }

    protozero::pbf_writer get_feature_writer()
    {
        feature_str.clear();
        return protozero::pbf_writer(feature_str);
    }

    MAPNIK_VECTOR_INLINE void add_feature(protozero::pbf_writer & feature_writer, 
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

    std::string & get_data()
    {
        return buffer_;
    }

    void build(layer_builder_pbf const& builder)
    {
        empty_ = builder.empty;
        painted_ = builder.painted;
    }
};

} // end ns vector_tile_impl

} // end ns mapnik

#if !defined(MAPNIK_VECTOR_TILE_LIBRARY)
#include "vector_tile_layer.ipp"
#endif

#endif // __MAPNIK_VECTOR_TILE_LAYER_H__
