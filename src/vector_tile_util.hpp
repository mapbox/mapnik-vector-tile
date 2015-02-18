#ifndef __MAPNIK_VECTOR_TILE_UTIL_H__
#define __MAPNIK_VECTOR_TILE_UTIL_H__

#include <string>

namespace vector_tile {
    class Tile;
}

namespace mapnik { namespace vector_tile_impl {

    bool is_solid_extent(vector_tile::Tile const& tile, std::string & key);

}}

#if !defined(MAPNIK_VECTOR_TILE_LIBRARY)
#include "vector_tile_util.ipp"
#endif

#endif // __MAPNIK_VECTOR_TILE_UTIL_H__
