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
#include <mapnik/util/variant.hpp>

#include <memory>
#include <stdexcept>
#include <string>

#include <protozero/pbf_reader.hpp>

#include <boost/optional.hpp>
#include <unicode/unistr.h>

#if defined(DEBUG)
#include <mapnik/debug.hpp>
#endif

namespace mapnik { namespace vector_tile_impl {

    template <typename Filter>
    class tile_featureset_pbf : public Featureset
    {
    public:
        tile_featureset_pbf(Filter const& filter,
                        mapnik::box2d<double> const& tile_extent,
                        mapnik::box2d<double> const& unbuffered_query,
                        std::set<std::string> const& attribute_names,
                        std::vector<protozero::pbf_reader> const& features,
                        double tile_x,
                        double tile_y,
                        double scale,
                        std::vector<std::string> const& layer_keys,
                        layer_pbf_attr_type const& layer_values)
            : filter_(filter),
              tile_extent_(tile_extent),
              unbuffered_query_(unbuffered_query),
              features_(features),
              layer_keys_(layer_keys),
              layer_values_(layer_values),
              num_keys_(layer_keys_.size()),
              num_values_(layer_values_.size()),
              tile_x_(tile_x),
              tile_y_(tile_y),
              scale_(scale),
              itr_(0),
              tr_("utf-8"),
              ctx_(std::make_shared<mapnik::context_type>())
        {
            std::set<std::string>::const_iterator pos = attribute_names.begin();
            std::set<std::string>::const_iterator end = attribute_names.end();
            for ( ;pos !=end; ++pos)
            {
                for (auto const& key : layer_keys_)
                {
                    if (key == *pos)
                    {
                        ctx_->push(*pos);
                        break;
                    }
                }
            }
        }

        virtual ~tile_featureset_pbf() {}

        feature_ptr next()
        {
            while ( itr_ < features_.size() )
            {
                protozero::pbf_reader f = features_.at(itr_);
                // TODO: auto-increment feature id counter here
                mapnik::feature_ptr feature = mapnik::feature_factory::create(ctx_,itr_);

                ++itr_;
                int32_t geometry_type = 0; // vector_tile::Tile_GeomType_UNKNOWN
                while (f.next())
                {
                    switch(f.tag())
                    {
                        case 1:
                            feature->set_id(f.get_uint64());
                            break;
                        case 2:
                            {
                                auto tag_iterator = f.get_packed_uint32();
                                for (auto _i = tag_iterator.first; _i != tag_iterator.second;)
                                {
                                    std::size_t key_name = *(_i++);
                                    assert(_i != tag_iterator.second);
                                    std::size_t key_value = *(_i++);
                                    if (key_name < num_keys_
                                        && key_value < num_values_)
                                    {
                                        std::string const& name = layer_keys_.at(key_name);
                                        if (feature->has_key(name))
                                        {
                                            pbf_attr_value_type val = layer_values_.at(key_value);
                                            if (val.is<std::string>())
                                            {
                                                feature->put(name, tr_.transcode(val.get<std::string>().data(), val.get<std::string>().length()));
                                            }
                                            else if (val.is<bool>())
                                            {
                                                feature->put(name, static_cast<mapnik::value_bool>(val.get<bool>()));
                                            }
                                            else if (val.is<int64_t>())
                                            {
                                                feature->put(name, static_cast<mapnik::value_integer>(val.get<int64_t>()));
                                            }
                                            else if (val.is<uint64_t>())
                                            {
                                                feature->put(name, static_cast<mapnik::value_integer>(val.get<uint64_t>()));
                                            }
                                            else if (val.is<double>())
                                            {
                                                feature->put(name, static_cast<mapnik::value_double>(val.get<double>()));
                                            }
                                            else if (val.is<float>())
                                            {
                                                feature->put(name, static_cast<mapnik::value_double>(val.get<float>()));
                                            }
                                            else
                                            {
                                                throw std::runtime_error("unknown attribute type while reading feature");
                                            }
                                        }
                                    }
                                }
                            }
                            break;
                        case 3:
                            geometry_type = f.get_enum();
                            switch (geometry_type)
                            {
                              case 1: //vector_tile::Tile_GeomType_POINT
                              case 2: // vector_tile::Tile_GeomType_LINESTRING
                              case 3: // vector_tile::Tile_GeomType_POLYGON
                                break;
                              default:  // vector_tile::Tile_GeomType_UNKNOWN or any other value
                                throw std::runtime_error("unknown geometry type " + std::to_string(geometry_type) + " in feature");
                            }
                            break;
                        case 5:
                            {
                            auto image_buffer = f.get_data();
                            std::unique_ptr<mapnik::image_reader> reader(mapnik::get_image_reader(image_buffer.first, image_buffer.second));
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
                                        feature->set_raster(raster);
                                        return feature;
                                    }
                                }
                            }
                          }
                            break;
                        case 4:
                            {
                                auto geom_itr = f.get_packed_uint32();
                                mapnik::vector_tile_impl::GeometryPBF<double> geoms(geom_itr, tile_x_,tile_y_,scale_,-1*scale_);
                                mapnik::geometry::geometry<double> geom = decode_geometry<double>(geoms, geometry_type, filter_.box_);
                                if (geom.is<mapnik::geometry::geometry_empty>())
                                {
                                    continue;
                                }
                                #if defined(DEBUG)
                                mapnik::box2d<double> envelope = mapnik::geometry::envelope(geom);
                                if (!filter_.pass(envelope))
                                {
                                    MAPNIK_LOG_ERROR(tile_datasource_pbf) << "tile_datasource_pbf: filter:pass should not get here";
                                    continue;
                                }
                                #endif
                                feature->set_geometry(std::move(geom));
                                return feature;
                            }
                            break;
                        default:
                            // NOTE:  The vector_tile.proto file technically allows for extension fields
                            //        of values 16 to max here.  Technically, we should just skip() those
                            //        fields.
                            //        However, if we're fed a corrupt file (or random data), we don't
                            //        want to just blindly follow the bytes, so we have made the decision
                            //        to abort cleanly, rather than doing GIGO.
                            throw std::runtime_error("unknown field type " + std::to_string(f.tag()) +" in feature");

                    }
                }
            }
            return feature_ptr();
        }

    private:
        Filter filter_;
        mapnik::box2d<double> tile_extent_;
        mapnik::box2d<double> unbuffered_query_;
        std::vector<protozero::pbf_reader> const& features_;
        std::vector<std::string> const& layer_keys_;
        layer_pbf_attr_type const& layer_values_;
        std::size_t num_keys_;
        std::size_t num_values_;

        double tile_x_;
        double tile_y_;
        double scale_;
        unsigned itr_;
        mapnik::transcoder tr_;
        mapnik::context_ptr ctx_;

    };

    // tile_datasource impl
    tile_datasource_pbf::tile_datasource_pbf(protozero::pbf_reader const& layer,
                                     unsigned x,
                                     unsigned y,
                                     unsigned z,
                                     unsigned tile_size)
        : datasource(parameters()),
          desc_("in-memory PBF encoded datasource","utf-8"),
          attributes_added_(false),
          layer_(layer),
          x_(x),
          y_(y),
          z_(z),
          tile_size_(tile_size),
          extent_initialized_(false),
          tile_x_(0.0),
          tile_y_(0.0),
          scale_(0.0),
          layer_extent_(0)
    {
        double resolution = mapnik::EARTH_CIRCUMFERENCE/(1 << z_);
        tile_x_ = -0.5 * mapnik::EARTH_CIRCUMFERENCE + x_ * resolution;
        tile_y_ =  0.5 * mapnik::EARTH_CIRCUMFERENCE - y_ * resolution;

        protozero::pbf_reader val_msg;

        while (layer_.next())
        {
            switch(layer_.tag())
            {
                case 1:
                    name_ = layer_.get_string();
                    break;
                case 2:
                    features_.push_back(layer_.get_message());
                    break;
                case 3:
                    layer_keys_.push_back(layer_.get_string());
                    break;
                case 4:
                    val_msg = layer_.get_message();
                    while (val_msg.next())
                    {
                        switch(val_msg.tag()) {
                            case 1:
                                layer_values_.push_back(val_msg.get_string());
                                break;
                            case 2:
                                layer_values_.push_back(val_msg.get_float());
                                break;
                            case 3:
                                layer_values_.push_back(val_msg.get_double());
                                break;
                            case 4:
                                layer_values_.push_back(val_msg.get_int64());
                                break;
                            case 5:
                                layer_values_.push_back(val_msg.get_uint64());
                                break;
                            case 6:
                                layer_values_.push_back(val_msg.get_sint64());
                                break;
                            case 7:
                                layer_values_.push_back(val_msg.get_bool());
                                break;
                            default:
                                throw std::runtime_error("unknown Value type " + std::to_string(layer_.tag()) + " in layer.values");
                        }
                    }
                    break;
                case 5:
                    layer_extent_ = layer_.get_uint32();
                    break;
                case 15:
                    layer_.skip();
                    break;
                default:
                    throw std::runtime_error("unknown field type " + std::to_string(layer_.tag()) + " in layer");
            }
        }
        scale_ = (static_cast<double>(layer_extent_) / tile_size_) * tile_size_/resolution;
    }

    tile_datasource_pbf::~tile_datasource_pbf() {}

    datasource::datasource_t tile_datasource_pbf::type() const
    {
        return datasource::Vector;
    }

    featureset_ptr tile_datasource_pbf::features(query const& q) const
    {
        mapnik::filter_in_box filter(q.get_bbox());
        return std::make_shared<tile_featureset_pbf<mapnik::filter_in_box> >
            (filter, get_tile_extent(), q.get_unbuffered_bbox(), q.property_names(), features_, tile_x_, tile_y_, scale_, layer_keys_, layer_values_);
    }

    featureset_ptr tile_datasource_pbf::features_at_point(coord2d const& pt, double tol) const
    {
        mapnik::filter_at_point filter(pt,tol);
        std::set<std::string> names;
        for (auto const& key : layer_keys_)
        {
            names.insert(key);
        }
        return std::make_shared<tile_featureset_pbf<filter_at_point> >
            (filter, get_tile_extent(), get_tile_extent(), names, features_, tile_x_, tile_y_, scale_, layer_keys_, layer_values_);
    }

    void tile_datasource_pbf::set_envelope(box2d<double> const& bbox)
    {
        extent_initialized_ = true;
        extent_ = bbox;
    }

    box2d<double> tile_datasource_pbf::get_tile_extent() const
    {
        spherical_mercator merc(tile_size_);
        double minx,miny,maxx,maxy;
        merc.xyz(x_,y_,z_,minx,miny,maxx,maxy);
        return box2d<double>(minx,miny,maxx,maxy);
    }

    box2d<double> tile_datasource_pbf::envelope() const
    {
        if (!extent_initialized_)
        {
            extent_ = get_tile_extent();
            extent_initialized_ = true;
        }
        return extent_;
    }

    boost::optional<mapnik::datasource_geometry_t> tile_datasource_pbf::get_geometry_type() const
    {
        return mapnik::datasource_geometry_t::Collection;
    }

    layer_descriptor tile_datasource_pbf::get_descriptor() const
    {
        if (!attributes_added_)
        {
            for (auto const& key : layer_keys_)
            {
                // Object type here because we don't know the precise value until features are unpacked
                desc_.add_descriptor(attribute_descriptor(key, Object));
            }
            attributes_added_ = true;
        }
        return desc_;
    }

    }} // end ns
