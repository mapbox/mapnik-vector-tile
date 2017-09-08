// mapnik-vector-tile
#include "vector_tile_geometry_decoder.hpp"

// mapnik
#include <mapnik/geometry/box2d.hpp>
#include <mapnik/feature_factory.hpp>
#include <mapnik/geometry.hpp>
#include <mapnik/image.hpp>
#include <mapnik/image_reader.hpp>
#include <mapnik/raster.hpp>
#include <mapnik/unicode.hpp>
#include <mapnik/view_transform.hpp>
#if defined(DEBUG)
#include <mapnik/debug.hpp>
#endif

// protozero
#include <protozero/pbf_reader.hpp>

namespace mapnik
{

namespace vector_tile_impl
{

template <typename Filter>
tile_featureset_pbf<Filter>::tile_featureset_pbf(Filter const& filter,
                                                 mapnik::box2d<double> const& tile_extent,
                                                 mapnik::box2d<double> const& unbuffered_query,
                                                 std::set<std::string> const& attribute_names,
                                                 std::vector<protozero::pbf_reader> const& features,
                                                 double tile_x,
                                                 double tile_y,
                                                 double scale,
                                                 std::vector<std::string> const& layer_keys,
                                                 layer_pbf_attr_type const& layer_values,
                                                 unsigned version)
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
          version_(version),
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

struct value_visitor
{
    mapnik::transcoder & tr_;
    mapnik::feature_ptr & feature_;
    std::string const& name_;

    value_visitor(mapnik::transcoder & tr,
                  mapnik::feature_ptr & feature,
                  std::string const& name)
        : tr_(tr),
          feature_(feature),
          name_(name) {}

    void operator() (std::string const& val)
    {
        feature_->put(name_, tr_.transcode(val.data(), val.length()));
    }

    void operator() (bool const& val)
    {
        feature_->put(name_, static_cast<mapnik::value_bool>(val));
    }

    void operator() (int64_t const& val)
    {
        feature_->put(name_, static_cast<mapnik::value_integer>(val));
    }

    void operator() (uint64_t const& val)
    {
        feature_->put(name_, static_cast<mapnik::value_integer>(val));
    }

    void operator() (double const& val)
    {
        feature_->put(name_, static_cast<mapnik::value_double>(val));
    }

    void operator() (float const& val)
    {
        feature_->put(name_, static_cast<mapnik::value_double>(val));
    }
};

template <typename Filter>
feature_ptr tile_featureset_pbf<Filter>::next()
{
    while ( itr_ < features_.size() )
    {
        protozero::pbf_reader f = features_.at(itr_);
        // TODO: auto-increment feature id counter here
        mapnik::feature_ptr feature = mapnik::feature_factory::create(ctx_,itr_);

        ++itr_;
        int32_t geometry_type = 0; // vector_tile::Tile_GeomType_UNKNOWN
        bool has_geometry = false;
        bool has_geometry_type = false;
        GeometryPBF::pbf_itr geom_itr;
        bool has_raster = false;
        std::unique_ptr<mapnik::image_reader> reader;
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
                        for (auto _i = tag_iterator.begin(); _i != tag_iterator.end();)
                        {
                            std::size_t key_name = *(_i++);
                            if (_i == tag_iterator.end())
                            {
                                throw std::runtime_error("Vector Tile has a feature with an odd number of tags, therefore the tile is invalid.");
                            }
                            std::size_t key_value = *(_i++);
                            if (key_name < num_keys_
                                && key_value < num_values_)
                            {
                                std::string const& name = layer_keys_.at(key_name);
                                if (feature->has_key(name))
                                {
                                    pbf_attr_value_type val = layer_values_.at(key_value);
                                    value_visitor vv(tr_, feature, name);
                                    mapnik::util::apply_visitor(vv, val);
                                }
                            }
                            else if (version_ == 2)
                            {
                                throw std::runtime_error("Vector Tile has a feature with repeated attributes with an invalid key or value as it does not appear in the layer. This is invalid according to the Mapbox Vector Tile Specification Version 2");
                            }
                        }
                    }
                    break;
                case 3:
                    has_geometry_type = true;
                    geometry_type = f.get_enum();
                    switch (geometry_type)
                    {
                      case 1: //vector_tile::Tile_GeomType_POINT
                      case 2: // vector_tile::Tile_GeomType_LINESTRING
                      case 3: // vector_tile::Tile_GeomType_POLYGON
                        break;
                      default:  // vector_tile::Tile_GeomType_UNKNOWN or any other value
                        throw std::runtime_error("Vector tile has an unknown geometry type " + std::to_string(geometry_type) + " in feature");
                    }
                    break;
                case 5:
                    {
                        if (has_geometry)
                        {
                            throw std::runtime_error("Vector Tile has a feature with a geometry and a raster, it must have only one of them");
                        }
                        if (has_raster)
                        {
                            throw std::runtime_error("Vector Tile has a feature with multiple raster fields, it must have only one of them");
                        }
                        has_raster = true;
                        auto image_buffer = f.get_view();
                        reader = std::unique_ptr<mapnik::image_reader>(mapnik::get_image_reader(image_buffer.data(), image_buffer.size()));
                    }
                    break;
                case 4:
                    if (has_raster)
                    {
                        throw std::runtime_error("Vector Tile has a feature with a geometry and a raster, it must have only one of them");
                    }
                    if (has_geometry)
                    {
                        throw std::runtime_error("Vector Tile has a feature with multiple geometry fields, it must have only one of them");
                    }
                    has_geometry = true;
                    geom_itr = f.get_packed_uint32();
                    break;
                default:
                    throw std::runtime_error("Vector Tile contains unknown field type " + std::to_string(f.tag()) +" in feature");

            }
        }
        if (has_raster)
        {
            if (reader.get())
            {
                int image_width = reader->width();
                int image_height = reader->height();
                if (image_width > 0 && image_height > 0)
                {
                    mapnik::view_transform t(image_width, image_height, tile_extent_, 0, 0);
                    box2d<double> intersect = tile_extent_.intersect(unbuffered_query_);
                    box2d<double> ext = t.forward(intersect);
                    if (ext.width() > std::numeric_limits<double>::epsilon() * 5.0
                        && ext.height() > std::numeric_limits<double>::epsilon() * 5.0)
                    {
                        // select minimum raster containing whole ext
                        int x_off = static_cast<int>(std::floor(ext.minx()));
                        int y_off = static_cast<int>(std::floor(ext.miny()));
                        int end_x = static_cast<int>(std::ceil(ext.maxx()));
                        int end_y = static_cast<int>(std::ceil(ext.maxy()));

                        // clip to available data
                        if (x_off >= image_width) x_off = image_width - 1;
                        if (y_off >= image_height) y_off = image_height - 1;
                        if (x_off < 0) x_off = 0;
                        if (y_off < 0) y_off = 0;
                        if (end_x > image_width) end_x = image_width;
                        if (end_y > image_height) end_y = image_height;
                        int width = end_x - x_off;
                        int height = end_y - y_off;
                        if (width < 1) width = 1;
                        if (height < 1) height = 1;
                        box2d<double> feature_raster_extent(x_off,
                                                            y_off,
                                                            x_off + width,
                                                            y_off + height);
                        feature_raster_extent = t.backward(feature_raster_extent);
                        double filter_factor = 1.0;
                        mapnik::image_any data = reader->read(x_off, y_off, width, height);
                        mapnik::raster_ptr raster = std::make_shared<mapnik::raster>(feature_raster_extent,
                                                      intersect,
                                                      std::move(data),
                                                      filter_factor
                                                      );
                        feature->set_raster(raster);
                    }
                }
            }
            return feature;
        }
        else if (has_geometry)
        {
            if (!has_geometry_type)
            {
                if (version_ == 1)
                {
                    continue;
                }
                else
                {
                    throw std::runtime_error("Vector Tile has a feature that does not define the required geometry type.");
                }
            }
            if (version_ != 1)
            {
                mapnik::vector_tile_impl::GeometryPBF geoms(geom_itr);
                mapnik::geometry::geometry<double> geom = decode_geometry<double>(geoms, geometry_type, version_, tile_x_, tile_y_, scale_, -1.0 * scale_, filter_.box_);
                if (geom.is<mapnik::geometry::geometry_empty>())
                {
                    continue;
                }
                #if defined(DEBUG)
                mapnik::box2d<double> envelope = mapnik::geometry::envelope(geom);
                if (!filter_.pass(envelope))
                {
                    MAPNIK_LOG_ERROR(tile_featureset_pbf) << "tile_featureset_pbf: filter:pass should not get here";
                    continue;
                }
                #endif
                feature->set_geometry(std::move(geom));
                return feature;
            }
            else
            {
                try
                {
                    mapnik::vector_tile_impl::GeometryPBF geoms(geom_itr);
                    mapnik::geometry::geometry<double> geom = decode_geometry<double>(geoms, geometry_type, version_, tile_x_, tile_y_, scale_, -1.0 * scale_, filter_.box_);
                    if (geom.is<mapnik::geometry::geometry_empty>())
                    {
                        continue;
                    }
                    #if defined(DEBUG)
                    mapnik::box2d<double> envelope = mapnik::geometry::envelope(geom);
                    if (!filter_.pass(envelope))
                    {
                        MAPNIK_LOG_ERROR(tile_featureset_pbf) << "tile_featureset_pbf: filter:pass should not get here";
                        continue;
                    }
                    #endif
                    feature->set_geometry(std::move(geom));
                }
                catch (std::exception const&)
                {
                    // For v1 any invalid geometry errors lets just skip the feature
                    continue;
                }
                return feature;
            }
        }
        else if (version_ != 1)
        {
            throw std::runtime_error("Vector Tile does not have a geometry or raster");
        }
    }
    return feature_ptr();
}

} // end ns vector_tile_impl

} // end ns mapnik
