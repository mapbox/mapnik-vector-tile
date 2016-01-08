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

namespace mapnik
{

namespace vector_tile_impl
{

namespace detail
{

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

} // end ns detail

tile_layer::tile_layer()
    : layer_(),
      empty_(true),
      painted_(false) {}

tile_layer::tile_layer(std::string const & name, std::uint32_t extent)
    : layer_(new vector_tile::Tile_Layer()),
      empty_(true),
      painted_(false)
{
    layer_->set_name(name);
    layer_->set_extent(extent);
    layer_->set_version(2);
}

MAPNIK_VECTOR_INLINE void tile_layer::add_feature(std::unique_ptr<vector_tile::Tile_Feature> & vt_feature, 
                                                  mapnik::feature_impl const& mapnik_feature)

{
    if (!layer_)
    {
        throw std::runtime_error("Attempting to add feature to a tile layer after the pointer has been released.");
    }
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
            keys_container::const_iterator key_itr = keys_.find(name);
            if (key_itr == keys_.end())
            {
                // The key doesn't exist yet in the dictionary.
                layer_->add_keys(name.c_str(), name.length());
                size_t index = keys_.size();
                keys_.emplace(name, index);
                vt_feature->add_tags(index);
            }
            else
            {
                vt_feature->add_tags(key_itr->second);
            }

            // Insert the value index
            values_container::const_iterator val_itr = values_.find(val);
            if (val_itr == values_.end())
            {
                // The value doesn't exist yet in the dictionary.
                detail::to_tile_value visitor(layer_->add_values());
                mapnik::util::apply_visitor(visitor, val);
                size_t index = values_.size();
                values_.emplace(val, index);
                vt_feature->add_tags(index);
            }
            else
            {
                vt_feature->add_tags(val_itr->second);
            }
        }
    }
    layer_->mutable_features()->AddAllocated(vt_feature.release());
    empty_ = false;
}

} // end ns vector_tile_impl

} // end ns mapnik
