#ifndef __MAPNIK_VECTOR_PROCESSOR_H__
#define __MAPNIK_VECTOR_PROCESSOR_H__

// mapnik
#include <mapnik/feature.hpp>
#include <mapnik/image_scaling.hpp>
#include <mapnik/layer.hpp>
#include <mapnik/map.hpp>
#include <mapnik/attribute.hpp>
#include <mapnik/request.hpp>
#include <mapnik/util/noncopyable.hpp>

// mapnik-vector-tile
#include "vector_tile_config.hpp"
#include "vector_tile_tile.hpp"
#include "vector_tile_merc_tile.hpp"

// std
#include <future>

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

class processor : private mapnik::util::noncopyable
{
private:
    mapnik::Map const& m_;
    std::string image_format_;
    double scale_factor_;
    double area_threshold_;
    double simplify_distance_;
    polygon_fill_type fill_type_;
    scaling_method_e scaling_method_;
    bool strictly_simple_;
    bool multi_polygon_union_;
    bool process_all_rings_;
    std::launch threading_mode_;
    mapnik::attributes vars_;

public:
    processor(mapnik::Map const& map, mapnik::attributes const& vars = mapnik::attributes())
        : m_(map),
          image_format_("webp"),
          scale_factor_(1.0),
          area_threshold_(0.1),
          simplify_distance_(0.0),
          fill_type_(positive_fill),
          scaling_method_(SCALING_BILINEAR),
          strictly_simple_(true),
          multi_polygon_union_(false),
          process_all_rings_(false),
          threading_mode_(std::launch::deferred),
          vars_(vars) {}

    MAPNIK_VECTOR_INLINE void update_tile(tile & t,
                                          double scale_denom = 0.0,
                                          int offset_x = 0,
                                          int offset_y = 0);

    merc_tile create_tile(std::uint64_t x,
                          std::uint64_t y,
                          std::uint64_t z,
                          std::uint32_t tile_size = 4096,
                          std::int32_t buffer_size = 0,
                          double scale_denom = 0.0,
                          int offset_x = 0,
                          int offset_y = 0)
    {
        merc_tile t(x, y, z, tile_size, buffer_size);
        update_tile(t, scale_denom, offset_x, offset_y);
        return t;
    }
    
    tile create_tile(mapnik::box2d<double> const & extent,
                     std::uint32_t tile_size = 4096,
                     std::int32_t buffer_size = 0,
                     double scale_denom = 0.0,
                     int offset_x = 0,
                     int offset_y = 0)
    {
        tile t(extent, tile_size, buffer_size);
        update_tile(t, scale_denom, offset_x, offset_y);
        return t;
    }

    void set_simplify_distance(double dist)
    {
        simplify_distance_ = dist;
    }
    
    double get_simplify_distance() const
    {
        return simplify_distance_;
    }

    void set_area_threshold(double value)
    {
        area_threshold_ = value;
    }
    
    double get_area_threshold() const
    {
        return area_threshold_;
    }

    void set_scale_factor(double value)
    {
        scale_factor_ = value;
    }
    
    double get_scale_factor() const
    {
        return scale_factor_;
    }

    void set_process_all_rings(bool value)
    {
        process_all_rings_ = value;
    }
    
    bool get_process_all_rings() const
    {
        return process_all_rings_;
    }

    void set_multi_polygon_union(bool value)
    {
        multi_polygon_union_ = value;
    }
    
    bool get_multi_polygon_union() const
    {
        return multi_polygon_union_;
    }
    
    void set_strictly_simple(bool value)
    {
        strictly_simple_ = value;
    }
    
    bool get_multipolygon_union() const
    {
        return strictly_simple_;
    }
    
    void set_fill_type(polygon_fill_type type)
    {
        fill_type_ = type;
    }
    
    polygon_fill_type set_fill_type() const
    {
        return fill_type_;
    }

    void set_scaling_method(scaling_method_e type)
    {
        scaling_method_ = type;
    }
    
    scaling_method_e set_scaling_method() const
    {
        return scaling_method_;
    }

    void set_image_format(std::string const& value)
    {
        image_format_ = value;
    }

    std::string const& get_image_format() const
    {
        return image_format_;
    }

    mapnik::attributes const& get_variables() const
    {
        return vars_;
    }

    void set_threading_mode(std::launch mode)
    {
        threading_mode_ = mode;
    }

    std::launch set_threading_mode() const
    {
        return threading_mode_;
    }

};

} // end ns vector_tile_impl

} // end ns mapnik

#if !defined(MAPNIK_VECTOR_TILE_LIBRARY)
#include "vector_tile_processor.ipp"
#endif

#endif // __MAPNIK_VECTOR_TILE_PROCESSOR_H__
