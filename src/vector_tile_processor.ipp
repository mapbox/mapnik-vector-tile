#include <mapnik/map.hpp>
#include <mapnik/request.hpp>
#include <mapnik/layer.hpp>
#include <mapnik/query.hpp>
#include <mapnik/geometry.hpp>
#include <mapnik/vertex_adapters.hpp>
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
#include <mapnik/geometry_is_empty.hpp>
#include <mapnik/geometry_envelope.hpp>
#include <mapnik/geometry_adapters.hpp>
//#include <mapnik/simplify.hpp>
//#include <mapnik/simplify_converter.hpp>
#include <mapnik/geometry_correct.hpp>
#include <boost/geometry/algorithms/unique.hpp>

// agg
#include "agg_path_storage.h"

// agg core clipper: http://www.angusj.com/delphi/clipper.php
#include "agg_conv_clipper.h"

// angus clipper
#include "agg_conv_clip_polygon.h"

#include "agg_conv_clip_polyline.h"
#include "agg_rendering_buffer.h"
#include "agg_pixfmt_rgba.h"
#include "agg_pixfmt_gray.h"
#include "agg_renderer_base.h"

#include <boost/optional.hpp>
#include <boost/geometry/algorithms/simplify.hpp>

#include <iostream>
#include <string>
#include <stdexcept>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "vector_tile.pb.h"
#pragma GCC diagnostic pop

namespace mapnik { namespace vector_tile_impl {

template <typename T>
struct visitor_raster_processor
{
public:
    typedef T backend_type;
private:
    mapnik::raster const& source_;
    mapnik::feature_impl const& feature_;
    box2d<double> const& target_ext_;
    box2d<double> const& ext_;
    backend_type & backend_;
    bool & painted_;
    mapnik::proj_transform const& prj_trans_;
    std::string const& image_format_;
    scaling_method_e scaling_method_;
    unsigned width_;
    unsigned height_;
    unsigned raster_width_;
    unsigned raster_height_;
    int start_x_;
    int start_y_;
public:
    visitor_raster_processor(mapnik::raster const& source,
                             mapnik::feature_impl const& feature,
                             box2d<double> const& target_ext,
                             box2d<double> const& ext,
                             backend_type & backend,
                             bool & painted,
                             mapnik::proj_transform const& prj_trans,
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

    void operator() (mapnik::image_gray8s & source_data)
    {
        mapnik::image_gray8s data(raster_width_, raster_height_);
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
            scale_image_agg(util::get<image_gray8s>(target.data_),
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

        mapnik::image_any im_tile(width_, height_, mapnik::image_dtype_gray8s);
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

    void operator() (mapnik::image_gray16s & source_data)
    {
        mapnik::image_gray16s data(raster_width_, raster_height_);
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
            scale_image_agg(util::get<image_gray16s>(target.data_),
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

        mapnik::image_any im_tile(width_, height_, mapnik::image_dtype_gray16s);
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

    void operator() (mapnik::image_gray32 & source_data)
    {
        mapnik::image_gray32 data(raster_width_, raster_height_);
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
            scale_image_agg(util::get<image_gray32>(target.data_),
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

        mapnik::image_any im_tile(width_, height_, mapnik::image_dtype_gray32);
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

    void operator() (mapnik::image_gray32s & source_data)
    {
        mapnik::image_gray32s data(raster_width_, raster_height_);
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
            scale_image_agg(util::get<image_gray32s>(target.data_),
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

        mapnik::image_any im_tile(width_, height_, mapnik::image_dtype_gray32s);
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

    void operator() (mapnik::image_gray64 & source_data)
    {
        mapnik::image_gray64 data(raster_width_, raster_height_);
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
            scale_image_agg(util::get<image_gray64>(target.data_),
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

        mapnik::image_any im_tile(width_, height_, mapnik::image_dtype_gray64);
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

    void operator() (mapnik::image_gray64s & source_data)
    {
        mapnik::image_gray64s data(raster_width_, raster_height_);
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
            scale_image_agg(util::get<image_gray64s>(target.data_),
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

        mapnik::image_any im_tile(width_, height_, mapnik::image_dtype_gray64s);
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

    void operator() (mapnik::image_gray64f & source_data)
    {
        mapnik::image_gray64f data(raster_width_, raster_height_);
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
            scale_image_agg(util::get<image_gray64f>(target.data_),
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

        mapnik::image_any im_tile(width_, height_, mapnik::image_dtype_gray64f);
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
      poly_clipper_type_(AGG_CLIPPER),
      simplify_distance_(0.0) {}

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
            }
            backend_.stop_tile_layer();
            return;
        }
        // vector pathway
        while (feature)
        {
            mapnik::geometry::geometry const& geom = feature->get_geometry();
            if (mapnik::geometry::is_empty(geom))
            {
                feature = features->next();
                continue;
            }
            if (geom.is<mapnik::geometry::geometry_collection>())
            {
                std::clog << "collection\n";
                auto const& collection = mapnik::util::get<mapnik::geometry::geometry_collection>(geom);
                for (auto const& part : collection)
                {
                    if (handle_geometry(*feature,
                                        part,
                                        prj_trans,
                                        buffered_query_ext) > 0)
                    {
                        painted_ = true;
                    }
                }
            }
            else
            {
                if (handle_geometry(*feature,
                                    geom,
                                    prj_trans,
                                    buffered_query_ext) > 0)
                {
                    painted_ = true;
                }
            }
            feature = features->next();
        }
        backend_.stop_tile_layer();
    }
}

template <typename T>
struct encoder_visitor {
    typedef T backend_type;
    encoder_visitor(backend_type & backend,
                    mapnik::feature_impl const& feature,
                    mapnik::proj_transform const& prj_trans,
                    mapnik::box2d<double> const& buffered_query_ext,
                    mapnik::view_transform const& t,
                    unsigned tolerance) :
      backend_(backend),
      feature_(feature),
      prj_trans_(prj_trans),
      buffered_query_ext_(buffered_query_ext),
      t_(t),
      tolerance_(tolerance) {}

    unsigned operator() (mapnik::geometry::point const& geom)
    {
        unsigned path_count = 0;
        if (buffered_query_ext_.intersects(geom.x,geom.y))
        {
            backend_.start_tile_feature(feature_);
            backend_.current_feature_->set_type(vector_tile::Tile_GeomType_POINT);
            using va_type = mapnik::geometry::point_vertex_adapter;
            using path_type = mapnik::transform_path_adapter<mapnik::view_transform,va_type>;
            va_type va(geom);
            path_type path(t_, va, prj_trans_);
            path_count = backend_.add_path(path, tolerance_);
            backend_.stop_tile_feature();
        }
        return path_count;
    }

    unsigned operator() (mapnik::geometry::multi_point const& geom)
    {
        unsigned path_count = 0;
        mapnik::box2d<double> bbox = mapnik::geometry::envelope(geom);
        if (buffered_query_ext_.intersects(bbox))
        {
            backend_.start_tile_feature(feature_);
            backend_.current_feature_->set_type(vector_tile::Tile_GeomType_POINT);
            using va_type = mapnik::geometry::point_vertex_adapter;
            using path_type = mapnik::transform_path_adapter<mapnik::view_transform,va_type>;
            for (auto const& pt : geom)
            {
                if (buffered_query_ext_.intersects(pt.x,pt.y))
                {
                    va_type va(pt);
                    path_type path(t_, va, prj_trans_);
                    path_count += backend_.add_path(path, tolerance_);
                }
            }
            backend_.stop_tile_feature();
        }
        return path_count;
    }

    unsigned operator() (mapnik::geometry::line_string const& geom)
    {
        unsigned path_count = 0;
        mapnik::box2d<double> bbox = mapnik::geometry::envelope(geom);
        if (buffered_query_ext_.intersects(bbox))
        {
            if (geom.size() > 1)
            {
                backend_.start_tile_feature(feature_);
                backend_.current_feature_->set_type(vector_tile::Tile_GeomType_LINESTRING);
                using va_type = mapnik::geometry::line_string_vertex_adapter;
                using clip_type = agg::conv_clip_polyline<va_type>;

                va_type va(geom);
                clip_type clipped(va);
                clipped.clip_box(
                    buffered_query_ext_.minx(),
                    buffered_query_ext_.miny(),
                    buffered_query_ext_.maxx(),
                    buffered_query_ext_.maxy());
                using path_type = mapnik::transform_path_adapter<mapnik::view_transform,clip_type>;
                path_type path(t_, clipped, prj_trans_);
                path_count = backend_.add_path(path, tolerance_);
                backend_.stop_tile_feature();
            }
        }
        return path_count;
    }

    unsigned operator() (mapnik::geometry::multi_line_string const& geom)
    {
        unsigned path_count = 0;
        mapnik::box2d<double> bbox = mapnik::geometry::envelope(geom);
        if (buffered_query_ext_.intersects(bbox))
        {
            backend_.start_tile_feature(feature_);
            backend_.current_feature_->set_type(vector_tile::Tile_GeomType_LINESTRING);
            using va_type = mapnik::geometry::line_string_vertex_adapter;
            using clip_type = agg::conv_clip_polyline<va_type>;
            for (auto const& line : geom)
            {
                mapnik::box2d<double> bbox = mapnik::geometry::envelope(line);
                if (line.size() > 1 && buffered_query_ext_.intersects(bbox))
                {
                    va_type va(line);
                    clip_type clipped(va);
                    clipped.clip_box(
                        buffered_query_ext_.minx(),
                        buffered_query_ext_.miny(),
                        buffered_query_ext_.maxx(),
                        buffered_query_ext_.maxy());
                    using path_type = mapnik::transform_path_adapter<mapnik::view_transform,clip_type>;
                    path_type path(t_, clipped, prj_trans_);
                    path_count += backend_.add_path(path, tolerance_);
                }
            }
            backend_.stop_tile_feature();
        }
        return path_count;
    }

    unsigned operator() (mapnik::geometry::polygon const& geom)
    {
        return 0;
        std::clog << "skippPPPING\n";
        /*
        unsigned path_count = 0;
        mapnik::box2d<double> bbox = mapnik::geometry::envelope(geom);
        if (buffered_query_ext_.intersects(bbox))
        {
            if (geom.exterior_ring.size() > 3)
            {
                using va_type = mapnik::geometry::polygon_vertex_adapter;
                if (true)
                {
                    std::clog << "HERERERERERER\n";
                    backend_.start_tile_feature(feature_);
                    backend_.current_feature_->set_type(vector_tile::Tile_GeomType_POLYGON);
                    using clip_type = agg::conv_clip_polygon<va_type>;
                    va_type va(geom);
                    clip_type clipped(va);
                    clipped.clip_box(
                        buffered_query_ext_.minx(),
                        buffered_query_ext_.miny(),
                        buffered_query_ext_.maxx(),
                        buffered_query_ext_.maxy());
                    using path_type = mapnik::transform_path_adapter<mapnik::view_transform,clip_type>;
                    path_type path(t_, clipped, prj_trans_);
                    path_count = backend_.add_path(path, tolerance_);
                    backend_.stop_tile_feature();
                }
                else
                {
                    double mult = 1000000.0;
                    ClipperLib::Clipper clipper;
                    std::vector<ClipperLib::IntPoint> path;
                    //path.reserve(poly.exterior_ring.size());
                    for (auto const& pt : geom.exterior_ring)
                    {
                        double x = static_cast<ClipperLib::cInt>(pt.x*mult);
                        double y = static_cast<ClipperLib::cInt>(pt.y*mult);
                        path.emplace_back(x,y);
                    }
                    if (!clipper.AddPath(path, ClipperLib::ptSubject, true))
                    {
                        std::clog << "ptSubject ext failed!\n";
                        //continue;
                    }
                    for (auto const& ring : geom.interior_rings)
                    {
                        path.clear();
                        for (auto const& pt : ring)
                        {
                            double x = static_cast<ClipperLib::cInt>(pt.x*mult);
                            double y = static_cast<ClipperLib::cInt>(pt.y*mult);
                            path.emplace_back(x,y);
                        }
                        if (!clipper.AddPath(path, ClipperLib::ptSubject, true))
                        {
                            std::clog << "ptSubject ext failed!\n";
                        }
                    }
                    std::vector<ClipperLib::IntPoint> clip_box;
                    clip_box.emplace_back(static_cast<ClipperLib::cInt>(buffered_query_ext_.minx()*mult),static_cast<ClipperLib::cInt>(buffered_query_ext_.miny()*mult));
                    clip_box.emplace_back(static_cast<ClipperLib::cInt>(buffered_query_ext_.maxx()*mult),static_cast<ClipperLib::cInt>(buffered_query_ext_.miny()*mult));
                    clip_box.emplace_back(static_cast<ClipperLib::cInt>(buffered_query_ext_.maxx()*mult),static_cast<ClipperLib::cInt>(buffered_query_ext_.maxy()*mult));
                    clip_box.emplace_back(static_cast<ClipperLib::cInt>(buffered_query_ext_.minx()*mult),static_cast<ClipperLib::cInt>(buffered_query_ext_.maxy()*mult));
                    clip_box.emplace_back(static_cast<ClipperLib::cInt>(buffered_query_ext_.minx()*mult),static_cast<ClipperLib::cInt>(buffered_query_ext_.miny()*mult));

                    if (!clipper.AddPath( clip_box, ClipperLib::ptClip, true ))
                    {
                        std::clog << "ptClip failed!\n";
                    }

                    ClipperLib::PolyTree polygons;
                    clipper.Execute(ClipperLib::ctIntersection, polygons, ClipperLib::pftNonZero, ClipperLib::pftNonZero);
                    clipper.Clear();
                    ClipperLib::PolyNode* polynode = polygons.GetFirst();
                    mapnik::geometry::multi_polygon mp;
                    mp.emplace_back();
                    bool first = true;
                    while (polynode)
                    {
                        //do stuff with polynode here
                        if (first) first = false;
                        else mp.emplace_back();
                        if (!polynode->IsHole())
                        {
                            for (auto const& pt : polynode->Contour)
                            {
                                mp.back().exterior_ring.add_coord(pt.X/mult, pt.Y/mult);
                            }
                        }
                        else
                        {
                            mapnik::geometry::linear_ring hole;
                            for (auto const& pt : polynode->Contour)
                            {
                                hole.add_coord(pt.X/mult, pt.Y/mult);
                            }
                            mp.back().add_hole(std::move(hole));
                        }
                        //std::cerr << "Is hole? " << polynode->IsHole() << std::endl;
                        polynode = polynode->GetNext();
                    }
                    mapnik::geometry::correct(mp);
                    for (auto const& poly : mp)
                    {
                        mapnik::box2d<double> bbox = mapnik::geometry::envelope(poly);
                        if (poly.exterior_ring.size() > 3 && buffered_query_ext_.intersects(bbox))
                        {
                            va_type va(poly);
                            using path_type = mapnik::transform_path_adapter<mapnik::view_transform,va_type>;
                            path_type path(t_, va, prj_trans_);
                            path_count += backend_.add_path(path, tolerance_);
                        }
                    }

                }
            }
        }
        return path_count;
        */
    }

    unsigned operator() (mapnik::geometry::multi_polygon const& geom)
    {
        unsigned path_count = 0;
        mapnik::box2d<double> bbox = mapnik::geometry::envelope(geom);
        if (buffered_query_ext_.intersects(bbox))
        {
            backend_.start_tile_feature(feature_);
            backend_.current_feature_->set_type(vector_tile::Tile_GeomType_POLYGON);
            using va_type = mapnik::geometry::polygon_vertex_adapter;
            if (false)
            {
                for (auto const& poly : geom)
                {
                    mapnik::box2d<double> bbox = mapnik::geometry::envelope(poly);
                    if (poly.exterior_ring.size() > 3 && buffered_query_ext_.intersects(bbox))
                    {
                            using clip_type = agg::conv_clip_polygon<va_type>;
                            using path_type = mapnik::transform_path_adapter<mapnik::view_transform,clip_type>;
                            va_type va(poly);
                            clip_type clipped(va);
                            clipped.clip_box(
                                buffered_query_ext_.minx(),
                                buffered_query_ext_.miny(),
                                buffered_query_ext_.maxx(),
                                buffered_query_ext_.maxy());
                            using path_type = mapnik::transform_path_adapter<mapnik::view_transform,clip_type>;
                            path_type path(t_, clipped, prj_trans_);
                            path_count += backend_.add_path(path, tolerance_);
                    }
                }
            }
            else
            {
                double mult = 1000000.0;
                ClipperLib::Clipper clipper;
                for (auto const& poly : geom)
                {
                    //mapnik::box2d<double> bbox = mapnik::geometry::envelope(poly);
                    if (poly.exterior_ring.size() > 3 /*&& buffered_query_ext_.intersects(bbox)*/)
                    {
                        std::vector<ClipperLib::IntPoint> path;
                        //path.reserve(poly.exterior_ring.size());
                        for (auto const& pt : poly.exterior_ring)
                        {
                            double x = static_cast<ClipperLib::cInt>(pt.x*mult);
                            double y = static_cast<ClipperLib::cInt>(pt.y*mult);
                            path.emplace_back(x,y);
                        }
                        if (!clipper.AddPath(path, ClipperLib::ptSubject, true))
                        {
                            std::clog << "ptSubject ext failed " << path.size() << "\n";
                            continue;
                        }
                        for (auto const& ring : poly.interior_rings)
                        {
                            if (ring.size() < 4) continue;
                            path.clear();
                            for (auto const& pt : ring)
                            {
                                double x = static_cast<ClipperLib::cInt>(pt.x*mult);
                                double y = static_cast<ClipperLib::cInt>(pt.y*mult);
                                path.emplace_back(x,y);
                            }
                            if (!clipper.AddPath(path, ClipperLib::ptSubject, true))
                            {
                                std::clog << "ptSubject hole failed! " << path.size() << " " << ring.size() << "\n";
                            }
                        }
                    }
                }
                ClipperLib::PolyTree polygons;
                //clipper.Execute(ClipperLib::ctUnion, polygons, ClipperLib::pftNonZero);
                //std::cerr << "path size=" << path.size() << std::endl;
                std::vector<ClipperLib::IntPoint> clip_box;
                clip_box.emplace_back(static_cast<ClipperLib::cInt>(buffered_query_ext_.minx()*mult),static_cast<ClipperLib::cInt>(buffered_query_ext_.miny()*mult));
                clip_box.emplace_back(static_cast<ClipperLib::cInt>(buffered_query_ext_.maxx()*mult),static_cast<ClipperLib::cInt>(buffered_query_ext_.miny()*mult));
                clip_box.emplace_back(static_cast<ClipperLib::cInt>(buffered_query_ext_.maxx()*mult),static_cast<ClipperLib::cInt>(buffered_query_ext_.maxy()*mult));
                clip_box.emplace_back(static_cast<ClipperLib::cInt>(buffered_query_ext_.minx()*mult),static_cast<ClipperLib::cInt>(buffered_query_ext_.maxy()*mult));
                clip_box.emplace_back(static_cast<ClipperLib::cInt>(buffered_query_ext_.minx()*mult),static_cast<ClipperLib::cInt>(buffered_query_ext_.miny()*mult));

                if (!clipper.AddPath( clip_box, ClipperLib::ptClip, true ))
                {
                    std::clog << "ptClip failed!\n";
                }

                clipper.Execute(ClipperLib::ctIntersection, polygons, ClipperLib::pftNonZero, ClipperLib::pftNonZero);
                clipper.Clear();
                ClipperLib::PolyNode* polynode = polygons.GetFirst();
                mapnik::geometry::multi_polygon mp;
                mp.emplace_back();
                bool first = true;

                while (polynode)
                {
                    if (!polynode->IsHole())
                    {
                        if (first) first = false;
                        else mp.emplace_back(); // start new polygon
                        for (auto const& pt : polynode->Contour)
                        {
                            mp.back().exterior_ring.add_coord(pt.X, pt.Y);
                        }
                        // children of exterior ring are always interior rings
                        for (auto const* ring : polynode->Childs)
                        {
                            mapnik::geometry::linear_ring hole;
                            for (auto const& pt : ring->Contour)
                            {
                                hole.add_coord(pt.X, pt.Y);
                            }
                            mp.back().add_hole(std::move(hole));
                        }
                    }
                    polynode = polynode->GetNext();
                }
                mapnik::geometry::correct(mp);

                for (auto const& poly : mp)
                {
                    //mapnik::box2d<double> bbox = mapnik::geometry::envelope(poly);
                    //if (poly.exterior_ring.size() > 3 && buffered_query_ext_.intersects(bbox))
                    //{
                        va_type va(poly);
                        using path_type = mapnik::transform_path_adapter<mapnik::view_transform,va_type>;
                        path_type path(t_, va, prj_trans_);
                        path_count += backend_.add_path(path, tolerance_);
                    //}
                }
            }
            backend_.stop_tile_feature();
        }
        return path_count;
    }

    template <typename Geom>
    unsigned operator() (Geom const& geom) {
        return 0;
    }

    backend_type & backend_;
    mapnik::feature_impl const& feature_;
    mapnik::proj_transform const& prj_trans_;
    mapnik::box2d<double> const& buffered_query_ext_;
    mapnik::view_transform const& t_;
    unsigned tolerance_;
};

template <typename T>
struct simplify_visitor {
    typedef T backend_type;
    simplify_visitor(double simplify_distance,
                     encoder_visitor<backend_type> & encoder) :
      encoder_(encoder),
      simplify_distance_(simplify_distance) {}

    unsigned operator() (mapnik::geometry::point const& geom)
    {
        return encoder_(geom);
    }

    unsigned operator() (mapnik::geometry::multi_point const& geom)
    {
        return encoder_(geom);
    }

    unsigned operator() (mapnik::geometry::line_string const& geom)
    {
        mapnik::geometry::line_string simplified;
        boost::geometry::simplify(geom,simplified,simplify_distance_);
        mapnik::geometry::correct(simplified);
        boost::geometry::unique(simplified);
        return encoder_(simplified);
    }

    unsigned operator() (mapnik::geometry::multi_line_string const& geom)
    {
        mapnik::geometry::multi_line_string simplified;
        boost::geometry::simplify(geom,simplified,simplify_distance_);
        mapnik::geometry::correct(simplified);
        boost::geometry::unique(simplified);
        return encoder_(simplified);
    }

    unsigned operator() (mapnik::geometry::polygon const& geom)
    {
        mapnik::geometry::polygon simplified;
        boost::geometry::simplify(geom,simplified,simplify_distance_);
        mapnik::geometry::correct(simplified);
        boost::geometry::unique(simplified);
        return encoder_(simplified);
    }

    unsigned operator() (mapnik::geometry::multi_polygon const& geom)
    {
        mapnik::geometry::multi_polygon simplified;
        boost::geometry::simplify(geom,simplified,simplify_distance_);
        mapnik::geometry::correct(simplified);
        boost::geometry::unique(simplified);
        return encoder_(simplified);
    }

    template <typename Geom>
    unsigned operator() (Geom const& geom) {
        return encoder_(geom);
    }

    encoder_visitor<backend_type> & encoder_;
    unsigned simplify_distance_;
};


template <typename T>
unsigned processor<T>::handle_geometry(mapnik::feature_impl const& feature,
                                       mapnik::geometry::geometry const& geom,
                                       mapnik::proj_transform const& prj_trans,
                                       mapnik::box2d<double> const& buffered_query_ext)
{
    encoder_visitor<T> encoder(backend_,feature,prj_trans,buffered_query_ext,t_,tolerance_);
    if (simplify_distance_ > 0)
    {
        simplify_visitor<T> simplifier(simplify_distance_,encoder);
        return mapnik::util::apply_visitor(simplifier,geom);
    }
    else
    {
        return mapnik::util::apply_visitor(encoder,geom);
    }
}

}} // end ns
