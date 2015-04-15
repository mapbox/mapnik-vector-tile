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
#include <mapnik/util/is_clockwise.hpp>
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

//tmp
#include <mapnik/util/geometry_to_geojson.hpp>
#include <mapnik/geometry_reprojection.hpp>


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "vector_tile.pb.h"
#pragma GCC diagnostic pop

namespace mapnik { namespace vector_tile_impl {

template <typename CalculationType>
struct coord_transformer
{
    using calc_type = CalculationType;

    coord_transformer(view_transform const& tr, proj_transform const& prj_trans)
        : tr_(tr), prj_trans_(prj_trans) {}


    template <typename P1, typename P2>
    inline bool apply(P1 const& p1, P2 & p2) const
    {
        using coordinate_type = typename boost::geometry::coordinate_type<P2>::type;
        calc_type x = boost::geometry::get<0>(p1);
        calc_type y = boost::geometry::get<1>(p1);
        calc_type z = 0.0;
        if (!prj_trans_.backward(x, y, z)) return false;
        tr_.forward(&x,&y);
        int x_ = static_cast<int>(x);
        int y_ = static_cast<int>(y);
        boost::geometry::set<0>(p2, boost::numeric_cast<coordinate_type>(x_));
        boost::geometry::set<1>(p2, boost::numeric_cast<coordinate_type>(y_));
        return true;
    }

    view_transform const& tr_;
    proj_transform const& prj_trans_;
};

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

inline bool check_polygon(mapnik::geometry::polygon const& poly)
{
    if (mapnik::util::is_clockwise(poly.exterior_ring))
    {
        std::cerr << "exterior ring winding order is wrong" << std::endl;
        return false;
    }
    for (auto const& ring : poly.interior_rings)
    {
        if (!mapnik::util::is_clockwise(ring))
        {
            std::cerr << "interior ring winding order is wrong" << std::endl;
            return false;
        }
    }
    return true;
}

inline void correct_winding_order(mapnik::geometry::polygon & poly)
{
    bool is_clockwise = mapnik::util::is_clockwise(poly.exterior_ring);
    
    if (!is_clockwise)
    {
        std::cerr << "correcting exterior ring: " << poly.exterior_ring.size() << std::endl;
        boost::geometry::reverse(poly.exterior_ring);
    }
    
    for (auto & ring : poly.interior_rings)
    {
        if (mapnik::util::is_clockwise(ring))
        {
            std::cerr << "correcting interior ring" << ring.size() << std::endl;
            boost::geometry::reverse(ring);
        }
    }
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
                                        buffered_query_ext, scale_denom) > 0)
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
                                    buffered_query_ext, scale_denom) > 0)
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
bool is_clockwise(T const& ring)
{
    double area = 0.0;
    std::size_t num_points = ring.size();
    for (std::size_t i = 0; i < num_points; ++i)
    {
        auto const& p0 = ring[i];
        auto const& p1 = ring[(i + 1) % num_points];
        area += p0.X * p1.Y - p0.Y * p1.X;
    }
    return (area < 0.0) ? true : false;
}

inline void process_polynode_branch(ClipperLib::PolyNode* polynode, 
                             mapnik::geometry::multi_polygon & mp,
                             double mult,
                             bool recursive = false)
{
    //ClipperLib::IntPoint p;
    //bool found_o = false;
    // Gives big island
    //p.X = static_cast<ClipperLib::cInt>(-5865777.550604425*mult); 
    //p.Y = static_cast<ClipperLib::cInt>(6026295.310003296*mult);
    // Gives entire ocean
    //p.X = static_cast<ClipperLib::cInt>(-6133479.132916184*mult); 
    //p.Y = static_cast<ClipperLib::cInt>(5965031.031832729*mult);
    //p.X = static_cast<ClipperLib::cInt>(-5346617.254491508*mult); 
    //p.Y = static_cast<ClipperLib::cInt>(8588464.498122405*mult);
    
    /*if (ClipperLib::PointInPolygon(p, polynode->Contour))
    {
        found_o = true;
        std::clog << "Found a target polygon" << std::endl;
    }*/
    mapnik::geometry::polygon polygon;
    for (auto const& pt : polynode->Contour)
    {
        polygon.exterior_ring.add_coord(pt.X/mult, pt.Y/mult);
    }
    if (polynode->Contour.front() != polynode->Contour.back())
    {
        auto const& pt = polynode->Contour.front();
        polygon.exterior_ring.add_coord(pt.X/mult, pt.Y/mult);
        //if (found_o)
        //{
            //std::clog << "Wasn't closed" << std::endl;
        //}
    }
    /*
    if (found_o)
    {
        mapnik::projection source("+init=epsg:3857");
        mapnik::projection dest("+init=epsg:4326");
        mapnik::proj_transform proj_trans(source, dest);
        unsigned int n_err = 0;
        mapnik::geometry::geometry geom = mapnik::geometry::reproject_copy(
                static_cast<mapnik::geometry::line_string>(polygon.exterior_ring),
                proj_trans,
                n_err);
        std::string foo;
        mapnik::util::to_geojson(foo, geom);
        if (recursive) std::clog << "A kid" << std::endl;
        if (recursive) std::clog << foo << std::endl;
        std::clog << polynode->ChildCount() << " outer kids" << std::endl;
    }
    bool found = false;
    */
    if (polygon.exterior_ring.size() > 3) // Throw out invalid polygons
    {
        // children of exterior ring are always interior rings
        for (auto const* ring : polynode->Childs)
        {
            /*
            if (ClipperLib::PointInPolygon(p, ring->Contour))
            {
                found = true;
                std::clog << "Found a target inner polygon" << std::endl;
            }
            */
            mapnik::geometry::linear_ring hole;
            for (auto const& pt : ring->Contour)
            {
                hole.add_coord(pt.X/mult, pt.Y/mult);
            }
            if (ring->Contour.front() != ring->Contour.back())
            {
                auto const& pt = ring->Contour.front();
                hole.add_coord(pt.X/mult, pt.Y/mult);
            }
            /*
            if (found)
            {
                mapnik::projection source("+init=epsg:3857");
                mapnik::projection dest("+init=epsg:4326");
                mapnik::proj_transform proj_trans(source, dest);
                unsigned int n_err = 0;
                mapnik::geometry::geometry geom = mapnik::geometry::reproject_copy(
                        static_cast<mapnik::geometry::line_string>(hole),
                        proj_trans,
                        n_err);
                std::string foo;
                mapnik::util::to_geojson(foo, geom);
                std::clog << foo << std::endl;
            }
            */  
            if (hole.size() < 4) continue; // Throw out invalid holes
            polygon.add_hole(std::move(hole));
        }
        mp.emplace_back(std::move(polygon));
    }
    for (auto * ring : polynode->Childs)
    {
        for (auto * sub_ring : ring->Childs)
        {
            process_polynode_branch(sub_ring, mp, mult, false);
        }
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
                    unsigned tolerance,
                    double scale_denom) :
      backend_(backend),
      feature_(feature),
      prj_trans_(prj_trans),
      buffered_query_ext_(buffered_query_ext),
      t_(t),
      tolerance_(tolerance),
      scale_denom_(scale_denom) {}

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
        unsigned path_count = 0;
        mapnik::box2d<double> bbox = mapnik::geometry::envelope(geom);
        if (buffered_query_ext_.intersects(bbox) && geom.exterior_ring.size() > 3)
        {
            using va_type = mapnik::geometry::polygon_vertex_adapter;
            double mult = 1000000.0;
            ClipperLib::Clipper clipper;
            ClipperLib::Paths paths;
            ClipperLib::Path outer_path;
            outer_path.reserve(geom.exterior_ring.size());
            for (auto const& pt : geom.exterior_ring)
            {
                ClipperLib::cInt x = static_cast<ClipperLib::cInt>(pt.x*mult);
                ClipperLib::cInt y = static_cast<ClipperLib::cInt>(pt.y*mult);
                outer_path.emplace_back(x,y);
            }
            if (outer_path.back() != outer_path.front())
            {
                outer_path.emplace_back(outer_path.front().X, outer_path.front().Y);
            }
            if (is_clockwise(outer_path))
            {
                std::reverse(outer_path.begin(), outer_path.end());
            }
            paths.emplace_back(std::move(outer_path));
            for (auto const& ring : geom.interior_rings)
            {
                if (ring.size() < 4) continue;
                ClipperLib::Path path;
                path.reserve(ring.size());
                for (auto const& pt : ring)
                {
                    ClipperLib::cInt x = static_cast<ClipperLib::cInt>(pt.x*mult);
                    ClipperLib::cInt y = static_cast<ClipperLib::cInt>(pt.y*mult);
                    path.emplace_back(x,y);
                }
                if (path.back() != path.front())
                {
                    path.emplace_back(path.front().X, path.front().Y);
                }
                if (!is_clockwise(path))
                {
                    std::reverse(path.begin(), path.end());
                }
                paths.emplace_back(std::move(path));
            }
            ClipperLib::CleanPolygons(paths); // We could add tolerance here?
            if (!clipper.AddPaths(paths, ClipperLib::ptSubject, true))
            {
                std::clog << "ptSubject ext failed " << paths.size() << std::endl;
                return 0;
            }
            //clipper.Execute(ClipperLib::ctUnion, polygons, ClipperLib::pftNonZero);
            //std::cerr << "path size=" << path.size() << std::endl;
            ClipperLib::Path clip_box;
            clip_box.emplace_back(static_cast<ClipperLib::cInt>(buffered_query_ext_.minx()*mult),
                                  static_cast<ClipperLib::cInt>(buffered_query_ext_.miny()*mult));
            clip_box.emplace_back(static_cast<ClipperLib::cInt>(buffered_query_ext_.maxx()*mult),
                                  static_cast<ClipperLib::cInt>(buffered_query_ext_.miny()*mult));
            clip_box.emplace_back(static_cast<ClipperLib::cInt>(buffered_query_ext_.maxx()*mult),
                                  static_cast<ClipperLib::cInt>(buffered_query_ext_.maxy()*mult));
            clip_box.emplace_back(static_cast<ClipperLib::cInt>(buffered_query_ext_.minx()*mult),
                                  static_cast<ClipperLib::cInt>(buffered_query_ext_.maxy()*mult));
            clip_box.emplace_back(static_cast<ClipperLib::cInt>(buffered_query_ext_.minx()*mult),
                                  static_cast<ClipperLib::cInt>(buffered_query_ext_.miny()*mult));

            if (!clipper.AddPath( clip_box, ClipperLib::ptClip, true ))
            {
                std::clog << "ptClip failed!\n";
                return 0;
            }
            
            ClipperLib::PolyTree polygons;
            clipper.StrictlySimple(true);
            // If we are using pftNonZero then our input polygon must follow winding order.
            clipper.Execute(ClipperLib::ctIntersection, polygons, ClipperLib::pftNonZero);
            clipper.Clear();

            mapnik::geometry::multi_polygon mp;

            for (auto * polynode : polygons.Childs)
            {
                process_polynode_branch(polynode, mp, mult); 
            }
            //mapnik::geometry::correct(mp);
            
            if (mp.empty())
            {
                return 0;
            }

            backend_.start_tile_feature(feature_);
            backend_.current_feature_->set_type(vector_tile::Tile_GeomType_POLYGON);
            
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
            backend_.stop_tile_feature();
        }
        return path_count;
    }

    unsigned operator() (mapnik::geometry::multi_polygon const& geom)
    {
        unsigned path_count = 0;
        mapnik::box2d<double> bbox = mapnik::geometry::envelope(geom);
        if (buffered_query_ext_.intersects(bbox) || geom.empty())
        {
            using va_type = mapnik::geometry::polygon_vertex_adapter;
            double mult = 2.024e4 / scale_denom_;
            double clean_distance = 1.415;
            double area_threshold = 200;
            //std::clog << "scale_denom: " << scale_denom_ << " mult: " << mult << std::endl;
            //double mult = 100000;
            ClipperLib::Path clip_box;
            clip_box.emplace_back(static_cast<ClipperLib::cInt>(buffered_query_ext_.minx()*mult),
                                  static_cast<ClipperLib::cInt>(buffered_query_ext_.miny()*mult));
            clip_box.emplace_back(static_cast<ClipperLib::cInt>(buffered_query_ext_.minx()*mult),
                                  static_cast<ClipperLib::cInt>(buffered_query_ext_.maxy()*mult));
            clip_box.emplace_back(static_cast<ClipperLib::cInt>(buffered_query_ext_.maxx()*mult),
                                  static_cast<ClipperLib::cInt>(buffered_query_ext_.maxy()*mult));
            clip_box.emplace_back(static_cast<ClipperLib::cInt>(buffered_query_ext_.maxx()*mult),
                                  static_cast<ClipperLib::cInt>(buffered_query_ext_.miny()*mult));
            clip_box.emplace_back(static_cast<ClipperLib::cInt>(buffered_query_ext_.minx()*mult),
                                  static_cast<ClipperLib::cInt>(buffered_query_ext_.miny()*mult));
            /*if (is_clockwise(clip_box)) std::clog << "clipbox cw" << std::endl;
            else std::clog << "clipbox ccw" << std::endl;
            ClipperLib::cInt diff_y = std::abs(static_cast<ClipperLib::cInt>(buffered_query_ext_.maxy()*mult) -
                                  static_cast<ClipperLib::cInt>(buffered_query_ext_.miny()*mult));
            ClipperLib::cInt diff_x = std::abs(static_cast<ClipperLib::cInt>(buffered_query_ext_.maxx()*mult) -
                                  static_cast<ClipperLib::cInt>(buffered_query_ext_.minx()*mult));
            */
            //std::clog << "diff y: " << diff_y << " x: " << diff_x << std::endl;
            ClipperLib::Clipper clipper;
            for (auto const& poly : geom)
            {
                mapnik::box2d<double> bbox = mapnik::geometry::envelope(poly);
                if (poly.exterior_ring.size() < 4 || !buffered_query_ext_.intersects(bbox))
                {
                    continue;
                }
                ClipperLib::Paths paths;
                ClipperLib::Path outer_path;
                outer_path.reserve(poly.exterior_ring.size());
                for (auto const& pt : poly.exterior_ring)
                {
                    ClipperLib::cInt x = static_cast<ClipperLib::cInt>(pt.x*mult);
                    ClipperLib::cInt y = static_cast<ClipperLib::cInt>(pt.y*mult);
                    outer_path.emplace_back(x,y);
                }
                if (outer_path.back() != outer_path.front())
                {
                    outer_path.emplace_back(outer_path.front().X, outer_path.front().Y);
                }
                double outer_area = ClipperLib::Area(outer_path);
                if (std::abs(outer_area) < area_threshold) continue;
                if (outer_area < 0)
                {   
                    std::reverse(outer_path.begin(), outer_path.end());
                }
                paths.emplace_back(std::move(outer_path));
                for (auto const& ring : poly.interior_rings)
                {
                    if (ring.size() < 4) continue;
                    ClipperLib::Path path;
                    path.reserve(ring.size());
                    for (auto const& pt : ring)
                    {
                        ClipperLib::cInt x = static_cast<ClipperLib::cInt>(pt.x*mult);
                        ClipperLib::cInt y = static_cast<ClipperLib::cInt>(pt.y*mult);
                        path.emplace_back(x,y);
                    }
                    if (path.back() != path.front())
                    {
                        path.emplace_back(path.front().X, path.front().Y);
                    }
                    double inner_area = ClipperLib::Area(path);
                    if (std::abs(inner_area) < area_threshold) continue;
                    if (inner_area > 0)
                    {
                        std::reverse(path.begin(), path.end());
                    }
                    paths.emplace_back(std::move(path));
                }
                ClipperLib::Clipper poly_clipper;
                ClipperLib::CleanPolygons(paths, clean_distance); // We could add tolerance here?
                //ClipperLib::SimplifyPolygons(paths); //, ClipperLib::pftNonZero); // This does a union internally, it is SLOW.
                poly_clipper.StrictlySimple(true);
                ClipperLib::Paths output_paths;
                if (!poly_clipper.AddPaths(paths, ClipperLib::ptSubject, true))
                {
                    //std::clog << "ptSubject failed! " << paths.size() << std::endl;
                    continue;
                }
                if (!poly_clipper.AddPath( clip_box, ClipperLib::ptClip, true ))
                {
                    //std::clog << "ptClip failed!\n";
                }
                poly_clipper.Execute(ClipperLib::ctIntersection, output_paths); //, ClipperLib::pftNonZero);
                poly_clipper.Clear();
                //ClipperLib::CleanPolygons(output_paths, clean_distance);
                if (!clipper.AddPaths(output_paths, ClipperLib::ptSubject, true))
                {
                    //std::clog << "ptSubject failed2! " << output_paths.size() << std::endl;
                }
            }
            ClipperLib::PolyTree polygons;
            clipper.StrictlySimple(true);
            clipper.Execute(ClipperLib::ctUnion, polygons);
            clipper.Clear();
            
            mapnik::geometry::multi_polygon mp;
            
            for (auto * polynode : polygons.Childs)
            {
                process_polynode_branch(polynode, mp, mult); 
            }
            //mapnik::geometry::correct(mp);
            if (mp.empty())
            {
                return 0;
            }
            backend_.start_tile_feature(feature_);
            backend_.current_feature_->set_type(vector_tile::Tile_GeomType_POLYGON);
            
            for (auto const& poly : mp)
            {
                mapnik::box2d<double> bbox = mapnik::geometry::envelope(poly);
                if (poly.exterior_ring.size() > 3 && buffered_query_ext_.intersects(bbox))
                {
                    coord_transformer<double> transformer(t_, prj_trans_);
                    mapnik::geometry::polygon transformed_poly;
                    //std::cerr << "input poly check=" << check_polygon(poly) << std::endl;
                    boost::geometry::transform(poly, transformed_poly, transformer);
                    correct_winding_order(transformed_poly);
                    //boost::geometry::correct(transformed_poly); // ensure winding orders of rings are correct after reprojecting
                    //std::cerr << "transformed poly check=" << check_polygon(transformed_poly) << std::endl;
                    va_type va(transformed_poly);
                    path_count += backend_.add_path(va, tolerance_);
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
    double scale_denom_;
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
                                       mapnik::box2d<double> const& buffered_query_ext,
                                       double scale_denom)
{
    encoder_visitor<T> encoder(backend_,feature,prj_trans,buffered_query_ext,t_,tolerance_, scale_denom);
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
