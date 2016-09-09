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
    TILE_HAS_DIFFERENT_VERSIONS,
    LAYER_HAS_NO_NAME,
    LAYER_HAS_MULTIPLE_NAME,
    LAYER_HAS_NO_EXTENT,
    LAYER_HAS_MULTIPLE_EXTENT,
    LAYER_HAS_MULTIPLE_VERSION,
    LAYER_HAS_NO_FEATURES,
    LAYER_HAS_UNSUPPORTED_VERSION,
    LAYER_HAS_RASTER_AND_VECTOR,
    LAYER_HAS_UNKNOWN_TAG,
    VALUE_MULTIPLE_VALUES,
    VALUE_NO_VALUE,
    VALUE_HAS_UNKNOWN_TAG,
    FEATURE_IS_EMPTY,
    FEATURE_MULTIPLE_ID,
    FEATURE_MULTIPLE_TAGS,
    FEATURE_MULTIPLE_GEOM,
    FEATURE_MULTIPLE_RASTER,
    FEATURE_RASTER_AND_GEOM,
    FEATURE_NO_GEOM_TYPE,
    FEATURE_HAS_INVALID_GEOM_TYPE,
    FEATURE_HAS_UNKNOWN_TAG,
    INVALID_PBF_BUFFER
};

inline std::string validity_error_to_string(validity_error err)
{
    switch (err)
    {
        case TILE_REPEATED_LAYER_NAMES:
            return "Vector Tile message has two or more layers with the same name";
        case TILE_HAS_UNKNOWN_TAG:
            return "Vector Tile message has an unknown tag";
        case TILE_HAS_DIFFERENT_VERSIONS:
            return "Vector Tile message has layers with different versions";
        case LAYER_HAS_NO_NAME:
            return "Vector Tile Layer message has no name";
        case LAYER_HAS_MULTIPLE_NAME:
            return "Vector Tile Layer message has multiple name tags";
        case LAYER_HAS_NO_EXTENT:
            return "Vector Tile Layer message has no extent";
        case LAYER_HAS_MULTIPLE_EXTENT:
            return "Vector Tile Layer message has multiple extent tags";
        case LAYER_HAS_NO_FEATURES:
            return "Vector Tile Layer message has no features";
        case LAYER_HAS_MULTIPLE_VERSION:
            return "Vector Tile Layer message has multiple version tags";
        case LAYER_HAS_UNSUPPORTED_VERSION:
            return "Vector Tile Layer message has an unsupported version";
        case LAYER_HAS_RASTER_AND_VECTOR:
            return "Vector Tile Layer contains raster and vector features";
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
        case FEATURE_MULTIPLE_ID:
            return "Vector Tile Feature message has multiple ids";
        case FEATURE_MULTIPLE_TAGS:
            return "Vector Tile Feature message has multiple repeated tags";
        case FEATURE_MULTIPLE_GEOM:
            return "Vector Tile Feature message has multiple geometries";
        case FEATURE_MULTIPLE_RASTER:
            return "Vector Tile Feature message has multiple rasters";
        case FEATURE_RASTER_AND_GEOM:
            return "Vector Tile Feature message has raster and geometry types";
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

inline void validity_error_to_string(std::set<validity_error> & errors, std::string & out)
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

inline void feature_is_valid(protozero::pbf_reader & feature_msg, 
                             std::set<validity_error> & errors,
                             std::uint64_t & point_feature_count,
                             std::uint64_t & line_feature_count,
                             std::uint64_t & polygon_feature_count,
                             std::uint64_t & unknown_feature_count,
                             std::uint64_t & raster_feature_count)
{
    bool has_geom = false;
    bool has_raster = false;
    bool has_type = false;
    bool has_id = false;
    bool has_tags = false;
    while (feature_msg.next())
    {
        switch (feature_msg.tag())
        {
            case Feature_Encoding::ID: // id
                if (has_id)
                {
                    errors.insert(FEATURE_MULTIPLE_ID);
                }
                has_id = true;
                feature_msg.skip();
                break;
            case Feature_Encoding::TAGS: // tags
                if (has_tags)
                {
                    errors.insert(FEATURE_MULTIPLE_TAGS);
                }
                has_tags = true;
                feature_msg.get_packed_uint32();
                break;
            case Feature_Encoding::TYPE: // geom type
                {
                    std::int32_t type = feature_msg.get_enum();
                    if (type == Geometry_Type::POINT)
                    {
                        ++point_feature_count;
                    }
                    else if (type == Geometry_Type::LINESTRING)
                    {
                        ++line_feature_count;
                    }
                    else if (type == Geometry_Type::POLYGON)
                    {
                        ++polygon_feature_count;
                    }
                    else if (type == Geometry_Type::UNKNOWN)
                    {
                        ++unknown_feature_count;
                    }
                    else
                    {
                        errors.insert(FEATURE_HAS_INVALID_GEOM_TYPE);
                    }
                    has_type = true;
                }
                break;
            case Feature_Encoding::GEOMETRY: // geometry
                if (has_geom)
                {
                    errors.insert(FEATURE_MULTIPLE_GEOM);
                }
                if (has_raster)
                {
                    errors.insert(FEATURE_RASTER_AND_GEOM);
                }
                has_geom = true;
                feature_msg.get_packed_uint32();
                break;
            case Feature_Encoding::RASTER: // raster
                if (has_geom)
                {
                    errors.insert(FEATURE_RASTER_AND_GEOM);
                }
                if (has_raster)
                {
                    errors.insert(FEATURE_MULTIPLE_RASTER);
                }
                has_raster = true;
                ++raster_feature_count;
                feature_msg.get_view();
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

inline void feature_is_valid(protozero::pbf_reader & feature_msg, 
                             std::set<validity_error> & errors)
{
    std::uint64_t point_feature_count = 0;
    std::uint64_t line_feature_count = 0;
    std::uint64_t polygon_feature_count = 0;
    std::uint64_t unknown_feature_count = 0;
    std::uint64_t raster_feature_count = 0;
    return feature_is_valid(feature_msg,
                            errors,
                            point_feature_count,
                            line_feature_count,
                            polygon_feature_count,
                            unknown_feature_count,
                            raster_feature_count);
}

inline void value_is_valid(protozero::pbf_reader & value_msg, std::set<validity_error> & errors)
{
    bool has_value = false;
    while (value_msg.next())
    {
        switch (value_msg.tag())
        {
            case Value_Encoding::STRING:
            case Value_Encoding::FLOAT:
            case Value_Encoding::DOUBLE:
            case Value_Encoding::INT:
            case Value_Encoding::UINT:
            case Value_Encoding::SINT:
            case Value_Encoding::BOOL:
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

inline void layer_is_valid(protozero::pbf_reader & layer_msg, 
                    std::set<validity_error> & errors, 
                    std::uint64_t & point_feature_count,
                    std::uint64_t & line_feature_count,
                    std::uint64_t & polygon_feature_count,
                    std::uint64_t & unknown_feature_count,
                    std::uint64_t & raster_feature_count)
{
    bool contains_a_feature = false;
    bool contains_a_name = false;
    bool contains_an_extent = false;
    bool contains_a_version = false;
    try
    {
        while (layer_msg.next())
        {
            switch (layer_msg.tag())
            {
                case Layer_Encoding::NAME: // name
                    if (contains_a_name)
                    {
                        errors.insert(LAYER_HAS_MULTIPLE_NAME);
                    }
                    contains_a_name = true;
                    layer_msg.skip();
                    break;
                case Layer_Encoding::FEATURES:
                    {
                        contains_a_feature = true;
                        protozero::pbf_reader feature_msg = layer_msg.get_message();
                        feature_is_valid(feature_msg, 
                                         errors,
                                         point_feature_count,
                                         line_feature_count,
                                         polygon_feature_count,
                                         unknown_feature_count,
                                         raster_feature_count);
                    }
                    break;
                case Layer_Encoding::KEYS: // keys
                    layer_msg.skip();
                    break;
                case Layer_Encoding::VALUES: // value
                    {
                        protozero::pbf_reader value_msg = layer_msg.get_message();
                        value_is_valid(value_msg, errors);
                    }
                    break;
                case Layer_Encoding::EXTENT: // extent
                    if (contains_an_extent)
                    {
                        errors.insert(LAYER_HAS_MULTIPLE_EXTENT);
                    }
                    contains_an_extent = true;
                    layer_msg.skip();
                    break;
                case Layer_Encoding::VERSION:
                    if (contains_a_version)
                    {
                        errors.insert(LAYER_HAS_MULTIPLE_VERSION);
                    }
                    contains_a_version = true;
                    layer_msg.skip();
                    break;
                default:
                    errors.insert(LAYER_HAS_UNKNOWN_TAG);
                    layer_msg.skip();
                    break;
            }
        }
    }
    catch (std::exception const&)
    {
        errors.insert(INVALID_PBF_BUFFER);   
    }
    if (!contains_a_name)
    {
        errors.insert(LAYER_HAS_NO_NAME);
    }
    if (!contains_an_extent)
    {
        errors.insert(LAYER_HAS_NO_EXTENT);
    }
    if (!contains_a_feature)
    {
        errors.insert(LAYER_HAS_NO_FEATURES);
    }
}

inline void layer_is_valid(protozero::pbf_reader & layer_msg, 
                           std::set<validity_error> & errors)
{

    std::uint64_t point_feature_count = 0;
    std::uint64_t line_feature_count = 0;
    std::uint64_t polygon_feature_count = 0;
    std::uint64_t unknown_feature_count = 0;
    std::uint64_t raster_feature_count = 0;
    return layer_is_valid(layer_msg, 
                          errors,
                          point_feature_count,
                          line_feature_count,
                          polygon_feature_count,
                          unknown_feature_count,
                          raster_feature_count);
}


} // end ns vector_tile_impl

} // end ns mapnik

#endif // __MAPNIK_VECTOR_TILE_IS_VALID_H__
