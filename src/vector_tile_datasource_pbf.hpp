#pragma once

// mapnik
//#include <mapnik/geometry/box2d.hpp>
#include <mapnik/datasource.hpp>
#include <mapnik/util/variant.hpp>

// protozero
#include <protozero/pbf_reader.hpp>

namespace mapnik
{

namespace vector_tile_impl
{

// TODO: consider using mapnik::value here instead
using pbf_attr_value_type = mapnik::util::variant<std::string, float, double, int64_t, uint64_t, bool>;
using layer_pbf_attr_type = std::vector<pbf_attr_value_type>;

class tile_datasource_pbf : public datasource
{
public:
    tile_datasource_pbf(protozero::pbf_reader const& layer,
                    std::uint64_t x,
                    std::uint64_t y,
                    std::uint64_t z,
                    bool use_tile_extent = false);
    virtual ~tile_datasource_pbf();
    datasource::datasource_t type() const;
    featureset_ptr features(query const& q) const;
    featureset_ptr features_at_point(coord2d const& pt, double tol = 0) const;
    void set_envelope(box2d<double> const& bbox);
    box2d<double> get_tile_extent() const;
    box2d<double> envelope() const;
    boost::optional<datasource_geometry_t> get_geometry_type() const;
    layer_descriptor get_descriptor() const;
    std::string const& get_name() { return name_; }
    std::uint32_t get_layer_extent() { return tile_size_; }
private:
    mutable mapnik::layer_descriptor desc_;
    mutable bool attributes_added_;
    mutable bool valid_layer_;
    protozero::pbf_reader layer_;
    std::uint64_t x_;
    std::uint64_t y_;
    std::uint64_t z_;
    std::uint32_t tile_size_;
    mutable bool extent_initialized_;
    mutable mapnik::box2d<double> extent_;
    double tile_x_;
    double tile_y_;
    double scale_;
    uint32_t version_;
    datasource::datasource_t type_;

    std::string name_;
    std::vector<protozero::pbf_reader> features_;
    std::vector<std::string> layer_keys_;
    layer_pbf_attr_type layer_values_;
};

} // end ns vector_tile_impl

} // end ns mapnik

#if !defined(MAPNIK_VECTOR_TILE_LIBRARY)
#include "vector_tile_datasource_pbf.ipp"
#endif
