// mapnik-vector-tile
#include "vector_tile_geometry_clipper.hpp"
#include "vector_tile_geometry_feature.hpp"
#include "vector_tile_geometry_intersects.hpp"
#include "vector_tile_geometry_simplifier.hpp"
#include "vector_tile_projection.hpp"
#include "vector_tile_raster_clipper.hpp"
#include "vector_tile_strategy.hpp"
#include "vector_tile_tile.hpp"
#include "vector_tile_layer.hpp"

// mapnik
#include <mapnik/box2d.hpp>
#include <mapnik/datasource.hpp>
#include <mapnik/feature.hpp>
#include <mapnik/geometry.hpp>
#include <mapnik/geometry_is_empty.hpp>
#include <mapnik/geometry_transform.hpp>
#include <mapnik/image_scaling.hpp>
#include <mapnik/layer.hpp>
#include <mapnik/map.hpp>
#include <mapnik/projection.hpp>
#include <mapnik/query.hpp>
#include <mapnik/request.hpp>
#include <mapnik/scale_denominator.hpp>
#include <mapnik/transform_path_adapter.hpp>
#include <mapnik/view_transform.hpp>

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

inline box2d<double> get_buffered_extent(tile const& t,
                                         std::uint32_t layer_extent, 
                                         mapnik::layer const& lay,
                                         mapnik::Map const & map)
{
    mapnik::box2d<double> ext(t.extent());
    double scale = ext.width() / layer_extent;
    double buffer_padding = 2.0 * scale;
    boost::optional<int> layer_buffer_size = lay.buffer_size();
    if (layer_buffer_size) // if layer overrides buffer size, use this value to compute buffered extent
    {
        buffer_padding *= *layer_buffer_size;
    }
    else
    {
        buffer_padding *= t.buffer_size();
    }
    double buffered_width = ext.width() + buffer_padding;
    double buffered_height = ext.height() + buffer_padding;
    if (buffered_width < 0.0)
    {
        buffered_width = 0.0;
    }
    if (buffered_height < 0.0)
    {
        buffered_height = 0.0;
    }
    ext.width(buffered_width);
    ext.height(buffered_height);
    
    boost::optional<box2d<double> > const& maximum_extent = map.maximum_extent();
    if (maximum_extent)
    {
        ext.clip(*maximum_extent);
    }
    return ext;
}

// change layer extent to the query extent
// return false if no query is required.
inline bool set_query_extent(mapnik::box2d<double> & layer_ext,
                             mapnik::box2d<double> buffered_ext,
                             mapnik::projection const& target_proj,
                             mapnik::projection const& source_proj)
{
    // set up a transform from target to source
    // target == final map (aka tile) projection, usually epsg:3857
    // source == projection of the data being queried
    mapnik::proj_transform prj_trans(target_proj, source_proj);
                      
    // first, try intersection of map extent forward projected into layer srs
    if (prj_trans.forward(buffered_ext, PROJ_ENVELOPE_POINTS) && buffered_ext.intersects(layer_ext))
    {
        // this modifies the layer_ext by clipping to the buffered_ext
        layer_ext.clip(buffered_ext);
    }
    // if no intersection and projections are also equal, early return
    else if (prj_trans.equal())
    {
        return false;
    }
    // next try intersection of layer extent back projected into map srs
    else if (prj_trans.backward(layer_ext, PROJ_ENVELOPE_POINTS) && buffered_ext.intersects(layer_ext))
    {
        layer_ext.clip(buffered_ext);
        // forward project layer extent back into native projection
        if (! prj_trans.forward(layer_ext, PROJ_ENVELOPE_POINTS))
        {
            throw std::runtime_error("vector_tile_processor: layer extent did not repoject back to map projection");
        }
    }
    else
    {
        // if no intersection then nothing to do for layer
        return false;    
    }
    return true;
}

inline mapnik::query build_query(box2d<double> const& tile_extent,
                                 std::uint32_t layer_extent,
                                 mapnik::box2d<double> const& query_ext,
                                 double scale_denom,
                                 mapnik::datasource_ptr ds)
{
    double qw = tile_extent.width() > 0 ? tile_extent.width() : 1;
    double qh = tile_extent.height() > 0 ? tile_extent.height() : 1;
    mapnik::query::resolution_type res(layer_extent / qw, layer_extent / qh);
    mapnik::query q(query_ext, res, scale_denom, tile_extent);
    mapnik::layer_descriptor lay_desc = ds->get_descriptor();
    for (mapnik::attribute_descriptor const& desc : lay_desc.get_descriptors())
    {
        q.add_property_name(desc.get_name());
    }
    return q;
}

inline tile_layer create_geom_layer(mapnik::datasource_ptr ds,
                                    mapnik::query const& q,
                                    std::string const& layer_name,
                                    std::uint32_t layer_extent,
                                    std::string const& target_proj_srs,
                                    std::string const& source_proj_srs,
                                    mapnik::view_transform const& view_trans,
                                    mapnik::box2d<double> const& buffered_extent,
                                    double simplify_distance,
                                    double area_threshold,
                                    polygon_fill_type fill_type,
                                    bool strictly_simple,
                                    bool multi_polygon_union,
                                    bool process_all_rings)
{
    // Setup projection
    mapnik::projection target_proj(target_proj_srs, true);
    mapnik::projection source_proj(source_proj_srs, true);
    // set up a transform from target to source
    // target == final map (aka tile) projection, usually epsg:3857
    // source == projection of the data being queried
    mapnik::proj_transform prj_trans(target_proj, source_proj);
    tile_layer layer;
    layer.name(layer_name);
    layer_builder_pbf builder(layer_name, layer_extent, layer.get_data());

    // query for the features
    mapnik::featureset_ptr features = ds->features(q);

    if (!features)
    {
        return layer;
    }

    mapnik::feature_ptr feature = features->next();

    if (!feature)
    {
        return layer;
    }
    
    using encoding_process = mapnik::vector_tile_impl::geometry_to_feature_pbf_visitor;
    using clipping_process = mapnik::vector_tile_impl::geometry_clipper<encoding_process>;
    if (simplify_distance > 0)
    {
        using simplifier_process = mapnik::vector_tile_impl::geometry_simplifier<clipping_process>;
        if (prj_trans.equal())
        {
            using strategy_type = mapnik::vector_tile_impl::vector_tile_strategy;
            using transform_type = mapnik::vector_tile_impl::transform_visitor<strategy_type, simplifier_process>;
            strategy_type vs(view_trans);
            mapnik::geometry::point<double> p1_min(buffered_extent.minx(), buffered_extent.miny());
            mapnik::geometry::point<double> p1_max(buffered_extent.maxx(), buffered_extent.maxy());
            mapnik::geometry::point<std::int64_t> p2_min = mapnik::geometry::transform<std::int64_t>(p1_min, vs);
            mapnik::geometry::point<std::int64_t> p2_max = mapnik::geometry::transform<std::int64_t>(p1_max, vs);
            double minx = std::min(p2_min.x, p2_max.x);
            double maxx = std::max(p2_min.x, p2_max.x);
            double miny = std::min(p2_min.y, p2_max.y);
            double maxy = std::max(p2_min.y, p2_max.y);
            mapnik::box2d<int> tile_clipping_extent(minx, miny, maxx, maxy);
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
            mapnik::vector_tile_impl::vector_tile_strategy vs(view_trans);
            mapnik::geometry::point<double> p1_min(buffered_extent.minx(), buffered_extent.miny());
            mapnik::geometry::point<double> p1_max(buffered_extent.maxx(), buffered_extent.maxy());
            mapnik::geometry::point<std::int64_t> p2_min = mapnik::geometry::transform<std::int64_t>(p1_min, vs);
            mapnik::geometry::point<std::int64_t> p2_max = mapnik::geometry::transform<std::int64_t>(p1_max, vs);
            double minx = std::min(p2_min.x, p2_max.x);
            double maxx = std::max(p2_min.x, p2_max.x);
            double miny = std::min(p2_min.y, p2_max.y);
            double maxy = std::max(p2_min.y, p2_max.y);
            mapnik::box2d<int> tile_clipping_extent(minx, miny, maxx, maxy);
            strategy_type vs2(prj_trans, view_trans);
            mapnik::box2d<double> trans_buffered_extent(buffered_extent);
            prj_trans.forward(trans_buffered_extent, PROJ_ENVELOPE_POINTS);
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
        if (prj_trans.equal())
        {
            using strategy_type = mapnik::vector_tile_impl::vector_tile_strategy;
            using transform_type = mapnik::vector_tile_impl::transform_visitor<strategy_type, clipping_process>;
            strategy_type vs(view_trans);
            mapnik::geometry::point<double> p1_min(buffered_extent.minx(), buffered_extent.miny());
            mapnik::geometry::point<double> p1_max(buffered_extent.maxx(), buffered_extent.maxy());
            mapnik::geometry::point<std::int64_t> p2_min = mapnik::geometry::transform<std::int64_t>(p1_min, vs);
            mapnik::geometry::point<std::int64_t> p2_max = mapnik::geometry::transform<std::int64_t>(p1_max, vs);
            double minx = std::min(p2_min.x, p2_max.x);
            double maxx = std::max(p2_min.x, p2_max.x);
            double miny = std::min(p2_min.y, p2_max.y);
            double maxy = std::max(p2_min.y, p2_max.y);
            mapnik::box2d<int> tile_clipping_extent(minx, miny, maxx, maxy);
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
            mapnik::vector_tile_impl::vector_tile_strategy vs(view_trans);
            mapnik::geometry::point<double> p1_min(buffered_extent.minx(), buffered_extent.miny());
            mapnik::geometry::point<double> p1_max(buffered_extent.maxx(), buffered_extent.maxy());
            mapnik::geometry::point<std::int64_t> p2_min = mapnik::geometry::transform<std::int64_t>(p1_min, vs);
            mapnik::geometry::point<std::int64_t> p2_max = mapnik::geometry::transform<std::int64_t>(p1_max, vs);
            double minx = std::min(p2_min.x, p2_max.x);
            double maxx = std::max(p2_min.x, p2_max.x);
            double miny = std::min(p2_min.y, p2_max.y);
            double maxy = std::max(p2_min.y, p2_max.y);
            mapnik::box2d<int> tile_clipping_extent(minx, miny, maxx, maxy);
            strategy_type vs2(prj_trans, view_trans);
            mapnik::box2d<double> trans_buffered_extent(buffered_extent);
            prj_trans.forward(trans_buffered_extent, PROJ_ENVELOPE_POINTS);
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
    return layer;
}

inline tile_layer create_raster_layer(mapnik::datasource_ptr ds,
                                      mapnik::query const& q,
                                      std::string const& layer_name,
                                      std::uint32_t layer_extent,
                                      std::string const& target_proj_srs,
                                      std::string const& source_proj_srs,
                                      mapnik::view_transform const& view_trans,
                                      std::string const& image_format,
                                      scaling_method_e scaling_method)
{
    // Setup projection
    mapnik::projection target_proj(target_proj_srs, true);
    mapnik::projection source_proj(source_proj_srs, true);
    // set up a transform from target to source
    // target == final map (aka tile) projection, usually epsg:3857
    // source == projection of the data being queried
    mapnik::proj_transform prj_trans(target_proj, source_proj);
    tile_layer layer;
    layer.name(layer_name);
    layer_builder_pbf builder(layer_name, layer_extent, layer.get_data());

    // query for the features
    mapnik::featureset_ptr features = ds->features(q);

    if (!features)
    {
        return layer;
    }

    mapnik::feature_ptr feature = features->next();

    if (!feature)
    {
        return layer;
    }
    
    mapnik::raster_ptr const& source = feature->get_raster();
    if (!source)
    {
        return layer;
    }

    mapnik::box2d<double> target_ext = box2d<double>(source->ext_);
    
    prj_trans.backward(target_ext, PROJ_ENVELOPE_POINTS);
    mapnik::box2d<double> ext = view_trans.forward(target_ext);

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
                             prj_trans,
                             image_format,
                             scaling_method,
                             layer_extent,
                             layer_extent,
                             raster_width,
                             raster_height,
                             start_x,
                             start_y);
        std::string buffer = mapnik::util::apply_visitor(visit, source->data_);
        raster_to_feature(buffer, *feature, builder);
    }
    layer.build(builder);
    return layer;
}

} // end ns detail

MAPNIK_VECTOR_INLINE void processor::update_tile(tile & t,
                                                 double scale_denom,
                                                 int offset_x,
                                                 int offset_y)
{
    // Futures
    std::vector<std::future<tile_layer> > lay_vec;
    lay_vec.reserve(m_.layers().size());
    
    for (mapnik::layer const& lay : m_.layers())
    {
        if (t.has_layer(lay.name()))
        {
            t.add_empty_layer(lay.name());
            continue;
        }
        
        mapnik::datasource_ptr ds = lay.datasource();
        if (!ds)
        {
            t.add_empty_layer(lay.name());
            continue;
        }
        
        std::uint32_t layer_extent = t.tile_size();

        auto ds_extent = ds->params().get<mapnik::value_integer>("vector_layer_extent");
        if (ds_extent)
        {
            layer_extent = *ds_extent;
        }

        if (layer_extent == 0)
        {
            t.add_empty_layer(lay.name());
            continue;
        }
        
        std::string const& target_proj_srs = m_.srs();
        std::string const& source_proj_srs = lay.srs();
        mapnik::projection target_proj(target_proj_srs, true);
        mapnik::projection source_proj(source_proj_srs, true);
        
        double layer_scale_denom = scale_denom;
        
        // Adjust the scale denominator if required
        if (layer_scale_denom <= 0.0)
        {
            double scale = t.extent().width() / layer_extent;
            layer_scale_denom = mapnik::scale_denominator(scale, target_proj.is_geographic());
        }
        layer_scale_denom *= scale_factor_;
    
        if (!lay.visible(scale_denom))
        {
            t.add_empty_layer(lay.name());
            continue;
        }
        mapnik::box2d<double> buffered_extent = detail::get_buffered_extent(t, layer_extent, lay, m_);
        mapnik::box2d<double> query_ext(lay.envelope());
        
        if (!detail::set_query_extent(query_ext, buffered_extent, target_proj, source_proj))
        {
            t.add_empty_layer(lay.name());
            continue;
        }
        
        mapnik::query q = detail::build_query(t.extent(), layer_extent, query_ext, layer_scale_denom, ds);
        
        mapnik::view_transform view_trans(layer_extent,
                                          layer_extent,
                                          t.extent(), 
                                          offset_x, 
                                          offset_y);

        if (ds->type() == datasource::Vector)
        {
            lay_vec.push_back(std::async(
                        std::launch::deferred, // uncomment to make single threaded  
                        detail::create_geom_layer,
                        ds,
                        q,
                        lay.name(),
                        layer_extent,
                        target_proj_srs,
                        source_proj_srs,
                        view_trans,
                        buffered_extent,
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
            lay_vec.push_back(std::async(
                        std::launch::deferred, // uncomment to make single threaded  
                        detail::create_raster_layer,
                        ds,
                        q,
                        lay.name(),
                        layer_extent,
                        target_proj_srs,
                        source_proj_srs,
                        view_trans,
                        image_format_,
                        scaling_method_
            ));
        }
    }

    for (auto & lay_future : lay_vec)
    {
        if (!lay_future.valid())
        {
            throw std::runtime_error("unexpected invalid async return");
        }
        tile_layer const& l = lay_future.get();
        t.add_layer(l); 
    }
}

} // end ns vector_tile_impl

} // end ns mapnik
