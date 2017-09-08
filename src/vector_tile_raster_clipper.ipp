// mapnik
#include <mapnik/geometry/box2d.hpp>
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

MAPNIK_VECTOR_INLINE std::string raster_clipper::operator() (mapnik::image_rgba8 & source_data)
{
    mapnik::image_rgba8 data(raster_width_, raster_height_, true, true);
    mapnik::raster target(target_ext_, std::move(data), source_.get_filter_factor());
    if (!prj_trans_.equal())
    {
        mapnik::premultiply_alpha(source_data);
        double offset_x = ext_.minx() - start_x_;
        double offset_y = ext_.miny() - start_y_;
        reproject_and_scale_raster(target, source_, prj_trans_,
                                   offset_x, offset_y,
                                   width_,
                                   scaling_method_);
    }
    else if ((raster_width_ == source_data.width()) && (raster_height_ == source_data.height()))
    {
        mapnik::demultiply_alpha(source_data);
        return mapnik::save_to_string(source_data, image_format_);
    }
    else
    {
        mapnik::premultiply_alpha(source_data);
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

    using pixfmt_type = agg::pixfmt_rgba32_pre;
    using renderer_type = agg::renderer_base<pixfmt_type>;

    mapnik::image_rgba8 im_tile(width_, height_, true, true);
    agg::rendering_buffer dst_buffer(im_tile.bytes(), im_tile.width(), im_tile.height(), im_tile.row_size());
    agg::rendering_buffer src_buffer(target.data_.bytes(),target.data_.width(), target.data_.height(), target.data_.row_size());
    pixfmt_type src_pixf(src_buffer);
    pixfmt_type dst_pixf(dst_buffer);
    renderer_type ren(dst_pixf);
    ren.copy_from(src_pixf,0,start_x_, start_y_);
    mapnik::demultiply_alpha(im_tile);
    return mapnik::save_to_string(im_tile, image_format_);
}

MAPNIK_VECTOR_INLINE std::string raster_clipper::operator() (mapnik::image_gray8 & source_data)
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
    else if ((raster_width_ == source_data.width()) && (raster_height_ == source_data.height()))
    {
        mapnik::image_any any(source_data);
        return mapnik::save_to_string(any, image_format_);
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

MAPNIK_VECTOR_INLINE std::string raster_clipper::operator() (mapnik::image_gray8s & source_data)
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
    else if ((raster_width_ == source_data.width()) && (raster_height_ == source_data.height()))
    {
        mapnik::image_any any(source_data);
        return mapnik::save_to_string(any, image_format_);
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

MAPNIK_VECTOR_INLINE std::string raster_clipper::operator() (mapnik::image_gray16 & source_data)
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
    else if ((raster_width_ == source_data.width()) && (raster_height_ == source_data.height()))
    {
        mapnik::image_any any(source_data);
        return mapnik::save_to_string(any, image_format_);
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

MAPNIK_VECTOR_INLINE std::string raster_clipper::operator() (mapnik::image_gray16s & source_data)
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
    else if ((raster_width_ == source_data.width()) && (raster_height_ == source_data.height()))
    {
        mapnik::image_any any(source_data);
        return mapnik::save_to_string(any, image_format_);
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

MAPNIK_VECTOR_INLINE std::string raster_clipper::operator() (mapnik::image_gray32 & source_data)
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
    else if ((raster_width_ == source_data.width()) && (raster_height_ == source_data.height()))
    {
        mapnik::image_any any(source_data);
        return mapnik::save_to_string(any, image_format_);
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

MAPNIK_VECTOR_INLINE std::string raster_clipper::operator() (mapnik::image_gray32s & source_data)
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
    else if ((raster_width_ == source_data.width()) && (raster_height_ == source_data.height()))
    {
        mapnik::image_any any(source_data);
        return mapnik::save_to_string(any, image_format_);
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

MAPNIK_VECTOR_INLINE std::string raster_clipper::operator() (mapnik::image_gray32f & source_data)
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
    else if ((raster_width_ == source_data.width()) && (raster_height_ == source_data.height()))
    {
        mapnik::image_any any(source_data);
        return mapnik::save_to_string(any, image_format_);
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

MAPNIK_VECTOR_INLINE std::string raster_clipper::operator() (mapnik::image_gray64 & source_data)
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
    else if ((raster_width_ == source_data.width()) && (raster_height_ == source_data.height()))
    {
        mapnik::image_any any(source_data);
        return mapnik::save_to_string(any, image_format_);
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

MAPNIK_VECTOR_INLINE std::string raster_clipper::operator() (mapnik::image_gray64s & source_data)
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
    else if ((raster_width_ == source_data.width()) && (raster_height_ == source_data.height()))
    {
        mapnik::image_any any(source_data);
        return mapnik::save_to_string(any, image_format_);
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

MAPNIK_VECTOR_INLINE std::string raster_clipper::operator() (mapnik::image_gray64f & source_data)
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
    else if ((raster_width_ == source_data.width()) && (raster_height_ == source_data.height()))
    {
        mapnik::image_any any(source_data);
        return mapnik::save_to_string(any, image_format_);
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
