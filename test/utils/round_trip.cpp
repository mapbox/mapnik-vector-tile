// test utils
#include "round_trip.hpp"

// mapnik-vector-tile
#include "vector_tile_processor.hpp"
#include "vector_tile_strategy.hpp"
#include "vector_tile_geometry_decoder.hpp"

// mapnik
#include <mapnik/feature_factory.hpp>
#include <mapnik/memory_datasource.hpp>

// std
#include <exception>

// libprotobuf
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include "vector_tile.pb.h"
#pragma GCC diagnostic pop

namespace test_utils
{

mapnik::geometry::geometry<double> round_trip(mapnik::geometry::geometry<double> const& geom,
                                              double simplify_distance,
                                              mapnik::vector_tile_impl::polygon_fill_type fill_type,
                                              bool mpu)
{
    unsigned tile_size = 256 * 1000;
    // Create map note its not 3857 -- round trip as 4326
    mapnik::Map map(tile_size,tile_size,"+init=epsg:4326");
    // create layer
    mapnik::layer lyr("layer",map.srs());
    // create feature with geometry
    mapnik::context_ptr ctx = std::make_shared<mapnik::context_type>();
    mapnik::feature_ptr feature(mapnik::feature_factory::create(ctx,1));
    mapnik::geometry::geometry<double> g(geom);
    feature->set_geometry(std::move(g));
    mapnik::parameters params;
    params["type"] = "memory";
    std::shared_ptr<mapnik::memory_datasource> ds = std::make_shared<mapnik::memory_datasource>(params);
    ds->push(feature);
    lyr.set_datasource(ds);
    map.add_layer(lyr);
    
    // Build request
    mapnik::box2d<double> bbox(-180,-90,180,90);

    // Build processor and create tile
    mapnik::vector_tile_impl::processor ren(map);
    ren.set_simplify_distance(simplify_distance);
    ren.set_fill_type(fill_type);
    ren.set_multi_polygon_union(mpu);
    mapnik::vector_tile_impl::tile out_tile = ren.create_tile(bbox, tile_size);
    
    if (out_tile.get_layers().size() != 1)
    {
        std::stringstream s;
        s << "expected 1 layer in `round_trip` found " << out_tile.get_layers().size();
        throw std::runtime_error(s.str());
    }

    protozero::pbf_reader layer_reader;
    out_tile.layer_reader(0, layer_reader);
    if (!layer_reader.next(mapnik::vector_tile_impl::Layer_Encoding::FEATURES))
    {
        throw std::runtime_error("Expected at least one feature in layer");
    }
    protozero::pbf_reader feature_reader = layer_reader.get_message();
    int32_t geometry_type = mapnik::vector_tile_impl::Geometry_Type::UNKNOWN; 
    std::pair<protozero::pbf_reader::const_uint32_iterator, protozero::pbf_reader::const_uint32_iterator> geom_itr;
    while (feature_reader.next())
    {
        if (feature_reader.tag() == mapnik::vector_tile_impl::Feature_Encoding::GEOMETRY)
        {
            geom_itr = feature_reader.get_packed_uint32();
        }
        else if (feature_reader.tag() == mapnik::vector_tile_impl::Feature_Encoding::TYPE)
        {
            geometry_type = feature_reader.get_enum();
        }
        else
        {
            feature_reader.skip();
        }
    }
    mapnik::vector_tile_impl::GeometryPBF geoms(geom_itr);
    return mapnik::vector_tile_impl::decode_geometry<double>(geoms, geometry_type, 2, 0.0, 0.0, 1000.0, -1000.0);
}

} // end ns
