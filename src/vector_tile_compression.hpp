#ifndef __MAPNIK_VECTOR_TILE_COMPRESSION_H__
#define __MAPNIK_VECTOR_TILE_COMPRESSION_H__

#include <string>
#include "vector_tile_config.hpp"

namespace mapnik { namespace vector_tile_impl {

inline bool is_zlib_compressed(std::string const& data)
{
    return data.size() > 2 && static_cast<uint8_t>(data[0]) == 0x78 && static_cast<uint8_t>(data[1]) == 0x9C;
}

inline bool is_gzip_compressed(std::string const& data)
{
    return data.size() > 2 && static_cast<uint8_t>(data[0]) == 0x1F && static_cast<uint8_t>(data[1]) == 0x8B;
}

// decodes both zlib and gzip
// http://stackoverflow.com/a/1838702/2333354
MAPNIK_VECTOR_INLINE void zlib_decompress(std::string const& input, std::string & output);
MAPNIK_VECTOR_INLINE void zlib_compress(std::string const& input, std::string & output, bool gzip=true);

}} // end ns

#if !defined(MAPNIK_VECTOR_TILE_LIBRARY)
#include "vector_tile_compression.ipp"
#endif

#endif // __MAPNIK_VECTOR_TILE_COMPRESSION_H__
