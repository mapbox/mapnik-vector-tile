#ifndef __MAPNIK_VECTOR_TILE_DATASOURCE_PBF_H__
#define __MAPNIK_VECTOR_TILE_DATASOURCE_PBF_H__

#include <mapnik/datasource.hpp>
#include <mapnik/box2d.hpp>
#include <protozero/pbf_reader.hpp>

namespace mapnik { namespace vector_tile_impl {

    // TODO: consider using mapnik::value here instead
    using pbf_attr_value_type = mapnik::util::variant<std::string, float, double, int64_t, uint64_t, bool>;
    using layer_pbf_attr_type = std::vector<pbf_attr_value_type>;

    class tile_datasource_pbf : public datasource
    {
    public:
        tile_datasource_pbf(protozero::pbf_reader const& layer,
                        unsigned x,
                        unsigned y,
                        unsigned z,
                        unsigned tile_size);
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
    private:
        mutable mapnik::layer_descriptor desc_;
        mutable bool attributes_added_;
        protozero::pbf_reader layer_;
        unsigned x_;
        unsigned y_;
        unsigned z_;
        unsigned tile_size_;
        mutable bool extent_initialized_;
        mutable mapnik::box2d<double> extent_;
        double tile_x_;
        double tile_y_;
        double scale_;
        uint32_t layer_extent_;

        std::string name_;
        std::vector<protozero::pbf_reader> features_;
        std::vector<std::string> layer_keys_;
        layer_pbf_attr_type layer_values_;
    };

}} // end ns

#if !defined(MAPNIK_VECTOR_TILE_LIBRARY)
#include "vector_tile_datasource_pbf.ipp"
#endif

#endif // __MAPNIK_VECTOR_TILE_DATASOURCE_PBF_H__
