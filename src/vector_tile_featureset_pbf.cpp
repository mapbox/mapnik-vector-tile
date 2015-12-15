#include "vector_tile_featureset_pbf.hpp"
#include "vector_tile_featureset_pbf.ipp"
#include <mapnik/geom_util.hpp>

template class mapnik::vector_tile_impl::tile_featureset_pbf<mapnik::filter_in_box>;
template class mapnik::vector_tile_impl::tile_featureset_pbf<mapnik::filter_at_point>;
