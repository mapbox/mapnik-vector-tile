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
    vector_tile::Tile tile = out_tile.get_tile();
    
    if (tile.layers_size() != 1)
    {
        std::stringstream s;
        s << "expected 1 layer in `round_trip` found " << tile.layers_size();
        throw std::runtime_error(s.str());
    }
    vector_tile::Tile_Layer const& layer = tile.layers(0);
    if (layer.features_size() != 1)
    {
        std::stringstream s;
        s << "expected 1 feature in `round_trip` found " << layer.features_size();
        throw std::runtime_error(s.str());
    }
    vector_tile::Tile_Feature const& f = layer.features(0);

    mapnik::vector_tile_impl::Geometry<double> geoms(f, 0, 0, 1000, -1000);
    return mapnik::vector_tile_impl::decode_geometry(geoms, f.type(), 2);
}

} // end ns
