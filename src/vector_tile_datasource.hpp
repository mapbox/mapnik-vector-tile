#ifndef __MAPNIK_VECTOR_TILE_DATASOURCE_H__
#define __MAPNIK_VECTOR_TILE_DATASOURCE_H__

#include "vector_tile.pb.h"
#include "vector_tile_projection.hpp"

#include <mapnik/box2d.hpp>
#include <mapnik/coord.hpp>
#include <mapnik/feature_layer_desc.hpp>
#include <mapnik/geometry.hpp>
#include <mapnik/params.hpp>
#include <mapnik/query.hpp>
#include <mapnik/unicode.hpp>
#include <mapnik/version.hpp>
#include <mapnik/value_types.hpp>
#include <mapnik/well_known_srs.hpp>

#include <mapnik/vertex.hpp>
#include <mapnik/datasource.hpp>
#include <mapnik/feature.hpp>
#include <mapnik/feature_factory.hpp>

#include <memory>
#include <stdexcept>
#include <string>

#include <boost/optional.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <unicode/unistr.h>

namespace mapnik { namespace vector {

    class tile_featureset : public Featureset
    {
    public:
        tile_featureset(mapnik::vector::tile_layer const& layer,
                        double tile_x,
                        double tile_y,
                        double scale)
            : layer_(layer),
              tile_x_(tile_x),
              tile_y_(tile_y),
              scale_(scale),
              itr_(0),
              end_(layer_.features_size()),
              tr_("utf-8"),
              ctx_(boost::make_shared<mapnik::context_type>())
        {}

        virtual ~tile_featureset() {}

        feature_ptr next()
        {
            while (itr_ < end_)
            {
                mapnik::vector::tile_feature const& f = layer_.features(itr_);
                mapnik::feature_ptr feature(
                    mapnik::feature_factory::create(ctx_,itr_++));

                std::auto_ptr<mapnik::geometry_type> geom(
                    new mapnik::geometry_type(
                        mapnik::eGeomType(f.type())));

                int cmd = -1;
                const int cmd_bits = 3;
                unsigned length = 0;
                double x = tile_x_, y = tile_y_;

                for (int k = 0; k < f.geometry_size();)
                {
                    if (!length) {
                        unsigned cmd_length = f.geometry(k++);
                        cmd = cmd_length & ((1 << cmd_bits) - 1);
                        length = cmd_length >> cmd_bits;
                    }
                    if (length > 0) {
                        length--;
                        if (cmd == mapnik::SEG_MOVETO || cmd == mapnik::SEG_LINETO)
                        {
                            int32_t dx = f.geometry(k++);
                            int32_t dy = f.geometry(k++);
                            dx = ((dx >> 1) ^ (-(dx & 1)));
                            dy = ((dy >> 1) ^ (-(dy & 1)));
                            x += (double)dx / scale_;
                            y -= (double)dy / scale_;
                            geom->push_vertex(x, y, static_cast<mapnik::CommandType>(cmd));
                        }
                        else if (cmd == (mapnik::SEG_CLOSE & ((1 << cmd_bits) - 1)))
                        {
                            geom->push_vertex(0, 0, mapnik::SEG_CLOSE);
                        }
                        else
                        {
                            throw std::runtime_error("Unknown command type");
                        }
                    }
                }
                feature->paths().push_back(geom);

                // attributes
                for (int m = 0; m < f.tags_size(); m += 2)
                {
                    std::size_t key_name = f.tags(m);
                    std::size_t key_value = f.tags(m + 1);

                    if (key_name < static_cast<std::size_t>(layer_.keys_size())
                        && key_value < static_cast<std::size_t>(layer_.values_size()))
                    {
                        std::string const& name = layer_.keys(key_name);
                        mapnik::vector::tile_value const& value = layer_.values(key_value);
                        if (value.has_string_value())
                        {
                            std::string str = value.string_value();
                            feature->put_new(name, tr_.transcode(str.data(), str.length()));
                        }
                        else if (value.has_int_value())
                        {
                            mapnik::value_integer val = value.int_value();
                            feature->put_new(name, val);
                        }
                        else if (value.has_double_value())
                        {
                            mapnik::value_double val = value.double_value();
                            feature->put_new(name, val);
                        }
                        else if (value.has_float_value())
                        {
                            mapnik::value_double val = value.float_value();
                            feature->put_new(name, val);
                        }
                        else if (value.has_bool_value())
                        {
                            mapnik::value_bool val = value.bool_value();
                            feature->put_new(name, val);
                        }
                        else if (value.has_sint_value())
                        {
                            mapnik::value_integer val = value.sint_value();
                            feature->put_new(name, val);
                        }
                        else if (value.has_uint_value())
                        {
                            mapnik::value_integer val = value.uint_value();
                            feature->put_new(name, val);
                        }
                        else
                        {
                            // Do nothing
                            //feature->put_new(name, mapnik::value_null());
                        }
                    }
                }
                return feature;
            }
            return feature_ptr();
        }

    private:
        mapnik::vector::tile_layer const& layer_;
        double tile_x_;
        double tile_y_;
        double scale_;
        unsigned itr_;
        unsigned end_;
        mapnik::transcoder tr_;
        mapnik::context_ptr ctx_;
    };

    class tile_datasource : public datasource
    {
        friend class tile_featureset;
    public:
        tile_datasource(mapnik::vector::tile_layer const& layer,
                        unsigned x,
                        unsigned y,
                        unsigned z,
                        unsigned tile_size);
        virtual ~tile_datasource();
        datasource::datasource_t type() const;
        featureset_ptr features(query const& q) const;
        featureset_ptr features_at_point(coord2d const& pt, double tol = 0) const;
        void set_envelope(box2d<double> const& bbox);
        box2d<double> envelope() const;
        boost::optional<geometry_t> get_geometry_type() const;
        layer_descriptor get_descriptor() const;
    private:
        mapnik::layer_descriptor desc_;
        mapnik::vector::tile_layer const& layer_;
        unsigned x_;
        unsigned y_;
        unsigned z_;
        unsigned tile_size_;
        mutable bool extent_initialized_;
        mutable mapnik::box2d<double> extent_;
        double tile_x_;
        double tile_y_;
        double scale_;
    };

// tile_datasource impl
    tile_datasource::tile_datasource(mapnik::vector::tile_layer const& layer,
                                     unsigned x,
                                     unsigned y,
                                     unsigned z,
                                     unsigned tile_size)
        : datasource(parameters()),
          desc_("in-memory datasource","utf-8"),
          layer_(layer),
          x_(x),
          y_(y),
          z_(z),
          tile_size_(tile_size),
          extent_initialized_(false) {
        double resolution = mapnik::EARTH_CIRCUMFERENCE/(1 << z_);
        tile_x_ = -0.5 * mapnik::EARTH_CIRCUMFERENCE + x_ * resolution;
        tile_y_ =  0.5 * mapnik::EARTH_CIRCUMFERENCE - y_ * resolution;
        scale_ = (static_cast<double>(layer_.extent()) / tile_size_) * tile_size_/resolution;
    }

    tile_datasource::~tile_datasource() {}

    datasource::datasource_t tile_datasource::type() const
    {
        return datasource::Vector;
    }

    featureset_ptr tile_datasource::features(query const& q) const
    {
        // TODO - restrict features based on query
        return boost::make_shared<tile_featureset>
            (layer_, tile_x_, tile_y_, scale_);
    }

    featureset_ptr tile_datasource::features_at_point(coord2d const& pt, double tol) const
    {
        // TODO - add support
        return featureset_ptr();
    }

    void tile_datasource::set_envelope(box2d<double> const& bbox)
    {
        extent_initialized_ = true;
        extent_ = bbox;
    }

    box2d<double> tile_datasource::envelope() const
    {
        if (!extent_initialized_)
        {
            mapnik::vector::spherical_mercator<22> merc(tile_size_);
            merc.xyz(extent_,x_,y_,z_);
            extent_initialized_ = true;
        }
        return extent_;
    }

    boost::optional<datasource::geometry_t> tile_datasource::get_geometry_type() const
    {
        return datasource::Collection;
    }

    layer_descriptor tile_datasource::get_descriptor() const
    {
        return desc_;
    }

    }} // end ns

#endif // __MAPNIK_VECTOR_TILE_DATASOURCE_H__
