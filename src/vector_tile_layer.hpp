#ifndef __MAPNIK_VECTOR_TILE_LAYER_H__
#define __MAPNIK_VECTOR_TILE_LAYER_H__

// mapnik
#include <mapnik/feature.hpp>
#include <mapnik/util/variant.hpp>
#include <mapnik/value.hpp>

// libprotobuf
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include "vector_tile.pb.h"
#pragma GCC diagnostic pop

// protozero
#include <protozero/pbf_writer.hpp>

// std
#include <map>
#include <unordered_map>

namespace mapnik
{

namespace vector_tile_impl
{

namespace detail
{

struct to_tile_value
{
public:
    to_tile_value(vector_tile::Tile_Value * value):
        value_(value) {}

    void operator () ( value_integer val ) const
    {
        // TODO: figure out shortest varint encoding.
        value_->set_int_value(val);
    }

    void operator () ( mapnik::value_bool val ) const
    {
        value_->set_bool_value(val);
    }

    void operator () ( mapnik::value_double val ) const
    {
        // TODO: Figure out how we can encode 32 bit floats in some cases.
        value_->set_double_value(val);
    }

    void operator () ( mapnik::value_unicode_string const& val ) const
    {
        std::string str;
        to_utf8(val, str);
        value_->set_string_value(str.data(), str.length());
    }

    void operator () ( mapnik::value_null const& /*val*/ ) const
    {
        // do nothing
    }
private:
    vector_tile::Tile_Value * value_;
};

struct to_tile_value_pbf
{
public:
    to_tile_value_pbf(protozero::pbf_writer & value):
        value_(value) {}

    void operator () ( value_integer val ) const
    {
        // TODO: figure out shortest varint encoding.
        value_.add_int64(Value_Encoding::INT,val);
    }

    void operator () ( mapnik::value_bool val ) const
    {
        value_.add_bool(Value_Encoding::BOOL,val);
    }

    void operator () ( mapnik::value_double val ) const
    {
        // TODO: Figure out how we can encode 32 bit floats in some cases.
        value_.add_double(Value_Encoding::DOUBLE, val);
    }

    void operator () ( mapnik::value_unicode_string const& val ) const
    {
        std::string str;
        to_utf8(val, str);
        value_.add_string(Value_Encoding::STRING, str);
    }

    void operator () ( mapnik::value_null const& /*val*/ ) const
    {
        // do nothing
    }
private:
    protozero::pbf_writer & value_;
};

} // end ns detail

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

    layer_builder(std::string const & name, std::uint32_t extent)
        : tile(), 
          layer(tile.add_layers()),
          keys(),
          values(),
          empty(true),
          painted(false)
    {
        layer->set_name(name);
        layer->set_extent(extent);
        layer->set_version(2);
    }

    void make_painted()
    {
        painted = true;
    }

    void add_feature(std::unique_ptr<vector_tile::Tile_Feature> & vt_feature, 
                     mapnik::feature_impl const& mapnik_feature)

    {
        // Feature id should be unique from mapnik so we should comply with
        // the following wording of the specification:
        // "the value of the (feature) id SHOULD be unique among the features of the parent layer."

        // note that feature.id is signed int64_t so we are casting.
        vt_feature->set_id(static_cast<std::uint64_t>(mapnik_feature.id()));

        // Mapnik features can not have more then one value for
        // a single key. Therefore, we do not have to check if
        // key already exists in the feature as we insert each
        // key value pair into the feature. 
        feature_kv_iterator itr = mapnik_feature.begin();
        feature_kv_iterator end = mapnik_feature.end();
        for (; itr!=end; ++itr)
        {
            std::string const& name = std::get<0>(*itr);
            mapnik::value const& val = std::get<1>(*itr);
            if (!val.is_null())
            {
                // Insert the key index
                keys_container::const_iterator key_itr = keys.find(name);
                if (key_itr == keys.end())
                {
                    // The key doesn't exist yet in the dictionary.
                    layer->add_keys(name.c_str(), name.length());
                    size_t index = keys.size();
                    keys.emplace(name, index);
                    vt_feature->add_tags(index);
                }
                else
                {
                    vt_feature->add_tags(key_itr->second);
                }

                // Insert the value index
                values_container::const_iterator val_itr = values.find(val);
                if (val_itr == values.end())
                {
                    // The value doesn't exist yet in the dictionary.
                    detail::to_tile_value visitor(layer->add_values());
                    mapnik::util::apply_visitor(visitor, val);
                    size_t index = values.size();
                    values.emplace(val, index);
                    vt_feature->add_tags(index);
                }
                else
                {
                    vt_feature->add_tags(val_itr->second);
                }
            }
        }
        layer->mutable_features()->AddAllocated(vt_feature.release());
        empty = false;
    }
};

struct layer_builder_pbf
{
    typedef std::map<std::string, unsigned> keys_container;
    typedef std::unordered_map<mapnik::value, unsigned> values_container;

    keys_container keys;
    values_container values;
    protozero::pbf_writer layer_writer;
    bool empty;
    bool painted;

    layer_builder_pbf(std::string const & name, std::uint32_t extent, std::string & layer_buffer)
        : keys(),
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

    void add_feature(std::string & feature_str,
                     protozero::pbf_writer & feature_writer, 
                     mapnik::feature_impl const& mapnik_feature)

    {
        // Feature id should be unique from mapnik so we should comply with
        // the following wording of the specification:
        // "the value of the (feature) id SHOULD be unique among the features of the parent layer."

        // note that feature.id is signed int64_t so we are casting.
        feature_writer.add_uint64(Feature_Encoding::ID, static_cast<std::uint64_t>(mapnik_feature.id()));
        
        {
            protozero::packed_field_uint32 tags(feature_writer, Feature_Encoding::TAGS);
            // Mapnik features can not have more then one value for
            // a single key. Therefore, we do not have to check if
            // key already exists in the feature as we insert each
            // key value pair into the feature. 
            feature_kv_iterator itr = mapnik_feature.begin();
            feature_kv_iterator end = mapnik_feature.end();
            for (; itr!=end; ++itr)
            {
                std::string const& name = std::get<0>(*itr);
                mapnik::value const& val = std::get<1>(*itr);
                if (!val.is_null())
                {
                    // Insert the key index
                    keys_container::const_iterator key_itr = keys.find(name);
                    if (key_itr == keys.end())
                    {
                        // The key doesn't exist yet in the dictionary.
                        layer_writer.add_string(Layer_Encoding::KEYS, name);
                        size_t index = keys.size();
                        keys.emplace(name, index);
                        tags.add_element(index);
                    }
                    else
                    {
                        tags.add_element(key_itr->second);
                    }

                    // Insert the value index
                    values_container::const_iterator val_itr = values.find(val);
                    if (val_itr == values.end())
                    {
                        // The value doesn't exist yet in the dictionary.
                        {
                            std::string value_str;
                            protozero::pbf_writer value_writer(value_str);
                            detail::to_tile_value_pbf visitor(value_writer);
                            mapnik::util::apply_visitor(visitor, val);
                            layer_writer.add_message(Layer_Encoding::VALUES, value_str);
                        }
                        size_t index = values.size();
                        values.emplace(val, index);
                        tags.add_element(index);
                    }
                    else
                    {
                        tags.add_element(val_itr->second);
                    }
                }
            }
        }
        layer_writer.add_message(Layer_Encoding::FEATURES, feature_str);
        empty = false;
    }
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

    void build(layer_builder const& builder)
    {
        empty_ = builder.empty;
        painted_ = builder.painted;
        builder.tile.SerializeToString(&buffer_);
    }
    
    void build(layer_builder_pbf const& builder)
    {
        empty_ = builder.empty;
        painted_ = builder.painted;
    }

};

} // end ns vector_tile_impl

} // end ns mapnik

#endif // __MAPNIK_VECTOR_TILE_LAYER_H__
