#ifndef __MAPNIK_VECTOR_TILE_FEATURESET_H__
#define __MAPNIK_VECTOR_TILE_FEATURESET_H__

// libprotobuf
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include "vector_tile.pb.h"
#pragma GCC diagnostic pop

// mapnik
#include <mapnik/box2d.hpp>
#include <mapnik/feature.hpp>
#include <mapnik/featureset.hpp>
#include <mapnik/unicode.hpp>
#include <mapnik/view_transform.hpp>

// std
#include <set>

namespace mapnik 
{ 

namespace vector_tile_impl 
{

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
                    double scale,
                    unsigned version);
    
    virtual ~tile_featureset() {}
    
    feature_ptr next();

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
    unsigned version_;
    mapnik::transcoder tr_;
    mapnik::context_ptr ctx_;
};

} // end ns vector_tile_impl

} // end ns mapnik

#if !defined(MAPNIK_VECTOR_TILE_LIBRARY)
#include "vector_tile_featureset.ipp"
#endif

#endif // __MAPNIK_VECTOR_TILE_FEATURESET_H__
