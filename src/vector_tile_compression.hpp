#pragma once

// mapnik-vector-tile
#include "vector_tile_config.hpp"

// zlib
#include <zlib.h>

// std
#include <string>

namespace mapnik 
{
 
namespace vector_tile_impl 
{

inline bool is_zlib_compressed(const char * data, std::size_t size)
{
    return size > 2 && 
           static_cast<uint8_t>(data[0]) == 0x78 && 
           (
               static_cast<uint8_t>(data[1]) == 0x9C || 
               static_cast<uint8_t>(data[1]) == 0x01 || 
               static_cast<uint8_t>(data[1]) == 0xDA || 
               static_cast<uint8_t>(data[1]) == 0x5E
           );
}

inline bool is_zlib_compressed(std::string const& data)
{
    return data.size() > 2 && 
           static_cast<uint8_t>(data[0]) == 0x78 && 
           (
               static_cast<uint8_t>(data[1]) == 0x9C || 
               static_cast<uint8_t>(data[1]) == 0x01 || 
               static_cast<uint8_t>(data[1]) == 0xDA || 
               static_cast<uint8_t>(data[1]) == 0x5E
           );
}

inline bool is_gzip_compressed(const char * data, std::size_t size)
{
    return size > 2 && static_cast<uint8_t>(data[0]) == 0x1F && static_cast<uint8_t>(data[1]) == 0x8B;
}

inline bool is_gzip_compressed(std::string const& data)
{
    return data.size() > 2 && static_cast<uint8_t>(data[0]) == 0x1F && static_cast<uint8_t>(data[1]) == 0x8B;
}

// decodes both zlib and gzip
// http://stackoverflow.com/a/1838702/2333354
MAPNIK_VECTOR_INLINE void zlib_decompress(std::string const& input, 
                                          std::string & output);

MAPNIK_VECTOR_INLINE void zlib_compress(std::string const& input, 
                                        std::string & output, 
                                        bool gzip=true, 
                                        int level=Z_DEFAULT_COMPRESSION, 
                                        int strategy=Z_DEFAULT_STRATEGY);

MAPNIK_VECTOR_INLINE void zlib_decompress(const char * data, 
                                          std::size_t size, 
                                          std::string & output);

MAPNIK_VECTOR_INLINE void zlib_compress(const char * data, 
                                        std::size_t size, 
                                        std::string & output, 
                                        bool gzip=true, 
                                        int level=Z_DEFAULT_COMPRESSION, 
                                        int strategy=Z_DEFAULT_STRATEGY);

} // end ns vector_tile_impl

} // end ns mapnik

#if !defined(MAPNIK_VECTOR_TILE_LIBRARY)
#include "vector_tile_compression.ipp"
#endif
