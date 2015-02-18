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

#include <boost/foreach.hpp>
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
class processor : private mapnik::util::noncopyable
{
public:
    typedef T backend_type;
    enum poly_clipper_type : std::uint8_t {
        ANGUS_CLIPPER,
        AGG_CLIPPER
    };
private:
    backend_type & backend_;
    mapnik::Map const& m_;
    mapnik::request const& m_req_;
    double scale_factor_;
    mapnik::view_transform t_;
    unsigned tolerance_;
    std::string image_format_;
    scaling_method_e scaling_method_;
    bool painted_;
    poly_clipper_type poly_clipper_type_;
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
        );

    void set_poly_clipper(poly_clipper_type clipper) const;

    inline poly_clipper_type get_poly_clipper() const
    {
        return poly_clipper_type_;
    }

    void apply(double scale_denom=0.0);

    bool painted() const;

    void apply_to_layer(mapnik::layer const& lay,
                        mapnik::projection const& proj0,
                        double scale,
                        double scale_denom,
                        unsigned width,
                        unsigned height,
                        box2d<double> const& extent,
                        int buffer_size);

    unsigned handle_geometry(mapnik::vertex_adapter & geom,
                             mapnik::proj_transform const& prj_trans,
                             mapnik::box2d<double> const& buffered_query_ext);
};

}} // end ns

#if !defined(MAPNIK_VECTOR_TILE_LIBRARY)
#include "vector_tile_processor.ipp"
#endif

#endif // __MAPNIK_VECTOR_TILE_PROCESSOR_H__
