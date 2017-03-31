#ifndef __MAPNIK_VECTOR_TILE_GEOMETRY_FEATURE_H__
#define __MAPNIK_VECTOR_TILE_GEOMETRY_FEATURE_H__

// mapnik-vector-tile
#include "vector_tile_config.hpp"
#include "vector_tile_layer.hpp"
#include "vector_tile_geometry_encoder_pbf.hpp"

// mapnik
#include <mapnik/feature.hpp>
#include <mapnik/util/variant.hpp>

// Mapbox
#include <mapbox/geometry/geometry.hpp>


namespace mapnik
{

namespace vector_tile_impl
{

inline void raster_to_feature(std::string const& buffer,
                              mapnik::feature_impl const& mapnik_feature,
                              layer_builder_pbf & builder)
{
    std::vector<std::uint32_t> feature_tags;
    protozero::pbf_writer layer_writer = builder.add_feature(mapnik_feature, feature_tags);
    protozero::pbf_writer feature_writer(layer_writer, Layer_Encoding::FEATURES);
    feature_writer.add_uint64(Feature_Encoding::ID, static_cast<std::uint64_t>(mapnik_feature.id()));
    feature_writer.add_string(Feature_Encoding::RASTER, buffer);
    feature_writer.add_packed_uint32(Feature_Encoding::TAGS, feature_tags.begin(), feature_tags.end());
    builder.make_not_empty();
}

struct geometry_to_feature_pbf_visitor
{
    mapnik::feature_impl const& mapnik_feature_;
    layer_builder_pbf & builder_;

    geometry_to_feature_pbf_visitor(mapnik::feature_impl const& mapnik_feature,
                                    layer_builder_pbf & builder)
        : mapnik_feature_(mapnik_feature),
          builder_(builder) {}

    template <typename T>
    void operator() (T const& geom)
    {
        std::int32_t x = 0;
        std::int32_t y = 0;
        bool success = false;
        std::vector<std::uint32_t> feature_tags;
        protozero::pbf_writer layer_writer = builder_.add_feature(mapnik_feature_, feature_tags);
        {
            protozero::pbf_writer feature_writer(layer_writer, Layer_Encoding::FEATURES);
            success = encode_geometry_pbf(geom, feature_writer, x, y);
            if (success)
            {
                feature_writer.add_uint64(Feature_Encoding::ID, static_cast<std::uint64_t>(mapnik_feature_.id()));
                feature_writer.add_packed_uint32(Feature_Encoding::TAGS, feature_tags.begin(), feature_tags.end());
                builder_.make_not_empty();
            }
            else
            {
                feature_writer.rollback();
            }
        }   
    }

    void operator() (mapbox::geometry::geometry_collection<std::int64_t> const& collection)
    {
        for (auto & g : collection)
        {
            mapbox::util::apply_visitor((*this), g);
        }
    }
};

} // end ns vector_tile_impl

} // end ns mapnik

#endif // __MAPNIK_VECTOR_GEOMETRY_FEATURE_H__
