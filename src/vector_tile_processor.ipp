// mapnik-vector-tile
#include "vector_tile_strategy.hpp"
#include "vector_tile_geometry_clipper.hpp"
#include "vector_tile_geometry_simplifier.hpp"
#include "vector_tile_raster_clipper.hpp"

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

// http://www.angusj.com/delphi/clipper.php
#include "clipper.hpp"

// boost
#include <boost/optional.hpp>

namespace mapnik 
{

namespace vector_tile_impl 
{

/*
  This processor combines concepts from mapnik's
  feature_style_processor and agg_renderer. It
  differs in that here we only process layers in
  isolation of their styles, and because of this we need
  options for clipping and simplification, for example,
  that would normally come from a style's symbolizers
*/

template <typename T>
processor<T>::processor(T & backend,
              mapnik::Map const& map,
              mapnik::request const& m_req,
              double scale_factor,
              unsigned offset_x,
              unsigned offset_y,
              double area_threshold,
              bool strictly_simple,
              std::string const& image_format,
              scaling_method_e scaling_method)
    : backend_(backend),
      m_(map),
      m_req_(m_req),
      scale_factor_(scale_factor),
      t_(m_req.width(),m_req.height(),m_req.extent(),offset_x,offset_y),
      area_threshold_(area_threshold),
      strictly_simple_(strictly_simple),
      image_format_(image_format),
      scaling_method_(scaling_method),
      painted_(false),
      simplify_distance_(0.0),
      multi_polygon_union_(false),
      fill_type_(ClipperLib::pftNonZero),
      process_all_rings_(false) {}

template <typename T>
void processor<T>::apply(double scale_denom)
{
    mapnik::projection proj(m_.srs(),true);
    if (scale_denom <= 0.0)
    {
        scale_denom = mapnik::scale_denominator(m_req_.scale(),proj.is_geographic());
    }
    scale_denom *= scale_factor_;
    for (mapnik::layer const& lay : m_.layers())
    {
        if (lay.visible(scale_denom))
        {
            apply_to_layer(lay,
                           proj,
                           m_req_.scale(),
                           scale_denom,
                           m_req_.width(),
                           m_req_.height(),
                           m_req_.extent(),
                           m_req_.buffer_size());
        }
    }
}

template <typename T>
bool processor<T>::painted() const
{
    return painted_;
}
   
template <typename T> 
void processor<T>::set_fill_type(polygon_fill_type type)
{
    switch (type) 
    {
    case polygon_fill_type_max:
    case even_odd_fill:
        fill_type_ = ClipperLib::pftEvenOdd;
        break; 
    case non_zero_fill: 
        fill_type_ = ClipperLib::pftNonZero;
        break;
    case positive_fill:
        fill_type_ = ClipperLib::pftPositive;
        break; 
    case negative_fill:
        fill_type_ = ClipperLib::pftNegative;
        break;
    }
}


template <typename T>
void processor<T>::apply_to_layer(mapnik::layer const& lay,
                    mapnik::projection const& target_proj,
                    double scale,
                    double scale_denom,
                    unsigned width,
                    unsigned height,
                    box2d<double> const& extent,
                    int buffer_size)
{
    mapnik::datasource_ptr ds = lay.datasource();
    if (!ds) return;

    mapnik::projection source_proj(lay.srs(),true);

    // set up a transform from target to source
    // target == final map (aka tile) projection, usually epsg:3857
    // source == projection of the data being queried
    mapnik::proj_transform prj_trans(target_proj,source_proj);

    // working version of unbuffered extent
    box2d<double> query_ext(extent);

    // working version of buffered extent
    box2d<double> buffered_query_ext(query_ext);

    // transform the user-driven buffer size into the right
    // size buffer into the target projection
    double buffer_padding = 2.0 * scale;
    boost::optional<int> layer_buffer_size = lay.buffer_size();
    if (layer_buffer_size) // if layer overrides buffer size, use this value to compute buffered extent
    {
        buffer_padding *= *layer_buffer_size;
    }
    else
    {
        buffer_padding *= buffer_size;
    }
    buffered_query_ext.width(query_ext.width() + buffer_padding);
    buffered_query_ext.height(query_ext.height() + buffer_padding);

    // ^^ now `buffered_query_ext` is actually buffered out.

    // clip buffered extent by maximum extent, if supplied
    // Note: Carto.js used to set this by default but no longer does after:
    // https://github.com/mapbox/carto/pull/342
    boost::optional<box2d<double>> const& maximum_extent = m_.maximum_extent();
    if (maximum_extent)
    {
        buffered_query_ext.clip(*maximum_extent);
    }

    // buffered_query_ext is transformed below
    // into the coordinate system of the source data
    // so grab a pristine copy of it to use later
    box2d<double> target_clipping_extent(buffered_query_ext);

    mapnik::box2d<double> layer_ext = lay.envelope();
    bool early_return = false;
    // first, try intersection of map extent forward projected into layer srs
    if (prj_trans.forward(buffered_query_ext, PROJ_ENVELOPE_POINTS) && buffered_query_ext.intersects(layer_ext))
    {
        // this modifies the layer_ext by clipping to the buffered_query_ext
        layer_ext.clip(buffered_query_ext);
    }
    // if no intersection and projections are also equal, early return
    else if (prj_trans.equal())
    {
        early_return = true;
    }
    // next try intersection of layer extent back projected into map srs
    else if (prj_trans.backward(layer_ext, PROJ_ENVELOPE_POINTS) && buffered_query_ext.intersects(layer_ext))
    {
        layer_ext.clip(buffered_query_ext);
        // forward project layer extent back into native projection
        if (! prj_trans.forward(layer_ext, PROJ_ENVELOPE_POINTS))
        {
            std::cerr << "feature_style_processor: Layer=" << lay.name()
                      << " extent=" << layer_ext << " in map projection "
                      << " did not reproject properly back to layer projection";
        }
    }
    else
    {
        // if no intersection then nothing to do for layer
        early_return = true;
    }
    if (early_return)
    {
        return;
    }

    double qw = query_ext.width()>0 ? query_ext.width() : 1;
    double qh = query_ext.height()>0 ? query_ext.height() : 1;
    mapnik::query::resolution_type res(width/qw,
                                       height/qh);
    mapnik::query q(layer_ext,res,scale_denom,extent);
    mapnik::layer_descriptor lay_desc = ds->get_descriptor();
    for (mapnik::attribute_descriptor const& desc : lay_desc.get_descriptors())
    {
        q.add_property_name(desc.get_name());
    }
    mapnik::featureset_ptr features = ds->features(q);

    if (!features) return;

    mapnik::feature_ptr feature = features->next();
    if (feature)
    {
        if (!backend_.start_tile_layer(lay.name()))
        {
            return;
        }
        raster_ptr const& source = feature->get_raster();
        if (source)
        {
            box2d<double> target_ext = box2d<double>(source->ext_);
            prj_trans.backward(target_ext, PROJ_ENVELOPE_POINTS);
            box2d<double> ext = t_.forward(target_ext);
            int start_x = static_cast<int>(std::floor(ext.minx()+.5));
            int start_y = static_cast<int>(std::floor(ext.miny()+.5));
            int end_x = static_cast<int>(std::floor(ext.maxx()+.5));
            int end_y = static_cast<int>(std::floor(ext.maxy()+.5));
            int raster_width = end_x - start_x;
            int raster_height = end_y - start_y;
            if (raster_width > 0 && raster_height > 0)
            {
                raster_clipper<T> visit(*source,
                                        *feature,
                                        target_ext,
                                        ext,
                                        backend_,
                                        painted_,
                                        prj_trans,
                                        image_format_,
                                        scaling_method_,
                                        width,
                                        height,
                                        raster_width,
                                        raster_height,
                                        start_x,
                                        start_y);
                mapnik::util::apply_visitor(visit, source->data_);
            }
            backend_.stop_tile_layer();
            return;
        }
        // vector pathway
        if (prj_trans.equal())
        {
            mapnik::vector_tile_impl::vector_tile_strategy vs(t_,backend_.get_path_multiplier());
            mapnik::geometry::point<double> p1_min(target_clipping_extent.minx(), target_clipping_extent.miny());
            mapnik::geometry::point<double> p1_max(target_clipping_extent.maxx(), target_clipping_extent.maxy());
            mapnik::geometry::point<std::int64_t> p2_min = mapnik::geometry::transform<std::int64_t>(p1_min, vs);
            mapnik::geometry::point<std::int64_t> p2_max = mapnik::geometry::transform<std::int64_t>(p1_max, vs);
            box2d<int> tile_clipping_extent(p2_min.x, p2_min.y, p2_max.x, p2_max.y);
            while (feature)
            {
                mapnik::geometry::geometry<double> const& geom = feature->get_geometry();
                if (mapnik::geometry::is_empty(geom))
                {
                    feature = features->next();
                    continue;
                }
                if (handle_geometry(vs,
                                    *feature,
                                    geom,
                                    tile_clipping_extent,
                                    target_clipping_extent))
                {
                    painted_ = true;
                }
                feature = features->next();
            }
        }
        else
        {
            mapnik::vector_tile_impl::vector_tile_strategy vs(t_,backend_.get_path_multiplier());
            mapnik::geometry::point<double> p1_min(target_clipping_extent.minx(), target_clipping_extent.miny());
            mapnik::geometry::point<double> p1_max(target_clipping_extent.maxx(), target_clipping_extent.maxy());
            mapnik::geometry::point<std::int64_t> p2_min = mapnik::geometry::transform<std::int64_t>(p1_min, vs);
            mapnik::geometry::point<std::int64_t> p2_max = mapnik::geometry::transform<std::int64_t>(p1_max, vs);
            box2d<int> tile_clipping_extent(p2_min.x, p2_min.y, p2_max.x, p2_max.y);
            mapnik::vector_tile_impl::vector_tile_strategy_proj vs2(prj_trans,t_,backend_.get_path_multiplier());
            prj_trans.forward(target_clipping_extent, PROJ_ENVELOPE_POINTS);
            while (feature)
            {
                mapnik::geometry::geometry<double> const& geom = feature->get_geometry();
                if (mapnik::geometry::is_empty(geom))
                {
                    feature = features->next();
                    continue;
                }
                if (handle_geometry(vs2,
                                    *feature,
                                    geom,
                                    tile_clipping_extent,
                                    target_clipping_extent) > 0)
                {
                    painted_ = true;
                }
                feature = features->next();
            }
        }
        backend_.stop_tile_layer();
    }
}

template <typename T> template <typename T2>
bool processor<T>::handle_geometry(T2 const& vs,
                                   mapnik::feature_impl const& feature,
                                   mapnik::geometry::geometry<double> const& geom,
                                   mapnik::box2d<int> const& tile_clipping_extent,
                                   mapnik::box2d<double> const& target_clipping_extent)
{
    // TODO
    // - no need to create a new skipping_transformer per geometry
    // - write a non-skipping / zero copy transformer to be used when no projection is needed
    using vector_tile_strategy_type = T2;
    mapnik::vector_tile_impl::transform_visitor<vector_tile_strategy_type> skipping_transformer(vs, target_clipping_extent);
    mapnik::geometry::geometry<std::int64_t> new_geom = mapnik::util::apply_visitor(skipping_transformer,geom);
    geometry_clipper<T> encoder(backend_, 
                                feature, 
                                tile_clipping_extent, 
                                area_threshold_, 
                                strictly_simple_, 
                                multi_polygon_union_, 
                                fill_type_,
                                process_all_rings_);
    if (simplify_distance_ > 0)
    {
        geometry_simplifier<T> simplifier(simplify_distance_,encoder);
        return mapnik::util::apply_visitor(simplifier,new_geom);
    }
    else
    {
        return mapnik::util::apply_visitor(encoder,new_geom);
    }
}

} // end ns vector_tile_impl

} // end ns mapnik
