// mapnik-vector-tile
#include "vector_tile_layer.hpp"
#include "vector_tile_geometry_encoder.hpp"

// mapnik
#include <mapnik/feature.hpp>
#include <mapnik/geometry.hpp>
#include <mapnik/util/variant.hpp>

// libprotobuf
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include "vector_tile.pb.h"
#pragma GCC diagnostic pop

// std
#include <memory>

namespace mapnik
{

namespace vector_tile_impl
{

namespace detail
{

struct geometry_to_feature_visitor
{
    mapnik::feature_impl const& mapnik_feature_;
    tile_layer & layer_;

    geometry_to_feature_visitor(mapnik::feature_impl const& mapnik_feature,
                                tile_layer & layer)
        : mapnik_feature_(mapnik_feature),
          layer_(layer) {}

    void operator() (mapnik::geometry::geometry_empty const&)
    {
    }

    template <typename T>
    void operator() (T const& geom)
    {
        geometry_to_feature(geom, mapnik_feature_, layer_);
    }

    void operator() (mapnik::geometry::geometry_collection<std::int64_t> const& collection)
    {
        for (auto & g : collection)
        {
            mapnik::util::apply_visitor((*this), g);
        }
    }
};

} // end ns detail

template <typename T>
MAPNIK_VECTOR_INLINE void geometry_to_feature(T const& geom,
                                              mapnik::feature_impl const& mapnik_feature,
                                              tile_layer & layer)
{
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::unique_ptr<vector_tile::Tile_Feature> vt_feature(new vector_tile::Tile_Feature());
    if (encode_geometry(geom, *vt_feature, x, y))
    {
        // Releasing the pointer is important here because the layer will take over ownership!
        layer.add_feature(vt_feature, mapnik_feature);
    }
}

MAPNIK_VECTOR_INLINE void geometry_to_feature(mapnik::geometry::geometry<std::int64_t> const& geom,
                                              mapnik::feature_impl const& mapnik_feature,
                                              tile_layer & layer)
{
    detail::geometry_to_feature_visitor visitor(mapnik_feature, layer);
    mapnik::util::apply_visitor(visitor, geom);
}

MAPNIK_VECTOR_INLINE void raster_to_feature(std::string const& buffer,
                                            mapnik::feature_impl const& mapnik_feature,
                                            tile_layer & layer)
{
    std::unique_ptr<vector_tile::Tile_Feature> vt_feature(new vector_tile::Tile_Feature());
    vt_feature->set_raster(buffer);
    // Releasing the pointer is important here because the layer will take over ownership!
    layer.add_feature(vt_feature, mapnik_feature);
}

} // end ns vector_tile_impl

} // end ns mapnik
