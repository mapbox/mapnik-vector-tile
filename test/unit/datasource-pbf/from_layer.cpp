#include "catch.hpp"

// mapnik vector tile
#include "vector_tile_datasource_pbf.hpp"

// libprotobuf
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include "vector_tile.pb.h"
#pragma GCC diagnostic pop

TEST_CASE( "cannot create datasource from layer pbf without name" )
{
    unsigned tile_size = 256;

    vector_tile::Tile_Layer layer;

    std::string buffer;
    layer.SerializePartialToString(&buffer);
    protozero::pbf_reader pbf_layer(buffer);

    CHECK_THROWS_AS(mapnik::vector_tile_impl::tile_datasource_pbf ds(pbf_layer,0,0,0,tile_size), std::runtime_error);
}
