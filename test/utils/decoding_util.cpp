// test utils
#include "decoding_util.hpp"

// mapnik-vector-tile
#include "vector_tile_geometry_decoder.hpp"

mapnik::vector_tile_impl::GeometryPBF feature_to_pbf_geometry(std::string const& feature_string)
{
    protozero::pbf_reader feature_pbf(feature_string);
    feature_pbf.next(4);
    return mapnik::vector_tile_impl::GeometryPBF(feature_pbf.get_packed_uint32());
}
