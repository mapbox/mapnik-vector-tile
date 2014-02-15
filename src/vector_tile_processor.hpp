#ifndef __MAPNIK_VECTOR_PROCESSOR_H__
#define __MAPNIK_VECTOR_PROCESSOR_H__

#include <mapnik/map.hpp>
#include <mapnik/request.hpp>
#include <mapnik/layer.hpp>
#include <mapnik/query.hpp>
#include <mapnik/ctrans.hpp>
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
#include <mapnik/noncopyable.hpp>

// agg
#ifdef CONV_CLIPPER
#include "agg_path_storage.h"
#include "agg_conv_clipper.h"
#else
#include "agg_conv_clip_polygon.h"
#endif

#include "agg_conv_clip_polyline.h"

#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include <iostream>
#include <string>
#include <stdexcept>

namespace mapnik { namespace vector {


/*
  This processor combines concepts from mapnik's
  feature_style_processor and agg_renderer. It
  differs in that here we only process layers in
  isolation of their styles, and because of this we need
  options for clipping and simplification, for example,
  that would normally come from a style's symbolizers
*/

    template <typename T>
    class processor : private mapnik::noncopyable
    {
    public:
        typedef T backend_type;
    private:
        backend_type & backend_;
        mapnik::Map const& m_;
        mapnik::request const& m_req_;
        double scale_factor_;
        mapnik::CoordTransform t_;
        unsigned tolerance_;
        bool painted_;
    public:
        processor(T & backend,
                  mapnik::Map const& map,
                  mapnik::request const& m_req,
                  double scale_factor=1.0,
                  unsigned offset_x=0,
                  unsigned offset_y=0,
                  unsigned tolerance=1)
            : backend_(backend),
              m_(map),
              m_req_(m_req),
              scale_factor_(scale_factor),
              t_(m_req.width(),m_req.height(),m_req.extent(),offset_x,offset_y),
              tolerance_(tolerance),
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
            if (!ds)
            {
                return;
            }
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
            if (!features)
            {
                return;
            }
            mapnik::feature_ptr feature = features->next();
            if (feature) {
                backend_.start_tile_layer(lay.name());
                while (feature)
                {
                    boost::ptr_vector<mapnik::geometry_type> & paths = feature->paths();
                    if (paths.empty()) continue;
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
            case mapnik::Point:
            {
                if (geom.size() > 0)
                {
                    typedef mapnik::coord_transform<mapnik::CoordTransform,
                        mapnik::geometry_type> path_type;
                    path_type path(t_, geom, prj_trans);
                    path_count = backend_.add_path(path, tolerance_, geom.type());
                }
                break;
            }
            case mapnik::LineString:
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
                    typedef mapnik::coord_transform<mapnik::CoordTransform, line_clipper> path_type;
                    path_type path(t_, clipped, prj_trans);
                    path_count = backend_.add_path(path, tolerance_, geom.type());
                }
                break;
            }
            case mapnik::Polygon:
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
                    typedef mapnik::coord_transform<mapnik::CoordTransform, poly_clipper> path_type;
                    path_type path(t_, clipped, prj_trans);
                    path_count = backend_.add_path(path, tolerance_, geom.type());
                }
                break;
            }
            case mapnik::Unknown:
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
