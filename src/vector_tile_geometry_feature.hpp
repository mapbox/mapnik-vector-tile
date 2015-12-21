#ifndef __MAPNIK_VECTOR_TILE_GEOMETRY_FEATURE_H__
#define __MAPNIK_VECTOR_TILE_GEOMETRY_FEATURE_H__

// mapnik-vector-tile
#include "vector_tile_config.hpp"
#include "vector_tile_layer.hpp"

// mapnik
#include <mapnik/feature.hpp>
#include <mapnik/geometry.hpp>

namespace mapnik
{

namespace vector_tile_impl
{

MAPNIK_VECTOR_INLINE void raster_to_feature(std::string const& buffer,
                                             mapnik::feature_impl const& mapnik_feature,
                                             tile_layer & layer);

template <typename T>
MAPNIK_VECTOR_INLINE void geometry_to_feature(T const& geom,
                                              mapnik::feature_impl const& mapnik_feature,
                                              tile_layer & layer,
                                              bool solid);

MAPNIK_VECTOR_INLINE void geometry_to_feature(mapnik::geometry::geometry<std::int64_t> const& geom,
                                              mapnik::feature_impl const& mapnik_feature,
                                              tile_layer & layer,
                                              bool solid);

} // end ns vector_tile_impl

} // end ns mapnik

#if !defined(MAPNIK_VECTOR_TILE_LIBRARY)
#include "vector_tile_geometry_feature.ipp"
#endif

#endif // __MAPNIK_VECTOR_GEOMETRY_FEATURE_H__
