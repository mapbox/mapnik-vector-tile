#ifndef __MAPNIK_VECTOR_TILE_BACKEND_PBF_H__
#define __MAPNIK_VECTOR_TILE_BACKEND_PBF_H__

// vector tile
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include "vector_tile.pb.h"
#pragma GCC diagnostic pop

#include "vector_tile_config.hpp"
#include "vector_tile_geometry_encoder.hpp"
#include <mapnik/value.hpp>
#include <mapnik/geometry.hpp>
#include <unordered_map>

namespace mapnik
{
class feature_impl;
}

namespace mapnik { namespace vector_tile_impl {

    struct backend_pbf
    {
        typedef std::map<std::string, unsigned> keys_container;
        typedef std::unordered_map<mapnik::value, unsigned> values_container;
    private:
        vector_tile::Tile & tile_;
        unsigned path_multiplier_;
        mutable vector_tile::Tile_Layer * current_layer_;
        keys_container keys_;
        values_container values_;
        int32_t x_, y_;
    public:
        mutable vector_tile::Tile_Feature * current_feature_;
        MAPNIK_VECTOR_INLINE explicit backend_pbf(vector_tile::Tile & _tile,
                             unsigned path_multiplier);

        MAPNIK_VECTOR_INLINE void add_tile_feature_raster(std::string const& image_buffer);
        MAPNIK_VECTOR_INLINE void stop_tile_feature();
        MAPNIK_VECTOR_INLINE void start_tile_feature(mapnik::feature_impl const& feature);
        MAPNIK_VECTOR_INLINE void start_tile_layer(std::string const& name);
        MAPNIK_VECTOR_INLINE unsigned get_path_multiplier()
        {
            return path_multiplier_;
        }
        inline void stop_tile_layer() {}

        template <typename T>
        inline bool add_path(T const& path)
        {
            if (current_feature_)
            {
                return encode_geometry(path,
                                       *current_feature_,
                                       x_,
                                       y_);
            }
            // no path was added return false
            return false;
        }
    };

}} // end ns

#if !defined(MAPNIK_VECTOR_TILE_LIBRARY)
#include "vector_tile_backend_pbf.ipp"
#endif

#endif // __MAPNIK_VECTOR_TILE_BACKEND_PBF_H__
