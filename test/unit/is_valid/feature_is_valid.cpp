#include "catch.hpp"

// mvt
#include "vector_tile_is_valid.hpp"

// protozero
#include <protozero/pbf_writer.hpp>

#include "vector_tile.pb.h"



TEST_CASE( "invalid empty feature" )
{
    std::string buffer;
    vector_tile::Tile_Feature value;
    std::set<mapnik::vector_tile_impl::validity_error> errs;

    value.SerializeToString(&buffer);
    protozero::pbf_reader pbf_feature(buffer);

    feature_is_valid(pbf_feature, errs);

    CHECK(errs.empty() == false);
    CHECK(errs.count(mapnik::vector_tile_impl::validity_error::FEATURE_IS_EMPTY) == 1);
}
