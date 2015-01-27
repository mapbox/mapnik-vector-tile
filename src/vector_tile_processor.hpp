#ifndef __MAPNIK_VECTOR_PROCESSOR_H__
#define __MAPNIK_VECTOR_PROCESSOR_H__

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

// agg
#ifdef CONV_CLIPPER
#include "agg_path_storage.h"
#include "agg_conv_clipper.h"
#else
#include "agg_conv_clip_polygon.h"
#endif

#include "agg_conv_clip_polyline.h"
#include "agg_rendering_buffer.h"
#include "agg_pixfmt_rgba.h"
#include "agg_pixfmt_gray.h"
#include "agg_renderer_base.h"

#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include <iostream>
#include <string>
#include <stdexcept>

#include "mapnik3x_compatibility.hpp"
#include MAPNIK_MAKE_SHARED_INCLUDE
#include MAPNIK_SHARED_INCLUDE
#include MAPNIK_VIEW_TRANSFORM_INCLUDE
#include MAPNIK_TRANSFORM_PATH_INCLUDE
#include MAPNIK_NONCOPYABLE_INCLUDE

namespace mapnik { namespace vector_tile_impl {

#if MAPNIK_VERSION >= 300000
template <typename T>
struct visitor_raster_processor 
{
public:
    typedef T backend_type;
private:
    mapnik::raster & source_;
    mapnik::feature_impl & feature_;
    box2d<double> & target_ext_;
    box2d<double> & ext_;
    backend_type & backend_;
    bool & painted_;
    mapnik::proj_transform & prj_trans_;
    std::string const& image_format_;
    scaling_method_e scaling_method_;
    unsigned width_;
    unsigned height_;
    unsigned raster_width_;
    unsigned raster_height_;
    int start_x_;
    int start_y_;
public:
    visitor_raster_processor(mapnik::raster & source,
                             mapnik::feature_impl & feature,
                             box2d<double> & target_ext,
                             box2d<double> & ext,
                             backend_type & backend,
                             bool & painted,
                             mapnik::proj_transform & prj_trans,
                             std::string const& image_format,
                             scaling_method_e scaling_method,
                             unsigned width,
                             unsigned height,
                             unsigned raster_width,
                             unsigned raster_height,
                             int start_x,
                             int start_y)
        : source_(source),
          feature_(feature),
          target_ext_(target_ext),
          ext_(ext),
          backend_(backend),
          painted_(painted),
          prj_trans_(prj_trans),
          image_format_(image_format),
          scaling_method_(scaling_method),
          width_(width),
          height_(height),
          raster_width_(raster_width),
          raster_height_(raster_height),
          start_x_(start_x),
          start_y_(start_y) {}
    
    void operator() (mapnik::image_rgba8 & source_data)
    {
        mapnik::image_rgba8 data(raster_width_, raster_height_);
        mapnik::raster target(target_ext_, data, source_.get_filter_factor());
        mapnik::premultiply_alpha(source_data);
        if (!prj_trans_.equal())
        {
            double offset_x = ext_.minx() - start_x_;
            double offset_y = ext_.miny() - start_y_;
            reproject_and_scale_raster(target, source_, prj_trans_,
                                       offset_x, offset_y,
                                       width_,
                                       scaling_method_);
        }
        else
        {
            double image_ratio_x = ext_.width() / source_data.width();
            double image_ratio_y = ext_.height() / source_data.height();
            scale_image_agg(util::get<image_rgba8>(target.data_),
                            source_data,
                            scaling_method_,
                            image_ratio_x,
                            image_ratio_y,
                            0.0,
                            0.0,
                            source_.get_filter_factor());
        }

        using pixfmt_type = agg::pixfmt_rgba32;
        using renderer_type = agg::renderer_base<pixfmt_type>;

        mapnik::image_any im_tile(width_, height_, mapnik::image_dtype_rgba8, true, true);
        agg::rendering_buffer dst_buffer(im_tile.getBytes(), im_tile.width(), im_tile.height(), im_tile.getRowSize());
        agg::rendering_buffer src_buffer(target.data_.getBytes(),target.data_.width(), target.data_.height(), target.data_.getRowSize());
        pixfmt_type src_pixf(src_buffer);
        pixfmt_type dst_pixf(dst_buffer);
        renderer_type ren(dst_pixf);
        ren.copy_from(src_pixf,0,start_x_, start_y_);
        backend_.start_tile_feature(feature_);
        backend_.add_tile_feature_raster(mapnik::save_to_string(im_tile,image_format_));
        painted_ = true;
    }

    void operator() (mapnik::image_gray8 & source_data)
    {
        mapnik::image_gray8 data(raster_width_, raster_height_);
        mapnik::raster target(target_ext_, data, source_.get_filter_factor());
        if (!prj_trans_.equal())
        {
            double offset_x = ext_.minx() - start_x_;
            double offset_y = ext_.miny() - start_y_;
            reproject_and_scale_raster(target, source_, prj_trans_,
                                       offset_x, offset_y,
                                       width_,
                                       scaling_method_);
        }
        else
        {
            double image_ratio_x = ext_.width() / source_data.width();
            double image_ratio_y = ext_.height() / source_data.height();
            scale_image_agg(util::get<image_gray8>(target.data_),
                            source_data,
                            scaling_method_,
                            image_ratio_x,
                            image_ratio_y,
                            0.0,
                            0.0,
                            source_.get_filter_factor());
        }

        using pixfmt_type = agg::pixfmt_gray8;
        using renderer_type = agg::renderer_base<pixfmt_type>;

        mapnik::image_any im_tile(width_, height_, mapnik::image_dtype_gray8);
        agg::rendering_buffer dst_buffer(im_tile.getBytes(), im_tile.width(), im_tile.height(), im_tile.getRowSize());
        agg::rendering_buffer src_buffer(target.data_.getBytes(),target.data_.width(), target.data_.height(), target.data_.getRowSize());
        pixfmt_type src_pixf(src_buffer);
        pixfmt_type dst_pixf(dst_buffer);
        renderer_type ren(dst_pixf);
        ren.copy_from(src_pixf,0,start_x_, start_y_);
        backend_.start_tile_feature(feature_);
        backend_.add_tile_feature_raster(mapnik::save_to_string(im_tile,image_format_));
        painted_ = true;
    }

    void operator() (mapnik::image_gray16 & source_data)
    {
        mapnik::image_gray16 data(raster_width_, raster_height_);
        mapnik::raster target(target_ext_, data, source_.get_filter_factor());
        if (!prj_trans_.equal())
        {
            double offset_x = ext_.minx() - start_x_;
            double offset_y = ext_.miny() - start_y_;
            reproject_and_scale_raster(target, source_, prj_trans_,
                                       offset_x, offset_y,
                                       width_,
                                       scaling_method_);
        }
        else
        {
            double image_ratio_x = ext_.width() / source_data.width();
            double image_ratio_y = ext_.height() / source_data.height();
            scale_image_agg(util::get<image_gray16>(target.data_),
                            source_data,
                            scaling_method_,
                            image_ratio_x,
                            image_ratio_y,
                            0.0,
                            0.0,
                            source_.get_filter_factor());
        }

        using pixfmt_type = agg::pixfmt_gray16;
        using renderer_type = agg::renderer_base<pixfmt_type>;

        mapnik::image_any im_tile(width_, height_, mapnik::image_dtype_gray16);
        agg::rendering_buffer dst_buffer(im_tile.getBytes(), im_tile.width(), im_tile.height(), im_tile.getRowSize());
        agg::rendering_buffer src_buffer(target.data_.getBytes(),target.data_.width(), target.data_.height(), target.data_.getRowSize());
        pixfmt_type src_pixf(src_buffer);
        pixfmt_type dst_pixf(dst_buffer);
        renderer_type ren(dst_pixf);
        ren.copy_from(src_pixf,0,start_x_, start_y_);
        backend_.start_tile_feature(feature_);
        backend_.add_tile_feature_raster(mapnik::save_to_string(im_tile,image_format_));
        painted_ = true;
    }

    void operator() (mapnik::image_gray32f & source_data)
    {
        mapnik::image_gray32f data(raster_width_, raster_height_);
        mapnik::raster target(target_ext_, data, source_.get_filter_factor());
        if (!prj_trans_.equal())
        {
            double offset_x = ext_.minx() - start_x_;
            double offset_y = ext_.miny() - start_y_;
            reproject_and_scale_raster(target, source_, prj_trans_,
                                       offset_x, offset_y,
                                       width_,
                                       scaling_method_);
        }
        else
        {
            double image_ratio_x = ext_.width() / source_data.width();
            double image_ratio_y = ext_.height() / source_data.height();
            scale_image_agg(util::get<image_gray32f>(target.data_),
                            source_data,
                            scaling_method_,
                            image_ratio_x,
                            image_ratio_y,
                            0.0,
                            0.0,
                            source_.get_filter_factor());
        }

        using pixfmt_type = agg::pixfmt_gray32;
        using renderer_type = agg::renderer_base<pixfmt_type>;

        mapnik::image_any im_tile(width_, height_, mapnik::image_dtype_gray32f);
        agg::rendering_buffer dst_buffer(im_tile.getBytes(), im_tile.width(), im_tile.height(), im_tile.getRowSize());
        agg::rendering_buffer src_buffer(target.data_.getBytes(),target.data_.width(), target.data_.height(), target.data_.getRowSize());
        pixfmt_type src_pixf(src_buffer);
        pixfmt_type dst_pixf(dst_buffer);
        renderer_type ren(dst_pixf);
        ren.copy_from(src_pixf,0,start_x_, start_y_);
        backend_.start_tile_feature(feature_);
        backend_.add_tile_feature_raster(mapnik::save_to_string(im_tile,image_format_));
        painted_ = true;
    }

    void operator() (image_null &) const
    {
        throw std::runtime_error("Null data passed to visitor");
    }

};
#endif

/*
  This processor combines concepts from mapnik's
  feature_style_processor and agg_renderer. It
  differs in that here we only process layers in
  isolation of their styles, and because of this we need
  options for clipping and simplification, for example,
  that would normally come from a style's symbolizers
*/

template <typename T>
class processor : private MAPNIK_NONCOPYABLE
{
public:
    typedef T backend_type;
private:
    backend_type & backend_;
    mapnik::Map const& m_;
    mapnik::request const& m_req_;
    double scale_factor_;
    MAPNIK_VIEW_TRANSFORM t_;
    unsigned tolerance_;
    std::string image_format_;
    scaling_method_e scaling_method_;
    bool painted_;
public:
    processor(T & backend,
              mapnik::Map const& map,
              mapnik::request const& m_req,
              double scale_factor=1.0,
              unsigned offset_x=0,
              unsigned offset_y=0,
              unsigned tolerance=1,
              std::string const& image_format="jpeg",
              scaling_method_e scaling_method=SCALING_NEAR
        )
        : backend_(backend),
          m_(map),
          m_req_(m_req),
          scale_factor_(scale_factor),
          t_(m_req.width(),m_req.height(),m_req.extent(),offset_x,offset_y),
        tolerance_(tolerance),
        image_format_(image_format),
        scaling_method_(scaling_method),
        painted_(false) {}

    void apply(double scale_denom=0.0)
    {
        mapnik::projection proj(m_.srs(),true);
        if (scale_denom <= 0.0)
        {
            scale_denom = mapnik::scale_denominator(m_req_.scale(),proj.is_geographic());
        }
        scale_denom *= scale_factor_;
        BOOST_FOREACH ( mapnik::layer const& lay, m_.layers() )
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

    bool painted() const
    {
        return painted_;
    }

    void apply_to_layer(mapnik::layer const& lay,
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
        BOOST_FOREACH(mapnik::attribute_descriptor const& desc, lay_desc.get_descriptors())
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
#if MAPNIK_VERSION >= 300000
                    visitor_raster_processor<T> visit(*source,
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
#else
                    raster target(target_ext, raster_width, raster_height);
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
                                                   2.0,
                                                   scaling_method_);
                    }
                    else
                    {
                        double image_ratio_x = ext.width() / source->data_.width();
                        double image_ratio_y = ext.height() / source->data_.height();
                        scale_image_agg<image_data_32>(target.data_,
                                                       source->data_,
                                                       scaling_method_,
                                                       image_ratio_x,
                                                       image_ratio_y,
                                                       0.0,
                                                       0.0,
                                                       2.0);
                    }
                    mapnik::image_data_32 im_tile(width,height);
                    composite(im_tile, target.data_,
                              src_over, 1,
                              start_x, start_y, false);
                    agg::rendering_buffer buffer(im_tile.getBytes(),
                                                 im_tile.width(),
                                                 im_tile.height(),
                                                 im_tile.width() * 4);
                    agg::pixfmt_rgba32 pixf(buffer);
                    pixf.demultiply();
                    backend_.start_tile_feature(*feature);
                    backend_.add_tile_feature_raster(mapnik::save_to_string(im_tile,image_format_));
                    painted_ = true;
#endif
                }
                backend_.stop_tile_layer();
                return;
            }
            // vector pathway
            while (feature)
            {
                boost::ptr_vector<mapnik::geometry_type> & paths = feature->paths();
                if (paths.empty()) {
                    feature = features->next();
                    continue;
                }
                backend_.start_tile_feature(*feature);
                BOOST_FOREACH( mapnik::geometry_type & geom, paths)
                {
                    mapnik::box2d<double> geom_box = geom.envelope();
                    if (!geom_box.intersects(buffered_query_ext))
                    {
                        continue;
                    }
                    if (handle_geometry(geom,
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

    unsigned handle_geometry(mapnik::geometry_type & geom,
                             mapnik::proj_transform const& prj_trans,
                             mapnik::box2d<double> const& buffered_query_ext)
    {
        unsigned path_count = 0;
        switch (geom.type())
        {
        case MAPNIK_POINT:
        {
            if (geom.size() > 0)
            {
                typedef MAPNIK_TRANSFORM_PATH<MAPNIK_VIEW_TRANSFORM,
                                              mapnik::geometry_type> path_type;
                path_type path(t_, geom, prj_trans);
                path_count = backend_.add_path(path, tolerance_, geom.type());
            }
            break;
        }
        case MAPNIK_LINESTRING:
        {
            if (geom.size() > 1)
            {
                typedef agg::conv_clip_polyline<mapnik::geometry_type> line_clipper;
                line_clipper clipped(geom);
                clipped.clip_box(
                    buffered_query_ext.minx(),
                    buffered_query_ext.miny(),
                    buffered_query_ext.maxx(),
                    buffered_query_ext.maxy());
                typedef MAPNIK_TRANSFORM_PATH<MAPNIK_VIEW_TRANSFORM, line_clipper> path_type;
                path_type path(t_, clipped, prj_trans);
                path_count = backend_.add_path(path, tolerance_, geom.type());
            }
            break;
        }
        case MAPNIK_POLYGON:
        {
            if (geom.size() > 2)
            {
#ifdef CONV_CLIPPER
                agg::path_storage ps;
                ps.move_to(buffered_query_ext.minx(), buffered_query_ext.miny());
                ps.line_to(buffered_query_ext.minx(), buffered_query_ext.maxy());
                ps.line_to(buffered_query_ext.maxx(), buffered_query_ext.maxy());
                ps.line_to(buffered_query_ext.maxx(), buffered_query_ext.miny());
                ps.close_polygon();
                typedef agg::conv_clipper<mapnik::geometry_type, agg::path_storage> poly_clipper;
                poly_clipper clipped(geom,ps,
                                     agg::clipper_and,
                                     agg::clipper_non_zero,
                                     agg::clipper_non_zero,
                                     1);
                //clipped.rewind(0);
#else
                typedef agg::conv_clip_polygon<mapnik::geometry_type> poly_clipper;
                poly_clipper clipped(geom);
                clipped.clip_box(
                    buffered_query_ext.minx(),
                    buffered_query_ext.miny(),
                    buffered_query_ext.maxx(),
                    buffered_query_ext.maxy());
#endif
                typedef MAPNIK_TRANSFORM_PATH<MAPNIK_VIEW_TRANSFORM, poly_clipper> path_type;
                path_type path(t_, clipped, prj_trans);
                path_count = backend_.add_path(path, tolerance_, geom.type());
            }
            break;
        }
        case MAPNIK_UNKNOWN:
        default:
        {
            throw std::runtime_error("unhandled geometry type");
            break;
        }
        }
        return path_count;
    }
};

}} // end ns

#endif // __MAPNIK_VECTOR_TILE_PROCESSOR_H__
