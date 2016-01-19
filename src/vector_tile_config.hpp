#ifndef __MAPNIK_VECTOR_TILE_CONFIG_H__
#define __MAPNIK_VECTOR_TILE_CONFIG_H__

#if !defined(MAPNIK_VECTOR_TILE_LIBRARY)
#define MAPNIK_VECTOR_INLINE inline
#else
#define MAPNIK_VECTOR_INLINE
#endif

namespace mapnik
{

namespace vector_tile_impl
{

enum TileEncoding {
    LAYERS = 3
};

enum LayerEncoding {
    VERSION = 15,
    NAME = 1,
    FEATURES = 2,
    KEYS = 3,
    VALUES = 4,
    EXTENT = 5
};

enum FeatureEncoding {
    ID = 1,
    TAGS = 2,
    TYPE = 3,
    GEOMETRY = 4,
    RASTER = 5
};

enum ValueEncoding {
    STRING = 1,
    FLOAT = 2,
    DOUBLE = 3,
    INT = 4,
    UINT = 5,
    SINT = 6,
    BOOL = 7
};

} // end ns vector_tile_impl

} // end ns mapnik

#endif // __MAPNIK_VECTOR_TILE_CONFIG_H__
