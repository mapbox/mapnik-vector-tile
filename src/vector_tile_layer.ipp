// mapnik
#include <mapnik/feature.hpp>
#include <mapnik/util/variant.hpp>
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

namespace detail
{

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

MAPNIK_VECTOR_INLINE void layer_builder_pbf::add_feature(protozero::pbf_writer & feature_writer, 
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

} // end ns vector_tile_impl

} // end ns mapnik
