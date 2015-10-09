#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include "vector_tile.pb.h"
#pragma GCC diagnostic pop

#include "vector_tile_projection.hpp"
#include "vector_tile_geometry_decoder.hpp"

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
#include <mapnik/version.hpp>
#include <mapnik/vertex.hpp>
#include <mapnik/datasource.hpp>
#include <mapnik/feature.hpp>
#include <mapnik/feature_factory.hpp>
#include <mapnik/geom_util.hpp>
#include <mapnik/image.hpp>
#include <mapnik/image_reader.hpp>
#include <mapnik/raster.hpp>
#include <mapnik/view_transform.hpp>

#include <memory>
#include <stdexcept>
#include <string>

#include <boost/optional.hpp>
#include <unicode/unistr.h>

#if defined(DEBUG)
#include <mapnik/debug.hpp>
#endif

namespace mapnik { namespace vector_tile_impl {

    void add_attributes(mapnik::feature_ptr feature,
                        vector_tile::Tile_Feature const& f,
                        vector_tile::Tile_Layer const& layer,
                        mapnik::transcoder const& tr)
    {
        std::size_t num_keys = static_cast<std::size_t>(layer.keys_size());
        std::size_t num_values = static_cast<std::size_t>(layer.values_size());
        for (int m = 0; m < f.tags_size(); m += 2)
        {
            std::size_t key_name = f.tags(m);
            std::size_t key_value = f.tags(m + 1);
            if (key_name < num_keys
                && key_value < num_values)
            {
                std::string const& name = layer.keys(key_name);
                if (feature->has_key(name))
                {
                    vector_tile::Tile_Value const& value = layer.values(key_value);
                    if (value.has_string_value())
                    {
                        std::string const& str = value.string_value();
                        feature->put(name, tr.transcode(str.data(), str.length()));
                    }
                    else if (value.has_int_value())
                    {
                        feature->put(name, static_cast<mapnik::value_integer>(value.int_value()));
                    }
                    else if (value.has_double_value())
                    {
                        feature->put(name, static_cast<mapnik::value_double>(value.double_value()));
                    }
                    else if (value.has_float_value())
                    {
                        feature->put(name, static_cast<mapnik::value_double>(value.float_value()));
                    }
                    else if (value.has_bool_value())
                    {
                        feature->put(name, static_cast<mapnik::value_bool>(value.bool_value()));
                    }
                    else if (value.has_sint_value())
                    {
                        feature->put(name, static_cast<mapnik::value_integer>(value.sint_value()));
                    }
                    else if (value.has_uint_value())
                    {
                        feature->put(name, static_cast<mapnik::value_integer>(value.uint_value()));
                    }
                }
            }
        }
    }

    template <typename Filter>
    class tile_featureset : public Featureset
    {
    public:
        tile_featureset(Filter const& filter,
                        mapnik::box2d<double> const& tile_extent,
                        mapnik::box2d<double> const& unbuffered_query,
                        std::set<std::string> const& attribute_names,
                        vector_tile::Tile_Layer const& layer,
                        double tile_x,
                        double tile_y,
                        double scale)
            : filter_(filter),
              tile_extent_(tile_extent),
              unbuffered_query_(unbuffered_query),
              layer_(layer),
              tile_x_(tile_x),
              tile_y_(tile_y),
              scale_(scale),
              itr_(0),
              end_(layer_.features_size()),
              tr_("utf-8"),
              ctx_(std::make_shared<mapnik::context_type>())
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
                vector_tile::Tile_Feature const& f = layer_.features(itr_);
                mapnik::value_integer feature_id = itr_++;
                if (f.has_id())
                {
                    feature_id = f.id();
                }
                if (f.has_raster())
                {
                    std::string const& image_buffer = f.raster();
                    std::unique_ptr<mapnik::image_reader> reader(mapnik::get_image_reader(image_buffer.data(),image_buffer.size()));
                    if (reader.get())
                    {
                        int image_width = reader->width();
                        int image_height = reader->height();
                        if (image_width > 0 && image_height > 0)
                        {
                            mapnik::view_transform t(image_width, image_height, tile_extent_, 0, 0);
                            box2d<double> intersect = tile_extent_.intersect(unbuffered_query_);
                            box2d<double> ext = t.forward(intersect);
                            if (ext.width() > 0.5 && ext.height() > 0.5 )
                            {
                                // select minimum raster containing whole ext
                                int x_off = static_cast<int>(std::floor(ext.minx() +.5));
                                int y_off = static_cast<int>(std::floor(ext.miny() +.5));
                                int end_x = static_cast<int>(std::floor(ext.maxx() +.5));
                                int end_y = static_cast<int>(std::floor(ext.maxy() +.5));

                                // clip to available data
                                if (x_off < 0)
                                    x_off = 0;
                                if (y_off < 0)
                                    y_off = 0;
                                if (end_x > image_width)
                                    end_x = image_width;
                                if (end_y > image_height)
                                    end_y = image_height;
                                int width = end_x - x_off;
                                int height = end_y - y_off;
                                box2d<double> feature_raster_extent(x_off,
                                                                    y_off,
                                                                    x_off + width,
                                                                    y_off + height);
                                intersect = t.backward(feature_raster_extent);
                                double filter_factor = 1.0;
                                mapnik::image_any data = reader->read(x_off, y_off, width, height);
                                mapnik::raster_ptr raster = std::make_shared<mapnik::raster>(intersect,
                                                              data,
                                                              filter_factor
                                                              );
                                mapnik::feature_ptr feature = mapnik::feature_factory::create(ctx_,feature_id);
                                feature->set_raster(raster);
                                add_attributes(feature,f,layer_,tr_);
                                return feature;
                            }
                        }
                    }
                }
                if (f.geometry_size() <= 0)
                {
                    continue;
                }
                mapnik::vector_tile_impl::Geometry<double> geoms(f,tile_x_, tile_y_, scale_, -1*scale_);
                mapnik::geometry::geometry<double> geom = decode_geometry<double>(geoms, f.type(), filter_.box_);
                if (geom.is<mapnik::geometry::geometry_empty>())
                {
                    continue;
                }
                #if defined(DEBUG)
                mapnik::box2d<double> envelope = mapnik::geometry::envelope(geom);
                if (!filter_.pass(envelope))
                {
                    MAPNIK_LOG_ERROR(tile_datasource_pbf) << "tile_datasource: filter:pass should not get here";
                    continue;
                }
                #endif
                mapnik::feature_ptr feature = mapnik::feature_factory::create(ctx_,feature_id);
                feature->set_geometry(std::move(geom));
                add_attributes(feature,f,layer_,tr_);
                return feature;
            }
            return feature_ptr();
        }

    private:
        Filter filter_;
        mapnik::box2d<double> tile_extent_;
        mapnik::box2d<double> unbuffered_query_;
        vector_tile::Tile_Layer const& layer_;
        double tile_x_;
        double tile_y_;
        double scale_;
        unsigned itr_;
        unsigned end_;
        mapnik::transcoder tr_;
        mapnik::context_ptr ctx_;
    };

    // tile_datasource impl
    tile_datasource::tile_datasource(vector_tile::Tile_Layer const& layer,
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

    tile_datasource::~tile_datasource() {}

    datasource::datasource_t tile_datasource::type() const
    {
        return datasource::Vector;
    }

    featureset_ptr tile_datasource::features(query const& q) const
    {
        mapnik::filter_in_box filter(q.get_bbox());
        return std::make_shared<tile_featureset<mapnik::filter_in_box> >
            (filter, get_tile_extent(), q.get_unbuffered_bbox(), q.property_names(), layer_, tile_x_, tile_y_, scale_);
    }

    featureset_ptr tile_datasource::features_at_point(coord2d const& pt, double tol) const
    {
        mapnik::filter_at_point filter(pt,tol);
        std::set<std::string> names;
        for (int i = 0; i < layer_.keys_size(); ++i)
        {
            names.insert(layer_.keys(i));
        }
        return std::make_shared<tile_featureset<filter_at_point> >
            (filter, get_tile_extent(), get_tile_extent(), names, layer_, tile_x_, tile_y_, scale_);
    }

    void tile_datasource::set_envelope(box2d<double> const& bbox)
    {
        extent_initialized_ = true;
        extent_ = bbox;
    }

    box2d<double> tile_datasource::get_tile_extent() const
    {
        spherical_mercator merc(tile_size_);
        double minx,miny,maxx,maxy;
        merc.xyz(x_,y_,z_,minx,miny,maxx,maxy);
        return box2d<double>(minx,miny,maxx,maxy);
    }

    box2d<double> tile_datasource::envelope() const
    {
        if (!extent_initialized_)
        {
            extent_ = get_tile_extent();
            extent_initialized_ = true;
        }
        return extent_;
    }

    boost::optional<mapnik::datasource_geometry_t> tile_datasource::get_geometry_type() const
    {
        return mapnik::datasource_geometry_t::Collection;
    }

    layer_descriptor tile_datasource::get_descriptor() const
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
