#ifndef __MAPNIK_VECTOR_TILE_RASTER_CLIPPER_H__
#define __MAPNIK_VECTOR_TILE_RASTER_CLIPPER_H__

// mapnik
#include <mapnik/box2d.hpp>
#include <mapnik/feature.hpp>
#include <mapnik/image_scaling.hpp>
#include <mapnik/proj_transform.hpp>
#include <mapnik/raster.hpp>

// std
#include <stdexcept>

namespace mapnik
{

namespace vector_tile_impl
{

template <typename T>
struct raster_clipper
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
    raster_clipper(mapnik::raster const& source,
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
                             int start_y);
    
    void operator() (mapnik::image_rgba8 & source_data);
    
    void operator() (mapnik::image_gray8 & source_data);

    void operator() (mapnik::image_gray8s & source_data);

    void operator() (mapnik::image_gray16 & source_data);

    void operator() (mapnik::image_gray16s & source_data);

    void operator() (mapnik::image_gray32 & source_data);

    void operator() (mapnik::image_gray32s & source_data);

    void operator() (mapnik::image_gray32f & source_data);
    
    void operator() (mapnik::image_gray64 & source_data);
   
    void operator() (mapnik::image_gray64s & source_data);

    void operator() (mapnik::image_gray64f & source_data);

    void operator() (image_null &) const
    {
        throw std::runtime_error("Null data passed to visitor");
    }

};

} // end ns vector_tile_impl

} // end ns mapnik

#if !defined(MAPNIK_VECTOR_TILE_LIBRARY)
#include "vector_tile_raster_clipper.ipp"
#endif

#endif // __MAPNIK_VECTOR_TILE_RASTER_CLIPPER_H__
