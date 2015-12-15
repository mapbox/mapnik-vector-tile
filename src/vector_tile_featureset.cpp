#include "vector_tile_featureset.hpp"
#include "vector_tile_featureset.ipp"
#include <mapnik/geom_util.hpp>

template class mapnik::vector_tile_impl::tile_featureset<mapnik::filter_in_box>;
template class mapnik::vector_tile_impl::tile_featureset<mapnik::filter_at_point>;
