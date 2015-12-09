#include <mapnik/vertex.hpp>
#include <mapnik/geometry.hpp>
#include <mapnik/geometry_adapters.hpp>
#include <mapnik/vertex_processor.hpp>
#include "vector_tile_geometry_decoder.hpp"
#include "vector_tile_geometry_encoder.hpp"

using namespace mapnik::geometry;


struct show_path;

struct encoder_visitor;

vector_tile::Tile_Feature geometry_to_feature(mapnik::geometry::geometry<std::int64_t> const& g);

template <typename T>
std::string decode_to_path_string(mapnik::geometry::geometry<T> const& g);

std::string compare(mapnik::geometry::geometry<std::int64_t> const& g, unsigned version);

std::string compare_pbf(mapnik::geometry::geometry<std::int64_t> const& g, unsigned version);

template <typename T>
mapnik::vector_tile_impl::GeometryPBF<T> feature_to_pbf_geometry(std::string const& feature_string);


