// mapnik-vector-tile
#include "vector_tile_geometry_clipper.hpp"
#include "vector_tile_geometry_feature.hpp"
#include "vector_tile_geometry_simplifier.hpp"
#include "vector_tile_raster_clipper.hpp"
#include "vector_tile_strategy.hpp"
#include "vector_tile_tile.hpp"
#include "vector_tile_layer.hpp"

// mapnik
#include <mapnik/geometry/box2d.hpp>
#include <mapnik/datasource.hpp>
#include <mapnik/feature.hpp>
#include <mapnik/image_scaling.hpp>
#include <mapnik/layer.hpp>
#include <mapnik/map.hpp>
#include <mapnik/version.hpp>
#include <mapnik/attribute.hpp>

#if MAPNIK_VERSION >= 300100
#include <mapnik/geometry/transform.hpp>
#else
#include <mapnik/geometry_transform.hpp>
#endif

// boost
#include <boost/optional.hpp>

// std
#include <future>

namespace mapnik
{

namespace vector_tile_impl
{

namespace detail
{

inline void create_geom_layer(tile_layer & layer,
                              double simplify_distance,
                              double area_threshold,
                              polygon_fill_type fill_type,
                              bool strictly_simple,
                              bool multi_polygon_union,
                              bool process_all_rings)
{
    layer_builder_pbf builder(layer.name(), layer.layer_extent(), layer.get_data());

    // query for the features
    mapnik::featureset_ptr features = layer.get_features();

    if (!features)
    {
        return;
    }

    mapnik::feature_ptr feature = features->next();

    if (!feature)
    {
        return;
    }

    using encoding_process = mapnik::vector_tile_impl::geometry_to_feature_pbf_visitor;
    using clipping_process = mapnik::vector_tile_impl::geometry_clipper<encoding_process>;

    mapnik::vector_tile_impl::vector_tile_strategy vs(layer.get_view_transform());
    mapnik::box2d<double> const& buffered_extent = layer.get_target_buffered_extent();
    const mapbox::geometry::point<double> p1_min(buffered_extent.minx(), buffered_extent.miny());
    const mapbox::geometry::point<double> p1_max(buffered_extent.maxx(), buffered_extent.maxy());
    const mapbox::geometry::point<std::int64_t> p2_min = mapnik::geometry::transform<std::int64_t>(p1_min, vs);
    const mapbox::geometry::point<std::int64_t> p2_max = mapnik::geometry::transform<std::int64_t>(p1_max, vs);
    const double minx = std::min(p2_min.x, p2_max.x);
    const double maxx = std::max(p2_min.x, p2_max.x);
    const double miny = std::min(p2_min.y, p2_max.y);
    const double maxy = std::max(p2_min.y, p2_max.y);
    const mapbox::geometry::box<std::int64_t> tile_clipping_extent(mapbox::geometry::point<std::int64_t>(minx, miny),
                                                                   mapbox::geometry::point<std::int64_t>(maxx, maxy));

    if (simplify_distance > 0)
    {
        using simplifier_process = mapnik::vector_tile_impl::geometry_simplifier<clipping_process>;
        if (layer.get_proj_transform().equal())
        {
            using strategy_type = mapnik::vector_tile_impl::vector_tile_strategy;
            using transform_type = mapnik::vector_tile_impl::transform_visitor<strategy_type, simplifier_process>;
            while (feature)
            {
                mapnik::geometry::geometry<double> const& geom = feature->get_geometry();
                encoding_process encoder(*feature, builder);
                clipping_process clipper(tile_clipping_extent,
                                         area_threshold,
                                         strictly_simple,
                                         multi_polygon_union,
                                         fill_type,
                                         process_all_rings,
                                         encoder);
                simplifier_process simplifier(simplify_distance, clipper);
                transform_type transformer(vs, buffered_extent, simplifier);
                mapnik::util::apply_visitor(transformer, geom);
                feature = features->next();
            }
        }
        else
        {
            using strategy_type = mapnik::vector_tile_impl::vector_tile_strategy_proj;
            using transform_type = mapnik::vector_tile_impl::transform_visitor<strategy_type, simplifier_process>;
            strategy_type vs2(layer.get_proj_transform(), layer.get_view_transform());
            mapnik::box2d<double> const& trans_buffered_extent = layer.get_source_buffered_extent();
            while (feature)
            {
                mapnik::geometry::geometry<double> const& geom = feature->get_geometry();
                encoding_process encoder(*feature, builder);
                clipping_process clipper(tile_clipping_extent,
                                         area_threshold,
                                         strictly_simple,
                                         multi_polygon_union,
                                         fill_type,
                                         process_all_rings,
                                         encoder);
                simplifier_process simplifier(simplify_distance, clipper);
                transform_type transformer(vs2, trans_buffered_extent, simplifier);
                mapnik::util::apply_visitor(transformer, geom);
                feature = features->next();
            }
        }
    }
    else
    {
        if (layer.get_proj_transform().equal())
        {
            using strategy_type = mapnik::vector_tile_impl::vector_tile_strategy;
            using transform_type = mapnik::vector_tile_impl::transform_visitor<strategy_type, clipping_process>;
            while (feature)
            {
                mapnik::geometry::geometry<double> const& geom = feature->get_geometry();
                encoding_process encoder(*feature, builder);
                clipping_process clipper(tile_clipping_extent,
                                         area_threshold,
                                         strictly_simple,
                                         multi_polygon_union,
                                         fill_type,
                                         process_all_rings,
                                         encoder);
                transform_type transformer(vs, buffered_extent, clipper);
                mapnik::util::apply_visitor(transformer, geom);
                feature = features->next();
            }
        }
        else
        {
            using strategy_type = mapnik::vector_tile_impl::vector_tile_strategy_proj;
            using transform_type = mapnik::vector_tile_impl::transform_visitor<strategy_type, clipping_process>;
            strategy_type vs2(layer.get_proj_transform(), layer.get_view_transform());
            mapnik::box2d<double> const& trans_buffered_extent = layer.get_source_buffered_extent();
            while (feature)
            {
                mapnik::geometry::geometry<double> const& geom = feature->get_geometry();
                encoding_process encoder(*feature, builder);
                clipping_process clipper(tile_clipping_extent,
                                         area_threshold,
                                         strictly_simple,
                                         multi_polygon_union,
                                         fill_type,
                                         process_all_rings,
                                         encoder);
                transform_type transformer(vs2, trans_buffered_extent, clipper);
                mapnik::util::apply_visitor(transformer, geom);
                feature = features->next();
            }
        }
    }
    layer.build(builder);
    return;
}

inline void create_raster_layer(tile_layer & layer,
                                std::string const& image_format,
                                scaling_method_e scaling_method)
{
    layer_builder_pbf builder(layer.name(), layer.layer_extent(), layer.get_data());

    // query for the features
    mapnik::featureset_ptr features = layer.get_features();

    if (!features)
    {
        return;
    }

    mapnik::feature_ptr feature = features->next();

    if (!feature)
    {
        return;
    }

    mapnik::raster_ptr const& source = feature->get_raster();
    if (!source)
    {
        return;
    }

    mapnik::box2d<double> target_ext = box2d<double>(source->ext_);

    layer.get_proj_transform().backward(target_ext, PROJ_ENVELOPE_POINTS);
    mapnik::box2d<double> ext = layer.get_view_transform().forward(target_ext);

    int start_x = static_cast<int>(std::floor(ext.minx()+.5));
    int start_y = static_cast<int>(std::floor(ext.miny()+.5));
    int end_x = static_cast<int>(std::floor(ext.maxx()+.5));
    int end_y = static_cast<int>(std::floor(ext.maxy()+.5));
    int raster_width = end_x - start_x;
    int raster_height = end_y - start_y;
    if (raster_width > 0 && raster_height > 0)
    {
        builder.make_painted();
        raster_clipper visit(*source,
                             target_ext,
                             ext,
                             layer.get_proj_transform(),
                             image_format,
                             scaling_method,
                             layer.layer_extent(),
                             layer.layer_extent(),
                             raster_width,
                             raster_height,
                             start_x,
                             start_y);
        std::string buffer = mapnik::util::apply_visitor(visit, source->data_);
        raster_to_feature(buffer, *feature, builder);
    }
    layer.build(builder);
    return;
}

} // end ns detail

MAPNIK_VECTOR_INLINE void processor::update_tile(tile & t,
                                                 double scale_denom,
                                                 int offset_x,
                                                 int offset_y)
{
    // Futures
    std::vector<tile_layer> tile_layers;

    for (mapnik::layer const& lay : m_.layers())
    {
        if (t.has_layer(lay.name()))
        {
            continue;
        }
        tile_layers.emplace_back(m_,
                             lay,
                             t.extent(),
                             t.tile_size(),
                             t.buffer_size(),
                             scale_factor_,
                             scale_denom,
                             offset_x,
                             offset_y,
                             vars_);
        if (!tile_layers.back().is_valid())
        {
            t.add_empty_layer(lay.name());
            tile_layers.pop_back();
            continue;
        }
    }

    if (threading_mode_ == std::launch::deferred)
    {
        for (auto & layer_ref : tile_layers)
        {
            if (layer_ref.get_ds()->type() == datasource::Vector)
            {
                detail::create_geom_layer(layer_ref,
                                          simplify_distance_,
                                          area_threshold_,
                                          fill_type_,
                                          strictly_simple_,
                                          multi_polygon_union_,
                                          process_all_rings_
                                         );
            }
            else // Raster
            {
                detail::create_raster_layer(layer_ref,
                                            image_format_,
                                            scaling_method_
                                           );
            }
        }
    }
    else
    {
        std::vector<std::future<void> > future_layers;
        future_layers.reserve(tile_layers.size());

        for (auto & layer_ref : tile_layers)
        {
            if (layer_ref.get_ds()->type() == datasource::Vector)
            {
                future_layers.push_back(std::async(
                                        threading_mode_,
                                        detail::create_geom_layer,
                                        std::ref(layer_ref),
                                        simplify_distance_,
                                        area_threshold_,
                                        fill_type_,
                                        strictly_simple_,
                                        multi_polygon_union_,
                                        process_all_rings_
                            ));
            }
            else // Raster
            {
                future_layers.push_back(std::async(
                                        threading_mode_,
                                        detail::create_raster_layer,
                                        std::ref(layer_ref),
                                        image_format_,
                                        scaling_method_
                ));
            }
        }

        for (auto && lay_future : future_layers)
        {
            if (!lay_future.valid())
            {
                throw std::runtime_error("unexpected invalid async return");
            }
            lay_future.get();
        }
    }

    for (auto & layer_ref : tile_layers)
    {
        t.add_layer(layer_ref);
    }
}

} // end ns vector_tile_impl

} // end ns mapnik
