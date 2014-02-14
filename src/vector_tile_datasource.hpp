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
#include <mapnik/geom_util.hpp>

#include <memory>
#include <stdexcept>
#include <string>

#include <boost/optional.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <unicode/unistr.h>

namespace mapnik { namespace vector {

    template <typename Filter>
    class tile_featureset : public Featureset
    {
    public:
        tile_featureset(Filter const& filter,
                        std::set<std::string> const& attribute_names,
                        mapnik::vector::tile_layer const& layer,
                        double tile_x,
                        double tile_y,
                        double scale)
            : filter_(filter),
              layer_(layer),
              tile_x_(tile_x),
              tile_y_(tile_y),
              scale_(scale),
              itr_(0),
              end_(layer_.features_size()),
              tr_("utf-8"),
              ctx_(boost::make_shared<mapnik::context_type>())
        {
            std::set<std::string>::const_iterator pos = attribute_names.begin();
            std::set<std::string>::const_iterator end = attribute_names.end();
            for ( ;pos !=end; ++pos)
            {
                for (int i = 0; i < layer_.keys_size(); ++i)
                {
                    if (layer_.keys(i) == *pos)
                    {
                        ctx_->push(*pos);
                        break;
                    }
                }
            }
        }

        virtual ~tile_featureset() {}

        feature_ptr next()
        {
            while (itr_ < end_)
            {
                mapnik::vector::tile_feature const& f = layer_.features(itr_);
                mapnik::value_integer feature_id = itr_++;
                // if encoded feature was given an id, respect it
                // https://github.com/mapbox/mapnik-vector-tile/issues/17
                // https://github.com/mapbox/mapnik-vector-tile/issues/18
                if (f.has_id())
                {
                    feature_id = f.id();
                }
                mapnik::feature_ptr feature(
                    mapnik::feature_factory::create(ctx_,feature_id));
                feature->paths().push_back(new mapnik::geometry_type(
                        mapnik::eGeomType(f.type())));
                mapnik::geometry_type * geom = &feature->paths().front();
                int cmd = -1;
                const int cmd_bits = 3;
                unsigned length = 0;
                double x = tile_x_, y = tile_y_;
                bool first = true;
                mapnik::box2d<double> envelope;
                double first_x=0;
                double first_y=0;
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
                            x += (static_cast<double>(dx) / scale_);
                            y -= (static_cast<double>(dy) / scale_);
                            if (cmd == mapnik::SEG_MOVETO)
                            {
                                if (!first) {
                                    feature->paths().push_back(new mapnik::geometry_type(
                                        mapnik::eGeomType(f.type())));
                                    geom = &feature->paths().back();
                                }
                                first_x = x;
                                first_y = y;
                            }
                            if (first)
                            {
                                envelope.init(x,y,x,y);
                                first = false;
                            }
                            else
                            {
                                envelope.expand_to_include(x,y);
                            }
                            geom->push_vertex(x, y, static_cast<mapnik::CommandType>(cmd));
                        }
                        else if (cmd == (mapnik::SEG_CLOSE & ((1 << cmd_bits) - 1)))
                        {
                            geom->push_vertex(first_x, first_y, mapnik::SEG_LINETO);
                            geom->push_vertex(0, 0, mapnik::SEG_CLOSE);
                        }
                        else
                        {
                            std::stringstream msg;
                            msg << "Unknown command type (tile_featureset): "
                                << cmd;
                            throw std::runtime_error(msg.str());
                        }
                    }
                }
                if (!filter_.pass(envelope))
                {
                    continue;
                }

                // attributes
                for (int m = 0; m < f.tags_size(); m += 2)
                {
                    std::size_t key_name = f.tags(m);
                    std::size_t key_value = f.tags(m + 1);

                    if (key_name < static_cast<std::size_t>(layer_.keys_size())
                        && key_value < static_cast<std::size_t>(layer_.values_size()))
                    {
                        std::string const& name = layer_.keys(key_name);
                        if (feature->has_key(name))
                        {
                            mapnik::vector::tile_value const& value = layer_.values(key_value);
                            if (value.has_string_value())
                            {
                                std::string str = value.string_value();
                                feature->put(name, tr_.transcode(str.data(), str.length()));
                            }
                            else if (value.has_int_value())
                            {
                                mapnik::value_integer val = value.int_value();
                                feature->put(name, val);
                            }
                            else if (value.has_double_value())
                            {
                                mapnik::value_double val = value.double_value();
                                feature->put(name, val);
                            }
                            else if (value.has_float_value())
                            {
                                mapnik::value_double val = value.float_value();
                                feature->put(name, val);
                            }
                            else if (value.has_bool_value())
                            {
                                mapnik::value_bool val = value.bool_value();
                                feature->put(name, val);
                            }
                            else if (value.has_sint_value())
                            {
                                mapnik::value_integer val = value.sint_value();
                                feature->put(name, val);
                            }
                            else if (value.has_uint_value())
                            {
                                mapnik::value_integer val = value.uint_value();
                                feature->put(name, val);
                            }
                            else
                            {
                                // Do nothing
                                //feature->put_new(name, mapnik::value_null());
                            }
                        }
                    }
                }
                return feature;
            }
            return feature_ptr();
        }

    private:
        Filter filter_;
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
        mutable mapnik::layer_descriptor desc_;
        mutable bool attributes_added_;
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
    inline tile_datasource::tile_datasource(mapnik::vector::tile_layer const& layer,
                                     unsigned x,
                                     unsigned y,
                                     unsigned z,
                                     unsigned tile_size)
        : datasource(parameters()),
          desc_("in-memory datasource","utf-8"),
          attributes_added_(false),
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

    inline tile_datasource::~tile_datasource() {}

    inline datasource::datasource_t tile_datasource::type() const
    {
        return datasource::Vector;
    }

    inline featureset_ptr tile_datasource::features(query const& q) const
    {
        mapnik::filter_in_box filter(q.get_bbox());
        return boost::make_shared<tile_featureset<mapnik::filter_in_box> >
            (filter, q.property_names(), layer_, tile_x_, tile_y_, scale_);
    }

    inline featureset_ptr tile_datasource::features_at_point(coord2d const& pt, double tol) const
    {
        mapnik::filter_at_point filter(pt,tol);
        std::set<std::string> names;
        for (int i = 0; i < layer_.keys_size(); ++i)
        {
            names.insert(layer_.keys(i));
        }
        return boost::make_shared<tile_featureset<filter_at_point> >
            (filter, names, layer_, tile_x_, tile_y_, scale_);
    }

    inline void tile_datasource::set_envelope(box2d<double> const& bbox)
    {
        extent_initialized_ = true;
        extent_ = bbox;
    }

    inline box2d<double> tile_datasource::envelope() const
    {
        if (!extent_initialized_)
        {
            mapnik::vector::spherical_mercator merc(tile_size_);
            double minx,miny,maxx,maxy;
            merc.xyz(x_,y_,z_,minx,miny,maxx,maxy);
            extent_.init(minx,miny,maxx,maxy);
            extent_initialized_ = true;
        }
        return extent_;
    }

    inline boost::optional<datasource::geometry_t> tile_datasource::get_geometry_type() const
    {
        return datasource::Collection;
    }

    inline layer_descriptor tile_datasource::get_descriptor() const
    {
        if (!attributes_added_)
        {
            for (int i = 0; i < layer_.keys_size(); ++i)
            {
                // Object type here because we don't know the precise value until features are unpacked
                desc_.add_descriptor(attribute_descriptor(layer_.keys(i), Object));
            }
            attributes_added_ = true;
        }
        return desc_;
    }

    }} // end ns

#endif // __MAPNIK_VECTOR_TILE_DATASOURCE_H__
