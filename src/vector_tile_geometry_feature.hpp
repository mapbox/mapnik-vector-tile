#ifndef __MAPNIK_VECTOR_TILE_GEOMETRY_FEATURE_H__
#define __MAPNIK_VECTOR_TILE_GEOMETRY_FEATURE_H__

// mapnik-vector-tile
#include "vector_tile_config.hpp"
#include "vector_tile_layer.hpp"
#include "vector_tile_geometry_encoder.hpp"
#include "vector_tile_geometry_encoder_pbf.hpp"

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

namespace mapnik
{

namespace vector_tile_impl
{

inline void raster_to_feature(std::string const& buffer,
                              mapnik::feature_impl const& mapnik_feature,
                              layer_builder & builder)
{
    std::unique_ptr<vector_tile::Tile_Feature> vt_feature(new vector_tile::Tile_Feature());
    vt_feature->set_raster(buffer);
    // Releasing the pointer is important here because the layer will take over ownership!
    builder.add_feature(vt_feature, mapnik_feature);
}

inline void raster_to_feature(std::string const& buffer,
                              mapnik::feature_impl const& mapnik_feature,
                              layer_builder_pbf & builder)
{
    protozero::pbf_writer feature_writer = builder.get_feature_writer();
    feature_writer.add_string(Feature_Encoding::RASTER, buffer);
    builder.add_feature(feature_writer, mapnik_feature);
}

struct geometry_to_feature_visitor
{
    mapnik::feature_impl const& mapnik_feature_;
    layer_builder & builder_;

    geometry_to_feature_visitor(mapnik::feature_impl const& mapnik_feature,
                                layer_builder & builder)
        : mapnik_feature_(mapnik_feature),
          builder_(builder) {}

    void operator() (mapnik::geometry::geometry_empty const&)
    {
        return;
    }

    template <typename T>
    void operator() (T const& geom)
    {
        std::int32_t x = 0;
        std::int32_t y = 0;
        std::unique_ptr<vector_tile::Tile_Feature> vt_feature(new vector_tile::Tile_Feature());
        if (encode_geometry(geom, *vt_feature, x, y))
        {
            // Releasing the pointer is important here because the layer will take over ownership!
            builder_.add_feature(vt_feature, mapnik_feature_);
        }
    }

    void operator() (mapnik::geometry::geometry_collection<std::int64_t> const& collection)
    {
        for (auto & g : collection)
        {
            mapnik::util::apply_visitor((*this), g);
        }
    }
};

struct geometry_to_feature_pbf_visitor
{
    mapnik::feature_impl const& mapnik_feature_;
    layer_builder_pbf & builder_;

    geometry_to_feature_pbf_visitor(mapnik::feature_impl const& mapnik_feature,
                                    layer_builder_pbf & builder)
        : mapnik_feature_(mapnik_feature),
          builder_(builder) {}

    void operator() (mapnik::geometry::geometry_empty const&)
    {
        return;
    }

    template <typename T>
    void operator() (T const& geom)
    {
        std::int32_t x = 0;
        std::int32_t y = 0;
        protozero::pbf_writer feature_writer = builder_.get_feature_writer();
        if (encode_geometry_pbf(geom, feature_writer, x, y))
        {
            // Releasing the pointer is important here because the layer will take over ownership!
            builder_.add_feature(feature_writer, mapnik_feature_);
        }
    }

    void operator() (mapnik::geometry::geometry_collection<std::int64_t> const& collection)
    {
        for (auto & g : collection)
        {
            mapnik::util::apply_visitor((*this), g);
        }
    }
};

} // end ns vector_tile_impl

} // end ns mapnik

#endif // __MAPNIK_VECTOR_GEOMETRY_FEATURE_H__
