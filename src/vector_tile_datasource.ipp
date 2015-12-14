// mapnik-vector-tile
#include "vector_tile_featureset.hpp"
#include "vector_tile_projection.hpp"

// libprotobuf
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include "vector_tile.pb.h"
#pragma GCC diagnostic pop

// mapnik
#include <mapnik/box2d.hpp>
#include <mapnik/coord.hpp>
#include <mapnik/datasource.hpp>
#include <mapnik/feature.hpp>
#include <mapnik/feature_layer_desc.hpp>
#include <mapnik/geom_util.hpp>
#include <mapnik/query.hpp>
#include <mapnik/version.hpp>
#include <mapnik/value_types.hpp>
#include <mapnik/well_known_srs.hpp>

// boost
#include <boost/optional.hpp>

// std
#include <memory>
#include <stdexcept>
#include <string>

namespace mapnik
{

namespace vector_tile_impl 
{
    
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
      extent_initialized_(false),
      version_(layer_.version())
{
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
        (filter, get_tile_extent(), q.get_unbuffered_bbox(), q.property_names(), layer_, tile_x_, tile_y_, scale_, version_);
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
        (filter, get_tile_extent(), get_tile_extent(), names, layer_, tile_x_, tile_y_, scale_, version_);
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

} // end ns vector_tile_impl

} // end ns mapnik
