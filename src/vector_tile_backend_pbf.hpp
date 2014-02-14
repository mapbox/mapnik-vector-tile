#ifndef __MAPNIK_VECTOR_TILE_BACKEND_PBF_H__
#define __MAPNIK_VECTOR_TILE_BACKEND_PBF_H__

// mapnik
#include <mapnik/feature.hpp>
#include <mapnik/version.hpp>
#include <mapnik/value_types.hpp>

// vector tile
#include "vector_tile.pb.h"

// boost
#include <boost/unordered_map.hpp>
#include <boost/foreach.hpp>

namespace mapnik { namespace vector {

    struct to_tile_value: public boost::static_visitor<>
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
                std::string const& name = boost::get<0>(*itr);
                mapnik::value const& val = boost::get<1>(*itr);
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
                    values_container::const_iterator val_itr = values_.find(val);
                    if (val_itr == values_.end())
                    {
                        // The value doesn't exist yet in the dictionary.
                        to_tile_value visitor(current_layer_->add_values());
                        boost::apply_visitor(visitor, val.base());

                        size_t index = values_.size();
                        values_.insert(values_container::value_type(val, index));
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

        void stop_tile_layer()
        {
            // NOTE: we intentionally do not remove layers without features
            // since the re-rendering logic expects a layer entry no matter what
            //std::cerr << "stop_tile_layer()" << std::endl;
        }

        template <typename T>
        unsigned add_path(T & path, unsigned tolerance, mapnik::eGeomType type)
        {
            unsigned count = 0;

            if (current_feature_)
            {
                path.rewind(0);
                current_feature_->set_type(mapnik::vector::tile_GeomType(type));

                vertex2d vtx(vertex2d::no_init);
                int cmd = -1;
                int cmd_idx = -1;
                const int cmd_bits = 3;
                unsigned length = 0;
                bool skipped_last = false;
                int32_t cur_x = 0;
                int32_t cur_y = 0;

                // See vector_tile.proto for a description of how vertex command
                // encoding works.
                while ((vtx.cmd = path.vertex(&vtx.x, &vtx.y)) != SEG_END)
                {
                    if (static_cast<int>(vtx.cmd) != cmd)
                    {
                        if (cmd_idx >= 0)
                        {
                            // Encode the previous length/command value.
                            current_feature_->set_geometry(cmd_idx, (length << cmd_bits) | (cmd & ((1 << cmd_bits) - 1)));
                        }
                        cmd = static_cast<int>(vtx.cmd);
                        length = 0;
                        cmd_idx = current_feature_->geometry_size();
                        current_feature_->add_geometry(0); // placeholder added in first pass
                    }

                    if (cmd == SEG_MOVETO || cmd == SEG_LINETO)
                    {
                        // Compute delta to the previous coordinate.
                        cur_x = static_cast<int32_t>(std::floor((vtx.x * path_multiplier_) + 0.5));
                        cur_y = static_cast<int32_t>(std::floor((vtx.y * path_multiplier_) + 0.5));
                        int32_t dx = cur_x - x_;
                        int32_t dy = cur_y - y_;

                        // Omit movements that are no-ops.
                        unsigned x_floor = static_cast<unsigned>(std::abs(dx));
                        unsigned y_floor = static_cast<unsigned>(std::abs(dy));
                        if (cmd == SEG_MOVETO ||
                            x_floor >= tolerance ||
                            y_floor >= tolerance ||
                            length == 0
                            )
                        {
                            // Manual zigzag encoding.
                            current_feature_->add_geometry((dx << 1) ^ (dx >> 31));
                            current_feature_->add_geometry((dy << 1) ^ (dy >> 31));
                            x_ = cur_x;
                            y_ = cur_y;
                            skipped_last = false;
                            ++length;
                        }
                        else
                        {
                            skipped_last = true;
                        }
                    }
                    else if (cmd == SEG_CLOSE)
                    {
                        ++length;
                    }
                    else
                    {
                        std::stringstream msg;
                        msg << "Unknown command type (backend_pbf): "
                            << cmd;
                        throw std::runtime_error(msg.str());
                    }

                    ++count;
                }

                if (skipped_last && length > 0)
                {
                    int32_t dx = cur_x - x_;
                    int32_t dy = cur_y - y_;
                    current_feature_->add_geometry((dx << 1) ^ (dx >> 31));
                    current_feature_->add_geometry((dy << 1) ^ (dy >> 31));
                    ++length;
                }

                // Update the last length/command value.
                if (cmd_idx >= 0)
                {
                    current_feature_->set_geometry(cmd_idx, (length << cmd_bits) | (cmd & ((1 << cmd_bits) - 1)));
                }
            }
            return count;
        }
    };

    }} // end ns

#endif // __MAPNIK_VECTOR_TILE_BACKEND_PBF_H__
