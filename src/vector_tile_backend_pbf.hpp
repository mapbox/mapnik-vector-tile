#ifndef __MAPNIK_VECTOR_TILE_BACKEND_PBF_H__
#define __MAPNIK_VECTOR_TILE_BACKEND_PBF_H__

#include "mapnik3x_compatibility.hpp"
#include MAPNIK_VARIANT_INCLUDE

// mapnik
#include <mapnik/feature.hpp>
#include <mapnik/value_types.hpp>

// vector tile
#include "vector_tile.pb.h"
#include "vector_tile_geometry_encoder.hpp"

// boost
#include <boost/unordered_map.hpp>
#include <boost/foreach.hpp>

#include MAPNIK_VARIANT_INCLUDE

namespace mapnik { namespace vector {

    struct to_tile_value: public MAPNIK_STATIC_VISITOR<>
    {
    public:
        to_tile_value(tile_value * value):
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
        tile_value * value_;
    };

    struct backend_pbf
    {
        typedef std::map<std::string, unsigned> keys_container;
        typedef boost::unordered_map<mapnik::value, unsigned> values_container;
    private:
        tile & tile_;
        unsigned path_multiplier_;
        mutable tile_layer * current_layer_;
        mutable tile_feature * current_feature_;
        keys_container keys_;
        values_container values_;
        int32_t x_, y_;
    public:
        explicit backend_pbf(tile & _tile,
                             unsigned path_multiplier)
            : tile_(_tile),
              path_multiplier_(path_multiplier),
              current_layer_(NULL),
              current_feature_(NULL)
        {
        }

        void add_tile_feature_raster(std::string const& image_buffer)
        {
            if (current_feature_)
            {
                current_feature_->set_raster(image_buffer);
            }
        }

        void stop_tile_feature()
        {
            if (current_feature_)
            {
                if (current_feature_->geometry_size() == 0 && current_layer_)
                {
                    current_layer_->mutable_features()->RemoveLast();
                }
            }
        }

        void start_tile_feature(mapnik::feature_impl const& feature)
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
                std::string const& name = MAPNIK_GET<0>(*itr);
                mapnik::value const& val = MAPNIK_GET<1>(*itr);
                if (!val.is_null())
                {
                    // Insert the key index
                    keys_container::const_iterator key_itr = keys_.find(name);
                    if (key_itr == keys_.end())
                    {
                        // The key doesn't exist yet in the dictionary.
                        current_layer_->add_keys(name.c_str(), name.length());
                        size_t index = keys_.size();
                        keys_.insert(keys_container::value_type(name, index));
                        current_feature_->add_tags(index);
                    }
                    else
                    {
                        current_feature_->add_tags(key_itr->second);
                    }

                    // Insert the value index
                    values_container::const_iterator val_itr = values_.end();//values_.find(val);
                    if (val_itr == values_.end())
                    {
                        // The value doesn't exist yet in the dictionary.
                        to_tile_value visitor(current_layer_->add_values());
                        MAPNIK_APPLY_VISITOR(visitor, val.base());

                        size_t index = values_.size();
                        //values_.insert(values_container::value_type(val, index));
                        current_feature_->add_tags(index);
                    }
                    else
                    {
                        current_feature_->add_tags(val_itr->second);
                    }
                }
            }
        }

        void start_tile_layer(std::string const& name)
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

        inline void stop_tile_layer()
        {
            //std::cerr << "stop_tile_layer()" << std::endl;
        }

        template <typename T>
        inline unsigned add_path(T & path, unsigned tolerance, MAPNIK_GEOM_TYPE type)
        {
            if (current_feature_)
            {
                return encode_geometry(path,
                                       static_cast<tile_GeomType>(type),
                                       *current_feature_,
                                       x_,
                                       y_,
                                       tolerance,
                                       path_multiplier_);
            }
            return 0;
        }
    };

}} // end ns

#endif // __MAPNIK_VECTOR_TILE_BACKEND_PBF_H__
