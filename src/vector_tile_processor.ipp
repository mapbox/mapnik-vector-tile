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
#include <mapnik/geometry_adapters.hpp>
#include <mapnik/geometry_transform.hpp>

// http://www.angusj.com/delphi/clipper.php
#include "clipper.hpp"

#include "agg_rendering_buffer.h"
#include "agg_pixfmt_rgba.h"
#include "agg_pixfmt_gray.h"
#include "agg_renderer_base.h"

#include <boost/optional.hpp>
#include <boost/geometry/algorithms/simplify.hpp>
#include <boost/geometry/algorithms/intersection.hpp>

#include <iostream>
#include <string>
#include <stdexcept>

#include "vector_tile_strategy.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include "vector_tile.pb.h"
#pragma GCC diagnostic pop

namespace mapnik { namespace vector_tile_impl {

template <typename T>
struct visitor_raster_processor
{
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
        agg::rendering_buffer dst_buffer(im_tile.bytes(), im_tile.width(), im_tile.height(), im_tile.row_size());
        agg::rendering_buffer src_buffer(target.data_.bytes(),target.data_.width(), target.data_.height(), target.data_.row_size());
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
        agg::rendering_buffer dst_buffer(im_tile.bytes(), im_tile.width(), im_tile.height(), im_tile.row_size());
        agg::rendering_buffer src_buffer(target.data_.bytes(),target.data_.width(), target.data_.height(), target.data_.row_size());
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
        agg::rendering_buffer dst_buffer(im_tile.bytes(), im_tile.width(), im_tile.height(), im_tile.row_size());
        agg::rendering_buffer src_buffer(target.data_.bytes(),target.data_.width(), target.data_.height(), target.data_.row_size());
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
        agg::rendering_buffer dst_buffer(im_tile.bytes(), im_tile.width(), im_tile.height(), im_tile.row_size());
        agg::rendering_buffer src_buffer(target.data_.bytes(),target.data_.width(), target.data_.height(), target.data_.row_size());
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
        agg::rendering_buffer dst_buffer(im_tile.bytes(), im_tile.width(), im_tile.height(), im_tile.row_size());
        agg::rendering_buffer src_buffer(target.data_.bytes(),target.data_.width(), target.data_.height(), target.data_.row_size());
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
        agg::rendering_buffer dst_buffer(im_tile.bytes(), im_tile.width(), im_tile.height(), im_tile.row_size());
        agg::rendering_buffer src_buffer(target.data_.bytes(),target.data_.width(), target.data_.height(), target.data_.row_size());
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
        agg::rendering_buffer dst_buffer(im_tile.bytes(), im_tile.width(), im_tile.height(), im_tile.row_size());
        agg::rendering_buffer src_buffer(target.data_.bytes(),target.data_.width(), target.data_.height(), target.data_.row_size());
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
        agg::rendering_buffer dst_buffer(im_tile.bytes(), im_tile.width(), im_tile.height(), im_tile.row_size());
        agg::rendering_buffer src_buffer(target.data_.bytes(),target.data_.width(), target.data_.height(), target.data_.row_size());
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
        agg::rendering_buffer dst_buffer(im_tile.bytes(), im_tile.width(), im_tile.height(), im_tile.row_size());
        agg::rendering_buffer src_buffer(target.data_.bytes(),target.data_.width(), target.data_.height(), target.data_.row_size());
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
        agg::rendering_buffer dst_buffer(im_tile.bytes(), im_tile.width(), im_tile.height(), im_tile.row_size());
        agg::rendering_buffer src_buffer(target.data_.bytes(),target.data_.width(), target.data_.height(), target.data_.row_size());
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
        agg::rendering_buffer dst_buffer(im_tile.bytes(), im_tile.width(), im_tile.height(), im_tile.row_size());
        agg::rendering_buffer src_buffer(target.data_.bytes(),target.data_.width(), target.data_.height(), target.data_.row_size());
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
      fill_type_(ClipperLib::pftNonZero) {}

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

inline void process_polynode_branch(ClipperLib::PolyNode* polynode, 
                             mapnik::geometry::multi_polygon<std::int64_t> & mp,
                             double area_threshold)
{
    mapnik::geometry::polygon<std::int64_t> polygon;
    polygon.set_exterior_ring(std::move(polynode->Contour));
    if (polygon.exterior_ring.size() > 2) // Throw out invalid polygons
    {
        double outer_area = ClipperLib::Area(polygon.exterior_ring);
        if (std::abs(outer_area) >= area_threshold)
        {
            // The view transform inverts the y axis so this should be positive still despite now
            // being clockwise for the exterior ring. If it is not lets invert it.
            if (outer_area < 0)
            {   
                std::reverse(polygon.exterior_ring.begin(), polygon.exterior_ring.end());
            }
            
            // children of exterior ring are always interior rings
            for (auto * ring : polynode->Childs)
            {
                if (ring->Contour.size() < 3)
                {
                    continue; // Throw out invalid holes
                }
                double inner_area = ClipperLib::Area(ring->Contour);
                if (std::abs(inner_area) < area_threshold)
                {
                    continue;
                }
                
                if (inner_area > 0)
                {
                    std::reverse(ring->Contour.begin(), ring->Contour.end());
                }
                polygon.add_hole(std::move(ring->Contour));
            }
            mp.emplace_back(std::move(polygon));
        }
    }
    for (auto * ring : polynode->Childs)
    {
        for (auto * sub_ring : ring->Childs)
        {
            process_polynode_branch(sub_ring, mp, area_threshold);
        }
    }
}

template <typename T>
struct encoder_visitor 
{
    typedef T backend_type;
    encoder_visitor(backend_type & backend,
                    mapnik::feature_impl const& feature,
                    mapnik::box2d<int> const& tile_clipping_extent,
                    double area_threshold,
                    bool strictly_simple,
                    bool multi_polygon_union,
                    ClipperLib::PolyFillType fill_type) :
      backend_(backend),
      feature_(feature),
      tile_clipping_extent_(tile_clipping_extent),
      area_threshold_(area_threshold),
      strictly_simple_(strictly_simple),
      multi_polygon_union_(multi_polygon_union),
      fill_type_(fill_type) {}

    bool operator() (mapnik::geometry::geometry_empty const&)
    {
        return false;
    }

    bool operator() (mapnik::geometry::geometry_collection<std::int64_t> & geom)
    {
        bool painted = false;
        for (auto & g : geom)
        {
            if (mapnik::util::apply_visitor((*this), g))
            {
                painted = true;
            }
        }
        return painted;
    }

    bool operator() (mapnik::geometry::point<std::int64_t> const& geom)
    {
        backend_.start_tile_feature(feature_);
        backend_.current_feature_->set_type(vector_tile::Tile_GeomType_POINT);
        bool painted = backend_.add_path(geom);
        backend_.stop_tile_feature();
        return painted;
    }

    bool operator() (mapnik::geometry::multi_point<std::int64_t> const& geom)
    {
        bool painted = false;
        if (!geom.empty())
        {
            backend_.start_tile_feature(feature_);
            backend_.current_feature_->set_type(vector_tile::Tile_GeomType_POINT);
            painted = backend_.add_path(geom);
            backend_.stop_tile_feature();
        }
        return painted;
    }

    bool operator() (mapnik::geometry::line_string<std::int64_t> & geom)
    {
        bool painted = false;
        boost::geometry::unique(geom);
        if (geom.size() < 2)
        {
            // This is false because it means the original data was invalid
            return false;
        }
        std::deque<mapnik::geometry::line_string<int64_t>> result;
        mapnik::geometry::linear_ring<std::int64_t> clip_box;
        clip_box.emplace_back(tile_clipping_extent_.minx(),tile_clipping_extent_.miny());
        clip_box.emplace_back(tile_clipping_extent_.maxx(),tile_clipping_extent_.miny());
        clip_box.emplace_back(tile_clipping_extent_.maxx(),tile_clipping_extent_.maxy());
        clip_box.emplace_back(tile_clipping_extent_.minx(),tile_clipping_extent_.maxy());
        clip_box.emplace_back(tile_clipping_extent_.minx(),tile_clipping_extent_.miny());
        boost::geometry::intersection(clip_box,geom,result);
        if (!result.empty())
        {
            // Found some data in tile so painted is now true
            painted = true;
            // Add the data to the tile
            backend_.start_tile_feature(feature_);
            backend_.current_feature_->set_type(vector_tile::Tile_GeomType_LINESTRING);
            for (auto const& ls : result)
            {
                backend_.add_path(ls);
            }
            backend_.stop_tile_feature();
        }
        return painted;
    }

    bool operator() (mapnik::geometry::multi_line_string<std::int64_t> & geom)
    {
        bool painted = false;
        mapnik::geometry::linear_ring<std::int64_t> clip_box;
        clip_box.emplace_back(tile_clipping_extent_.minx(),tile_clipping_extent_.miny());
        clip_box.emplace_back(tile_clipping_extent_.maxx(),tile_clipping_extent_.miny());
        clip_box.emplace_back(tile_clipping_extent_.maxx(),tile_clipping_extent_.maxy());
        clip_box.emplace_back(tile_clipping_extent_.minx(),tile_clipping_extent_.maxy());
        clip_box.emplace_back(tile_clipping_extent_.minx(),tile_clipping_extent_.miny());
        bool first = true;
        boost::geometry::unique(geom);
        for (auto const& line : geom)
        {
            if (line.size() < 2)
            {
               continue;
            }
            // If any line reaches here painted is now true because
            std::deque<mapnik::geometry::line_string<int64_t>> result;
            boost::geometry::intersection(clip_box,line,result);
            if (!result.empty())
            {
                if (first)
                {
                    painted = true;
                    first = false;
                    backend_.start_tile_feature(feature_);
                    backend_.current_feature_->set_type(vector_tile::Tile_GeomType_LINESTRING);
                }
                for (auto const& ls : result)
                {
                    backend_.add_path(ls);
                }
            }
        }
        if (!first)
        {
            backend_.stop_tile_feature();
        }
        return painted;
    }

    bool operator() (mapnik::geometry::polygon<std::int64_t> & geom)
    {
        bool painted = false;
        if (geom.exterior_ring.size() < 3)
        {
            // Invalid geometry so will be false
            return false;
        }
        // Because of geometry cleaning and other methods
        // we automatically call this tile painted even if there is no intersections.
        painted = true;
        double clean_distance = 1.415;
        mapnik::geometry::linear_ring<std::int64_t> clip_box;
        clip_box.emplace_back(tile_clipping_extent_.minx(),tile_clipping_extent_.miny());
        clip_box.emplace_back(tile_clipping_extent_.maxx(),tile_clipping_extent_.miny());
        clip_box.emplace_back(tile_clipping_extent_.maxx(),tile_clipping_extent_.maxy());
        clip_box.emplace_back(tile_clipping_extent_.minx(),tile_clipping_extent_.maxy());
        clip_box.emplace_back(tile_clipping_extent_.minx(),tile_clipping_extent_.miny());
        ClipperLib::Clipper clipper;
        ClipperLib::CleanPolygon(geom.exterior_ring, clean_distance);
        double outer_area = ClipperLib::Area(geom.exterior_ring);
        if (std::abs(outer_area) < area_threshold_)
        {
            return painted;
        }
        // The view transform inverts the y axis so this should be positive still despite now
        // being clockwise for the exterior ring. If it is not lets invert it.
        if (outer_area > 0)
        {   
            std::reverse(geom.exterior_ring.begin(), geom.exterior_ring.end());
        }
        ClipperLib::Clipper poly_clipper;
        
        if (strictly_simple_) 
        {
            poly_clipper.StrictlySimple(true);
        }
        if (!poly_clipper.AddPath(geom.exterior_ring, ClipperLib::ptSubject, true))
        {
            return painted;
        }
        for (auto & ring : geom.interior_rings)
        {
            if (ring.size() < 3) 
            {
                continue;
            }
            ClipperLib::CleanPolygon(ring, clean_distance);
            double inner_area = ClipperLib::Area(ring);
            if (std::abs(inner_area) < area_threshold_)
            {
                continue;
            }
            // This should be a negative area, the y axis is down, so the ring will be "CCW" rather
            // then "CW" after the view transform, but if it is not lets reverse it
            if (inner_area < 0)
            {
                std::reverse(ring.begin(), ring.end());
            }
            if (!poly_clipper.AddPath(ring, ClipperLib::ptSubject, true))
            {
                continue;
            }
        }
        if (!poly_clipper.AddPath( clip_box, ClipperLib::ptClip, true ))
        {
            return painted;
        }
        ClipperLib::PolyTree polygons;
        poly_clipper.Execute(ClipperLib::ctIntersection, polygons, fill_type_);
        poly_clipper.Clear();
        
        mapnik::geometry::multi_polygon<std::int64_t> mp;
        
        for (auto * polynode : polygons.Childs)
        {
            process_polynode_branch(polynode, mp, area_threshold_); 
        }

        if (mp.empty())
        {
            return painted;
        }

        backend_.start_tile_feature(feature_);
        backend_.current_feature_->set_type(vector_tile::Tile_GeomType_POLYGON);
        
        for (auto const& poly : mp)
        {
            backend_.add_path(poly);
        }
        backend_.stop_tile_feature();
        return painted;
    }

    bool operator() (mapnik::geometry::multi_polygon<std::int64_t> & geom)
    {
        bool painted = false;
        //mapnik::box2d<std::int64_t> bbox = mapnik::geometry::envelope(geom);
        if (geom.empty())
        {
            return painted;
        }
        // From this point on due to polygon cleaning etc, we just assume that the tile has some sort
        // of intersection and is painted.
        painted = true;   
        double clean_distance = 1.415;
        mapnik::geometry::linear_ring<std::int64_t> clip_box;
        clip_box.emplace_back(tile_clipping_extent_.minx(),tile_clipping_extent_.miny());
        clip_box.emplace_back(tile_clipping_extent_.maxx(),tile_clipping_extent_.miny());
        clip_box.emplace_back(tile_clipping_extent_.maxx(),tile_clipping_extent_.maxy());
        clip_box.emplace_back(tile_clipping_extent_.minx(),tile_clipping_extent_.maxy());
        clip_box.emplace_back(tile_clipping_extent_.minx(),tile_clipping_extent_.miny());
        mapnik::geometry::multi_polygon<std::int64_t> mp;
        
        ClipperLib::Clipper clipper;
        for (auto & poly : geom)
        {
            if (!clipper.AddPath( clip_box, ClipperLib::ptClip, true ))
            {
                return painted;
            }
            if (poly.exterior_ring.size() < 3)
            {
                continue;
            }
            ClipperLib::CleanPolygon(poly.exterior_ring, clean_distance);
            double outer_area = ClipperLib::Area(poly.exterior_ring);
            if (std::abs(outer_area) < area_threshold_)
            {
                continue;
            }
            // The view transform inverts the y axis so this should be positive still despite now
            // being clockwise for the exterior ring. If it is not lets invert it.
            if (outer_area > 0)
            {   
                std::reverse(poly.exterior_ring.begin(), poly.exterior_ring.end());
            }
            if (!clipper.AddPath(poly.exterior_ring, ClipperLib::ptSubject, true))
            {
                continue;
            }
            for (auto & ring : poly.interior_rings)
            {
                if (ring.size() < 3)
                {
                    continue;
                }
                ClipperLib::CleanPolygon(ring, clean_distance);
                double inner_area = ClipperLib::Area(ring);
                if (std::abs(inner_area) < area_threshold_)
                {
                    continue;
                }
                // This should be a negative area, the y axis is down, so the ring will be "CCW" rather
                // then "CW" after the view transform, but if it is not lets reverse it
                if (inner_area < 0)
                {
                    std::reverse(ring.begin(), ring.end());
                }
                if (!clipper.AddPath(ring, ClipperLib::ptSubject, true))
                {
                    continue;
                }
            }
            if (multi_polygon_union_)
            {
                continue;
            }
            ClipperLib::PolyTree polygons;
            if (strictly_simple_) 
            {
                clipper.StrictlySimple(true);
            }
            clipper.Execute(ClipperLib::ctIntersection, polygons, fill_type_);
            clipper.Clear();
            
            for (auto * polynode : polygons.Childs)
            {
                process_polynode_branch(polynode, mp, area_threshold_); 
            }
        }

        if (multi_polygon_union_)
        {
            ClipperLib::PolyTree polygons;
            if (strictly_simple_) 
            {
                clipper.StrictlySimple(true);
            }
            clipper.ReverseSolution(true);
            clipper.Execute(ClipperLib::ctIntersection, polygons, fill_type_);
            clipper.Clear();
            
            for (auto * polynode : polygons.Childs)
            {
                process_polynode_branch(polynode, mp, area_threshold_); 
            }
        }

        if (mp.empty())
        {
            return painted;
        }

        backend_.start_tile_feature(feature_);
        backend_.current_feature_->set_type(vector_tile::Tile_GeomType_POLYGON);
        
        for (auto const& poly : mp)
        {
            backend_.add_path(poly);
        }
        backend_.stop_tile_feature();
        return painted;
    }

    backend_type & backend_;
    mapnik::feature_impl const& feature_;
    mapnik::box2d<int> const& tile_clipping_extent_;
    double area_threshold_;
    bool strictly_simple_;
    bool multi_polygon_union_;
    ClipperLib::PolyFillType fill_type_;
};

template <typename T>
struct simplify_visitor 
{
    typedef T backend_type;
    simplify_visitor(double simplify_distance,
                     encoder_visitor<backend_type> & encoder) :
      encoder_(encoder),
      simplify_distance_(simplify_distance) {}

    bool operator() (mapnik::geometry::point<std::int64_t> const& geom)
    {
        return encoder_(geom);
    }

    bool operator() (mapnik::geometry::multi_point<std::int64_t> const& geom)
    {
        return encoder_(geom);
    }

    bool operator() (mapnik::geometry::line_string<std::int64_t> const& geom)
    {
        mapnik::geometry::line_string<std::int64_t> simplified;
        boost::geometry::simplify(geom,simplified,simplify_distance_);
        return encoder_(simplified);
    }

    bool operator() (mapnik::geometry::multi_line_string<std::int64_t> const& geom)
    {
        mapnik::geometry::multi_line_string<std::int64_t> simplified;
        boost::geometry::simplify(geom,simplified,simplify_distance_);
        return encoder_(simplified);
    }

    bool operator() (mapnik::geometry::polygon<std::int64_t> const& geom)
    {
        mapnik::geometry::polygon<std::int64_t> simplified;
        boost::geometry::simplify(geom,simplified,simplify_distance_);
        return encoder_(simplified);
    }

    bool operator() (mapnik::geometry::multi_polygon<std::int64_t> const& geom)
    {
        mapnik::geometry::multi_polygon<std::int64_t> simplified;
        boost::geometry::simplify(geom,simplified,simplify_distance_);
        return encoder_(simplified);
    }

    bool operator() (mapnik::geometry::geometry_collection<std::int64_t> const& geom)
    {
        bool painted = false;
        for (auto const& g : geom)
        {
            if (mapnik::util::apply_visitor((*this), g))
            {
                painted = true;
            }
        }
        return painted;
    }

    bool operator() (mapnik::geometry::geometry_empty const&)
    {
        return false;
    }

    encoder_visitor<backend_type> & encoder_;
    unsigned simplify_distance_;
};


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
    encoder_visitor<T> encoder(backend_, 
                               feature, 
                               tile_clipping_extent, 
                               area_threshold_, 
                               strictly_simple_, 
                               multi_polygon_union_, 
                               fill_type_);
    if (simplify_distance_ > 0)
    {
        simplify_visitor<T> simplifier(simplify_distance_,encoder);
        return mapnik::util::apply_visitor(simplifier,new_geom);
    }
    else
    {
        return mapnik::util::apply_visitor(encoder,new_geom);
    }
}

}} // end ns
