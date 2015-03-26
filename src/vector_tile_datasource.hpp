#ifndef __MAPNIK_VECTOR_TILE_DATASOURCE_H__
#define __MAPNIK_VECTOR_TILE_DATASOURCE_H__

#include <mapnik/datasource.hpp>
#include <mapnik/box2d.hpp>
#include "vector_tile_config.hpp"

namespace vector_tile {
    class Tile_Layer;
}

namespace mapnik { namespace vector_tile_impl {

    class tile_datasource : public datasource
    {
    public:
        tile_datasource(vector_tile::Tile_Layer const& layer,
                        unsigned x,
                        unsigned y,
                        unsigned z,
                        unsigned tile_size);
        virtual ~tile_datasource();
        datasource::datasource_t type() const;
        featureset_ptr features(query const& q) const;
        featureset_ptr features_at_point(coord2d const& pt, double tol = 0) const;
        void set_envelope(box2d<double> const& bbox);
        box2d<double> get_tile_extent() const;
        box2d<double> envelope() const;
        boost::optional<datasource_geometry_t> get_geometry_type() const;
        layer_descriptor get_descriptor() const;
    private:
        mutable mapnik::layer_descriptor desc_;
        mutable bool attributes_added_;
        vector_tile::Tile_Layer const& layer_;
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

}} // end ns

#if !defined(MAPNIK_VECTOR_TILE_LIBRARY)
#include "vector_tile_datasource.ipp"
#endif

#endif // __MAPNIK_VECTOR_TILE_DATASOURCE_H__
