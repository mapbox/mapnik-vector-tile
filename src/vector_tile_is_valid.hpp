#ifndef __MAPNIK_VECTOR_TILE_IS_VALID_H__
#define __MAPNIK_VECTOR_TILE_IS_VALID_H__

// mapnik protzero parsing configuration
#include "vector_tile_config.hpp"

//protozero
#include <protozero/pbf_reader.hpp>

// std
#include <sstream>

namespace mapnik
{

namespace vector_tile_impl
{

enum validity_error : std::uint8_t
{
    TILE_REPEATED_LAYER_NAMES = 0,
    TILE_HAS_UNKNOWN_TAG,
    LAYER_HAS_NO_NAME,
    LAYER_HAS_NO_EXTENT,
    LAYER_HAS_NO_FEATURES,
    LAYER_HAS_INVALID_VERSION,
    LAYER_HAS_UNKNOWN_TAG,
    VALUE_MULTIPLE_VALUES,
    VALUE_NO_VALUE,
    VALUE_HAS_UNKNOWN_TAG,
    FEATURE_IS_EMPTY,
    FEATURE_MULTIPLE_GEOM,
    FEATURE_NO_GEOM_TYPE,
    FEATURE_HAS_INVALID_GEOM_TYPE,
    FEATURE_HAS_UNKNOWN_TAG,
    INVALID_PBF_BUFFER

};

std::string validity_error_to_string(validity_error err)
{
    switch (err)
    {
        case TILE_REPEATED_LAYER_NAMES:
            return "Vector Tile message has two or more layers with the same name";
        case TILE_HAS_UNKNOWN_TAG:
            return "Vector Tile message has an unknown tag";
        case LAYER_HAS_NO_NAME:
            return "Vector Tile Layer message has no name";
        case LAYER_HAS_NO_EXTENT:
            return "Vector Tile Layer message has no extent";
        case LAYER_HAS_NO_FEATURES:
            return "Vector Tile Layer message has no features";
        case LAYER_HAS_INVALID_VERSION:
            return "Vector Tile Layer message has an invalid version";
        case LAYER_HAS_UNKNOWN_TAG:
            return "Vector Tile Layer message has an unknown tag";
        case VALUE_MULTIPLE_VALUES:
            return "Vector Tile Value message contains more then one values";
        case VALUE_NO_VALUE:
            return "Vector Tile Value message contains no values";
        case VALUE_HAS_UNKNOWN_TAG:
            return "Vector Tile Value message has an unknown tag";
        case FEATURE_IS_EMPTY:
            return "Vector Tile Feature message has no geometry";
        case FEATURE_MULTIPLE_GEOM:
            return "Vector Tile Feature message has multiple geometries";
        case FEATURE_NO_GEOM_TYPE:
            return "Vector Tile Feature message is missing a geometry type";
        case FEATURE_HAS_INVALID_GEOM_TYPE:
            return "Vector Tile Feature message has an invalid geometry type";
        case FEATURE_HAS_UNKNOWN_TAG:
            return "Vector Tile Feature message has an unknown tag";
        case INVALID_PBF_BUFFER:
            return "Buffer is not encoded as a valid PBF";
        default:
            return "UNKNOWN ERROR";
    }
}

void validity_error_to_string(std::set<validity_error> & errors, std::string & out)
{
    if (errors.empty())
    {
        return;
    }
    std::ostringstream err;
    err << "Vector Tile Validity Errors Found:" << std::endl;
    for (auto e : errors)
    {
        err << " - " << validity_error_to_string(e) << std::endl;
    }
    out.append(err.str());
}

void feature_is_valid(protozero::pbf_reader & feature_msg, std::set<validity_error> & errors)
{
    bool has_geom = false;
    bool has_raster = false;
    bool has_type = false;
    while (feature_msg.next())
    {
        switch (feature_msg.tag())
        {
            case FeatureEncoding::ID: // id
                feature_msg.skip();
                break;
            case FeatureEncoding::TAGS: // tags
                feature_msg.get_packed_uint32();
                break;
            case FeatureEncoding::TYPE: // geom type
                {
                    std::int32_t type = feature_msg.get_enum();
                    if (type <= 0 || type > 3)
                    {
                        errors.insert(FEATURE_HAS_INVALID_GEOM_TYPE);
                    }
                    has_type = true;
                }
                break;
            case FeatureEncoding::GEOMETRY: // geometry
                if (has_geom || has_raster)
                {
                    errors.insert(FEATURE_MULTIPLE_GEOM);
                }
                has_geom = true;
                feature_msg.get_packed_uint32();
                break;
            case FeatureEncoding::RASTER: // raster
                if (has_geom || has_raster)
                {
                    errors.insert(FEATURE_MULTIPLE_GEOM);
                }
                has_raster = true;
                feature_msg.get_data();
                break;
            default:
                errors.insert(FEATURE_HAS_UNKNOWN_TAG);
                feature_msg.skip();
                break;
        }        
    }
    if (!has_geom && !has_raster)
    {
        errors.insert(FEATURE_IS_EMPTY);
    }
    if (has_geom && !has_type)
    {
        errors.insert(FEATURE_NO_GEOM_TYPE);
    }
}

void value_is_valid(protozero::pbf_reader & value_msg, std::set<validity_error> & errors)
{
    bool has_value = false;
    while (value_msg.next())
    {
        switch (value_msg.tag())
        {
            case ValueEncoding::STRING:
            case ValueEncoding::FLOAT:
            case ValueEncoding::DOUBLE:
            case ValueEncoding::INT:
            case ValueEncoding::UINT:
            case ValueEncoding::SINT:
            case ValueEncoding::BOOL:
                if (has_value)
                {
                    errors.insert(VALUE_MULTIPLE_VALUES);
                }
                has_value = true;
                value_msg.skip();
                break;
            default:
                errors.insert(VALUE_HAS_UNKNOWN_TAG);
                value_msg.skip();
                break;
        }
    }
    if (!has_value)
    {
        errors.insert(VALUE_NO_VALUE);
    }
}

void layer_is_valid(protozero::pbf_reader & layer_msg, 
                    std::set<validity_error> & errors, 
                    std::string & layer_name,
                    std::uint32_t & version)
{
    bool contains_a_feature = false;
    bool contains_a_name = false;
    bool contains_an_extent = false;
    try
    {
        while (layer_msg.next())
        {
            switch (layer_msg.tag())
            {
                case LayerEncoding::NAME: // name
                    contains_a_name = true;
                    layer_name = layer_msg.get_string();
                    break;
                case LayerEncoding::FEATURES:
                    {
                        contains_a_feature = true;
                        protozero::pbf_reader feature_msg = layer_msg.get_message();
                        feature_is_valid(feature_msg, errors);
                    }
                    break;
                case LayerEncoding::KEYS: // keys
                    layer_msg.skip();
                    break;
                case LayerEncoding::VALUES: // value
                    {
                        protozero::pbf_reader value_msg = layer_msg.get_message();
                        value_is_valid(value_msg, errors);
                    }
                    break;
                case LayerEncoding::EXTENT: // extent
                    contains_an_extent = true;
                    layer_msg.skip();
                    break;
                case LayerEncoding::VERSION:
                    version = layer_msg.get_uint32();
                    break;
                default:
                    errors.insert(LAYER_HAS_UNKNOWN_TAG);
                    layer_msg.skip();
                    break;
            }
        }
    }
    catch (...)
    {
        errors.insert(INVALID_PBF_BUFFER);   
    }
    if (!contains_a_name)
    {
        errors.insert(LAYER_HAS_NO_NAME);
    }
    if (version > 2 || version == 0)
    {
        errors.insert(LAYER_HAS_INVALID_VERSION);
    }
    if (!contains_an_extent && version != 1)
    {
        errors.insert(LAYER_HAS_NO_EXTENT);
    }
    if (!contains_a_feature && version != 1)
    {
        errors.insert(LAYER_HAS_NO_FEATURES);
    }
}

// Check some common things that could be wrong with a tile
// does not check all items in line with the v2 spec. For
// example does does not check validity of geometry. It also does not 
// verify that all feature attributes are valid.
void tile_is_valid(protozero::pbf_reader & tile_msg, std::set<validity_error> & errors)
{
    std::set<std::string> layer_names_set;
    try
    {
        while (tile_msg.next())
        {
            switch (tile_msg.tag())
            {
                case TileEncoding::LAYERS:
                    {
                        protozero::pbf_reader layer_msg = tile_msg.get_message();
                        std::string layer_name;
                        std::uint32_t version = 1;
                        layer_is_valid(layer_msg, errors, layer_name, version);
                        auto p = layer_names_set.insert(layer_name);
                        if (!p.second)
                        {
                            errors.insert(TILE_REPEATED_LAYER_NAMES);
                        }
                    }
                    break;
                default:
                    errors.insert(TILE_HAS_UNKNOWN_TAG);
                    tile_msg.skip();
                    break;
            }
        }
    }
    catch (...)
    {
        errors.insert(INVALID_PBF_BUFFER);
    }
}

void tile_is_valid(std::string const& tile, std::set<validity_error> & errors)
{
    protozero::pbf_reader tile_msg(tile);
    tile_is_valid(tile_msg, errors);
}

} // end ns vector_tile_impl

} // end ns mapnik

#endif // __MAPNIK_VECTOR_TILE_IS_VALID_H__
