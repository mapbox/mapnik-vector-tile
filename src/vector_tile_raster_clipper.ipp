// mapnik
#include <mapnik/box2d.hpp>
#include <mapnik/image.hpp>
#include <mapnik/image_any.hpp>
#include <mapnik/image_scaling.hpp>
#include <mapnik/image_util.hpp>
#include <mapnik/proj_transform.hpp>
#include <mapnik/raster.hpp>
#include <mapnik/warp.hpp>

// agg
#include "agg_rendering_buffer.h"
#include "agg_pixfmt_rgba.h"
#include "agg_pixfmt_gray.h"
#include "agg_renderer_base.h"


namespace mapnik
{

namespace vector_tile_impl
{

raster_clipper::raster_clipper(mapnik::raster const& source,
                               box2d<double> const& target_ext,
                               box2d<double> const& ext,
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
      target_ext_(target_ext),
      ext_(ext),
      prj_trans_(prj_trans),
      image_format_(image_format),
      scaling_method_(scaling_method),
      width_(width),
      height_(height),
      raster_width_(raster_width),
      raster_height_(raster_height),
      start_x_(start_x),
      start_y_(start_y)
{
}

std::string raster_clipper::operator() (mapnik::image_rgba8 & source_data)
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
    return mapnik::save_to_string(im_tile, image_format_);    
}

std::string raster_clipper::operator() (mapnik::image_gray8 & source_data)
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
    return mapnik::save_to_string(im_tile, image_format_);    
}

std::string raster_clipper::operator() (mapnik::image_gray8s & source_data)
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
    return mapnik::save_to_string(im_tile, image_format_);    
}

std::string raster_clipper::operator() (mapnik::image_gray16 & source_data)
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
    return mapnik::save_to_string(im_tile, image_format_);    
}

std::string raster_clipper::operator() (mapnik::image_gray16s & source_data)
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
    return mapnik::save_to_string(im_tile, image_format_);    
}

std::string raster_clipper::operator() (mapnik::image_gray32 & source_data)
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
    return mapnik::save_to_string(im_tile, image_format_);    
}

std::string raster_clipper::operator() (mapnik::image_gray32s & source_data)
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
    return mapnik::save_to_string(im_tile, image_format_);    
}

std::string raster_clipper::operator() (mapnik::image_gray32f & source_data)
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
    return mapnik::save_to_string(im_tile, image_format_);    
}

std::string raster_clipper::operator() (mapnik::image_gray64 & source_data)
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
    return mapnik::save_to_string(im_tile, image_format_);    
}

std::string raster_clipper::operator() (mapnik::image_gray64s & source_data)
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
    return mapnik::save_to_string(im_tile, image_format_);    
}

std::string raster_clipper::operator() (mapnik::image_gray64f & source_data)
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
    return mapnik::save_to_string(im_tile, image_format_);    
}

} // end ns vector_tile_impl

} // end ns mapnik
