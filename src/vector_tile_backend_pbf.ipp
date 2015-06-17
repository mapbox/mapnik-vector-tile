// mapnik
#include <mapnik/version.hpp>
#include <mapnik/feature.hpp>
#include <mapnik/value_types.hpp>
#include <mapnik/util/variant.hpp>

// vector tile
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include "vector_tile.pb.h"
#pragma GCC diagnostic pop

#include "vector_tile_geometry_encoder.hpp"

#include <unordered_map>

namespace mapnik { namespace vector_tile_impl {

struct to_tile_value: public mapnik::util::static_visitor<>
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

backend_pbf::backend_pbf(vector_tile::Tile & _tile,
                     unsigned path_multiplier)
    : tile_(_tile),
      path_multiplier_(path_multiplier),
      current_layer_(NULL),
      x_(0),
      y_(0),
      current_feature_(NULL)
{
}

void backend_pbf::add_tile_feature_raster(std::string const& image_buffer)
{
    if (current_feature_)
    {
        current_feature_->set_raster(image_buffer);
    }
}

void backend_pbf::stop_tile_feature()
{
    if (current_feature_)
    {
        if (current_feature_->geometry_size() == 0 && current_layer_)
        {
            current_layer_->mutable_features()->RemoveLast();
        }
    }
}

void backend_pbf::start_tile_feature(mapnik::feature_impl const& feature)
{
    if (current_layer_)
    {
        current_feature_ = current_layer_->add_features();
        x_ = y_ = 0;

        // TODO - encode as sint64: (n << 1) ^ ( n >> 63)
        // test current behavior with negative numbers
        current_feature_->set_id(feature.id());

        feature_kv_iterator itr = feature.begin();
        feature_kv_iterator end = feature.end();
        for ( ;itr!=end; ++itr)
        {
            std::string const& name = std::get<0>(*itr);
            mapnik::value const& val = std::get<1>(*itr);
            if (!val.is_null())
            {
                // Insert the key index
                keys_container::const_iterator key_itr = keys_.find(name);
                if (key_itr == keys_.end())
                {
                    // The key doesn't exist yet in the dictionary.
                    current_layer_->add_keys(name.c_str(), name.length());
                    size_t index = keys_.size();
                    keys_.emplace(name, index);
                    current_feature_->add_tags(index);
                }
                else
                {
                    current_feature_->add_tags(key_itr->second);
                }

                // Insert the value index
                values_container::const_iterator val_itr = values_.find(val);
                if (val_itr == values_.end())
                {
                    // The value doesn't exist yet in the dictionary.
                    to_tile_value visitor(current_layer_->add_values());
                    mapnik::util::apply_visitor(visitor, val);
                    size_t index = values_.size();
                    values_.emplace(val, index);
                    current_feature_->add_tags(index);
                }
                else
                {
                    current_feature_->add_tags(val_itr->second);
                }
            }
        }
    }
}

void backend_pbf::start_tile_layer(std::string const& name)
{
    // Key/value dictionary is per-layer.
    keys_.clear();
    values_.clear();

    current_layer_ = tile_.add_layers();
    current_layer_->set_name(name);
    current_layer_->set_version(1);

    // We currently use path_multiplier as a factor to scale the coordinates.
    // Eventually, we should replace this with the extent specifying the
    // bounding box in both dimensions. E.g. an extent of 4096 means that
    // the coordinates encoded in this tile should be visible in the range
    // from 0..4095.
    current_layer_->set_extent(256 * path_multiplier_);
}

}} // end ns
