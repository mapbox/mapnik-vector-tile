#include <mapnik/vertex.hpp>
#include <mapnik/geometry.hpp>
#include <mapnik/geometry_adapters.hpp>
#include <mapnik/vertex_processor.hpp>
#include "vector_tile_geometry_decoder.hpp"
#include "vector_tile_geometry_encoder.hpp"

namespace {

using namespace mapnik::geometry;

struct print;

}

struct show_path;

struct encoder_visitor;

vector_tile::Tile_Feature geometry_to_feature(mapnik::geometry::geometry<std::int64_t> const& g);

template <typename T>
std::string decode_to_path_string(mapnik::geometry::geometry<T> const& g);

std::string compare(mapnik::geometry::geometry<std::int64_t> const& g);
