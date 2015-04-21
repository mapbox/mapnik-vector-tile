#ifndef __MAPNIK_VECTOR_PROCESSOR_H__
#define __MAPNIK_VECTOR_PROCESSOR_H__

#include <mapnik/map.hpp>
#include <mapnik/layer.hpp>
#include <mapnik/feature.hpp>
#include <mapnik/util/noncopyable.hpp>
#include <mapnik/request.hpp>
#include <mapnik/view_transform.hpp>
#include <mapnik/image_scaling.hpp>
#include <mapnik/image_compositing.hpp>
#include <mapnik/geometry.hpp>

#include "vector_tile_config.hpp"

namespace mapnik { namespace vector_tile_impl {


/*
  This processor combines concepts from mapnik's
  feature_style_processor and agg_renderer. It
  differs in that here we only process layers in
  isolation of their styles, and because of this we need
  options for clipping and simplification, for example,
  that would normally come from a style's symbolizers
*/

enum poly_clipper_type : std::uint8_t {
    ANGUS_CLIPPER,
    AGG_CLIPPER
};

template <typename T>
class processor : private mapnik::util::noncopyable
{
public:
    typedef T backend_type;
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
    double simplify_distance_;
public:
    MAPNIK_VECTOR_INLINE processor(T & backend,
              mapnik::Map const& map,
              mapnik::request const& m_req,
              double scale_factor=1.0,
              unsigned offset_x=0,
              unsigned offset_y=0,
              unsigned tolerance=1,
              std::string const& image_format="jpeg",
              scaling_method_e scaling_method=SCALING_NEAR
        );

    inline void set_simplify_distance(double dist)
    {
        simplify_distance_ = dist;
    }

    inline double get_simplify_distance() const
    {
        return simplify_distance_;
    }

    MAPNIK_VECTOR_INLINE void set_poly_clipper(poly_clipper_type clipper);

    inline poly_clipper_type get_poly_clipper() const
    {
        return poly_clipper_type_;
    }

    MAPNIK_VECTOR_INLINE void apply(double scale_denom=0.0);

    MAPNIK_VECTOR_INLINE bool painted() const;

    MAPNIK_VECTOR_INLINE void apply_to_layer(mapnik::layer const& lay,
                        mapnik::projection const& proj0,
                        double scale,
                        double scale_denom,
                        unsigned width,
                        unsigned height,
                        box2d<double> const& extent,
                        int buffer_size);

    MAPNIK_VECTOR_INLINE unsigned handle_geometry(mapnik::feature_impl const& feature,
                                                  mapnik::geometry::geometry<double> const& geom,
                                                  mapnik::proj_transform const& prj_trans,
                                                  mapnik::box2d<double> const& buffered_query_ext);
};

}} // end ns

#if !defined(MAPNIK_VECTOR_TILE_LIBRARY)
#include "vector_tile_processor.ipp"
#endif

#endif // __MAPNIK_VECTOR_TILE_PROCESSOR_H__
