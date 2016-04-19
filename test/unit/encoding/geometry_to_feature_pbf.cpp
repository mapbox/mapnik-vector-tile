#include "catch.hpp"

// mapnik vector tile
#include "vector_tile_geometry_feature.hpp"

// mapnik
#include <mapnik/geometry.hpp>
#include <mapnik/feature.hpp>
#include <mapnik/feature_factory.hpp>
#include <mapnik/util/variant.hpp>

// protozero
#include <protozero/pbf_writer.hpp>

// libprotobuf
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include "vector_tile.pb.h"
#pragma GCC diagnostic pop

// std
#include <limits>

//
// Unit tests for encoding of geometries to features
//

TEST_CASE("encode feature pbf of degenerate linestring")
{
    mapnik::geometry::geometry_empty empty;
    std::string empty_buffer = "";
    mapnik::vector_tile_impl::layer_builder_pbf empty_layer("foo", 4096, empty_buffer);
    mapnik::feature_ptr empty_feature(mapnik::feature_factory::create(std::make_shared<mapnik::context_type>(),1));
    mapnik::vector_tile_impl::geometry_to_feature_pbf_visitor empty_visitor(*empty_feature, empty_layer);
    empty_visitor(empty);

    mapnik::geometry::line_string<std::int64_t> line;
    line.add_coord(10,10);

    std::string layer_buffer = "";
    mapnik::vector_tile_impl::layer_builder_pbf layer("foo", 4096, layer_buffer);
    mapnik::feature_ptr f(mapnik::feature_factory::create(std::make_shared<mapnik::context_type>(),1));
    mapnik::vector_tile_impl::geometry_to_feature_pbf_visitor visitor(*f, layer);
    visitor(line);

    REQUIRE(layer_buffer == empty_buffer);
    REQUIRE(layer.empty == true);
}
