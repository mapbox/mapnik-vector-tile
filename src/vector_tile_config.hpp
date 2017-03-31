#pragma once

#if !defined(MAPNIK_VECTOR_TILE_LIBRARY)
#define MAPNIK_VECTOR_INLINE inline
#else
#define MAPNIK_VECTOR_INLINE
#endif

#include <protozero/types.hpp>

#define VT_LEGACY_IMAGE_SIZE 256.0

namespace mapnik
{

namespace vector_tile_impl
{

enum Tile_Encoding : protozero::pbf_tag_type
{
    LAYERS = 3
};

enum Layer_Encoding : protozero::pbf_tag_type
{
    VERSION = 15,
    NAME = 1,
    FEATURES = 2,
    KEYS = 3,
    VALUES = 4,
    EXTENT = 5
};

enum Feature_Encoding : protozero::pbf_tag_type
{
    ID = 1,
    TAGS = 2,
    TYPE = 3,
    GEOMETRY = 4,
    RASTER = 5
};

enum Value_Encoding : protozero::pbf_tag_type
{
    STRING = 1,
    FLOAT = 2,
    DOUBLE = 3,
    INT = 4,
    UINT = 5,
    SINT = 6,
    BOOL = 7
};

enum Geometry_Type : std::uint8_t
{
    UNKNOWN = 0,
    POINT = 1,
    LINESTRING = 2,
    POLYGON = 3
};

enum polygon_fill_type : std::uint8_t {
    even_odd_fill = 0, 
    non_zero_fill, 
    positive_fill, 
    negative_fill,
    polygon_fill_type_max
};

} // end ns vector_tile_impl

} // end ns mapnik
