#include <mapnik/map.hpp>
#include <mapnik/request.hpp>
#include <mapnik/layer.hpp>
#include <mapnik/query.hpp>
#include <mapnik/geometry.hpp>
#include <mapnik/feature.hpp>
#include <mapnik/datasource.hpp>
#include <mapnik/projection.hpp>
#include <mapnik/proj_transform.hpp>
#include <mapnik/scale_denominator.hpp>
#include <mapnik/attribute_descriptor.hpp>
#include <mapnik/feature_layer_desc.hpp>
#include <mapnik/config.hpp>
#include <mapnik/box2d.hpp>
#include <mapnik/version.hpp>
#include <mapnik/image_util.hpp>
#include <mapnik/raster.hpp>
#include <mapnik/warp.hpp>
#include <mapnik/version.hpp>
#include <mapnik/image_scaling.hpp>
#include <mapnik/image_compositing.hpp>
#include <mapnik/view_transform.hpp>
#include <mapnik/util/noncopyable.hpp>
#include <mapnik/transform_path_adapter.hpp>

// agg
#include "agg_path_storage.h"

// agg core clipper: http://www.angusj.com/delphi/clipper.php
#include "agg_conv_clipper.h"

// angus clipper
#include "agg_conv_clip_polygon.h"

#include "agg_conv_clip_polyline.h"
#include "agg_rendering_buffer.h"
#include "agg_pixfmt_rgba.h"
#include "agg_renderer_base.h"

#include <boost/optional.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include <iostream>
#include <string>
#include <stdexcept>

namespace mapnik { namespace vector_tile_impl {


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
              unsigned tolerance,
              std::string const& image_format,
              scaling_method_e scaling_method)
    : backend_(backend),
      m_(map),
      m_req_(m_req),
      scale_factor_(scale_factor),
      t_(m_req.width(),m_req.height(),m_req.extent(),offset_x,offset_y),
      tolerance_(tolerance),
      image_format_(image_format),
      scaling_method_(scaling_method),
      painted_(false),
      poly_clipper_type_(AGG_CLIPPER) {}

template <typename T>
void processor<T>::set_poly_clipper(poly_clipper_type clipper)
{
    poly_clipper_type_ = clipper;
}

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
void processor<T>::apply_to_layer(mapnik::layer const& lay,
                    mapnik::projection const& proj0,
                    double scale,
                    double scale_denom,
                    unsigned width,
                    unsigned height,
                    box2d<double> const& extent,
                    int buffer_size)
{
    mapnik::datasource_ptr ds = lay.datasource();
    if (!ds) return;

    mapnik::projection proj1(lay.srs(),true);
    mapnik::proj_transform prj_trans(proj0,proj1);
    box2d<double> query_ext = extent; // unbuffered
    box2d<double> buffered_query_ext(query_ext);  // buffered
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
    // clip buffered extent by maximum extent, if supplied
    boost::optional<box2d<double> > const& maximum_extent = m_.maximum_extent();
    if (maximum_extent)
    {
        buffered_query_ext.clip(*maximum_extent);
    }
    mapnik::box2d<double> layer_ext = lay.envelope();
    bool fw_success = false;
    bool early_return = false;
    // first, try intersection of map extent forward projected into layer srs
    if (prj_trans.forward(buffered_query_ext, PROJ_ENVELOPE_POINTS) && buffered_query_ext.intersects(layer_ext))
    {
        fw_success = true;
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

    // if we've got this far, now prepare the unbuffered extent
    // which is used as a bbox for clipping geometries
    if (maximum_extent)
    {
        query_ext.clip(*maximum_extent);
    }
    mapnik::box2d<double> layer_ext2 = lay.envelope();
    if (fw_success)
    {
        if (prj_trans.forward(query_ext, PROJ_ENVELOPE_POINTS))
        {
            layer_ext2.clip(query_ext);
        }
    }
    else
    {
        if (prj_trans.backward(layer_ext2, PROJ_ENVELOPE_POINTS))
        {
            layer_ext2.clip(query_ext);
            prj_trans.forward(layer_ext2, PROJ_ENVELOPE_POINTS);
        }
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
        backend_.start_tile_layer(lay.name());
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
                mapnik::image_data_rgba8 data(raster_width, raster_height);
                raster target(target_ext, data, source->get_filter_factor());
                if (!source->premultiplied_alpha_)
                {
                    agg::rendering_buffer buffer(source->data_.getBytes(),
                                                 source->data_.width(),
                                                 source->data_.height(),
                                                 source->data_.width() * 4);
                    agg::pixfmt_rgba32 pixf(buffer);
                    pixf.premultiply();
                }
                if (!prj_trans.equal())
                {
                    double offset_x = ext.minx() - start_x;
                    double offset_y = ext.miny() - start_y;
                    reproject_and_scale_raster(target, *source, prj_trans,
                                               offset_x, offset_y,
                                               width,
                                               scaling_method_);
                }
                else
                {
                    double image_ratio_x = ext.width() / source->data_.width();
                    double image_ratio_y = ext.height() / source->data_.height();
                    scale_image_agg(util::get<image_data_rgba8>(target.data_),
                                    util::get<image_data_rgba8>(source->data_),
                                    scaling_method_,
                                    image_ratio_x,
                                    image_ratio_y,
                                    0.0,
                                    0.0,
                                    source->get_filter_factor());
                }
                if (target.data_.is<image_data_rgba8>())
                {
                    using pixfmt_type = agg::pixfmt_rgba32;
                    using renderer_type = agg::renderer_base<pixfmt_type>;

                    mapnik::image_data_rgba8 im_tile(width, height);
                    auto & src = util::get<image_data_rgba8>(target.data_);
                    agg::rendering_buffer dst_buffer(im_tile.getBytes(), im_tile.width(), im_tile.height(), im_tile.width() * 4);
                    agg::rendering_buffer src_buffer(src.getBytes(),src.width(), src.height(), src.width() * 4);
                    pixfmt_type src_pixf(src_buffer);
                    pixfmt_type dst_pixf(dst_buffer);
                    renderer_type ren(dst_pixf);
                    ren.blend_from(src_pixf,0,start_x, start_y, 255);
                    backend_.start_tile_feature(*feature);
                    backend_.add_tile_feature_raster(mapnik::save_to_string(im_tile,image_format_));
                    painted_ = true;
                }
                else
                {
                    std::clog << "TODO: support other pixel types\n";
                }
            }
            backend_.stop_tile_layer();
            return;
        }
        // vector pathway
        while (feature)
        {
            mapnik::geometry_container const& paths = feature->paths();
            if (paths.empty()) {
                feature = features->next();
                continue;
            }
            backend_.start_tile_feature(*feature);
            for (mapnik::geometry_type const& geom : paths)
            {
                mapnik::vertex_adapter va(geom);
                mapnik::box2d<double> geom_box = va.envelope();
                if (!geom_box.intersects(buffered_query_ext))
                {
                    continue;
                }
                if (handle_geometry(va,
                                    prj_trans,
                                    buffered_query_ext) > 0)
                {
                    painted_ = true;
                }
            }
            backend_.stop_tile_feature();
            feature = features->next();
        }
        backend_.stop_tile_layer();
    }
}

template <typename T>
unsigned processor<T>::handle_geometry(mapnik::vertex_adapter & geom,
                         mapnik::proj_transform const& prj_trans,
                         mapnik::box2d<double> const& buffered_query_ext)
{
    unsigned path_count = 0;
    switch (geom.type())
    {
    case mapnik::geometry_type::types::Point:
    {
        if (geom.size() > 0)
        {
            typedef mapnik::transform_path_adapter<mapnik::view_transform,
                                          mapnik::vertex_adapter> path_type;
            path_type path(t_, geom, prj_trans);
            path_count = backend_.add_path(path, tolerance_, geom.type());
        }
        break;
    }
    case mapnik::geometry_type::types::LineString:
    {
        if (geom.size() > 1)
        {
            typedef agg::conv_clip_polyline<mapnik::vertex_adapter> line_clipper;
            line_clipper clipped(geom);
            clipped.clip_box(
                buffered_query_ext.minx(),
                buffered_query_ext.miny(),
                buffered_query_ext.maxx(),
                buffered_query_ext.maxy());
            typedef mapnik::transform_path_adapter<mapnik::view_transform, line_clipper> path_type;
            path_type path(t_, clipped, prj_trans);
            path_count = backend_.add_path(path, tolerance_, geom.type());
        }
        break;
    }
    case mapnik::geometry_type::types::Polygon:
    {
        if (geom.size() > 2)
        {
            if (get_poly_clipper() == ANGUS_CLIPPER)
            {
                agg::path_storage ps;
                ps.move_to(buffered_query_ext.minx(), buffered_query_ext.miny());
                ps.line_to(buffered_query_ext.minx(), buffered_query_ext.maxy());
                ps.line_to(buffered_query_ext.maxx(), buffered_query_ext.maxy());
                ps.line_to(buffered_query_ext.maxx(), buffered_query_ext.miny());
                ps.close_polygon();
                typedef agg::conv_clipper<mapnik::vertex_adapter, agg::path_storage> poly_clipper;
                poly_clipper clipped(geom,ps,
                                     agg::clipper_and,
                                     agg::clipper_non_zero,
                                     agg::clipper_non_zero,
                                     1);
                typedef mapnik::transform_path_adapter<mapnik::view_transform, poly_clipper> path_type;
                path_type path(t_, clipped, prj_trans);
                path_count = backend_.add_path(path, tolerance_, geom.type());
            }
            else
            {
                typedef agg::conv_clip_polygon<mapnik::vertex_adapter> poly_clipper;
                poly_clipper clipped(geom);
                clipped.clip_box(
                    buffered_query_ext.minx(),
                    buffered_query_ext.miny(),
                    buffered_query_ext.maxx(),
                    buffered_query_ext.maxy());

                typedef mapnik::transform_path_adapter<mapnik::view_transform, poly_clipper> path_type;
                path_type path(t_, clipped, prj_trans);
                path_count = backend_.add_path(path, tolerance_, geom.type());
            }
        }
        break;
    }
    case mapnik::geometry_type::types::Unknown:
    default:
    {
        throw std::runtime_error("unhandled geometry type");
        break;
    }
    }
    return path_count;
}

}} // end ns
