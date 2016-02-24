#ifndef __MAPNIK_VECTOR_TILE_LOAD_TILE_H__
#define __MAPNIK_VECTOR_TILE_LOAD_TILE_H__

// mapnik-vector-tile
#include "vector_tile_config.hpp"
#include "vector_tile_compression.hpp"
#include "vector_tile_tile.hpp"
#include "vector_tile_datasource_pbf.hpp"
#include "vector_tile_is_valid.hpp"

// mapnik
#include <mapnik/map.hpp>
#include <mapnik/layer.hpp>

//protozero
#include <protozero/pbf_reader.hpp>
#include <protozero/pbf_writer.hpp>

// std
#include <set>
#include <string>

namespace mapnik
{

namespace vector_tile_impl
{

void merge_from_buffer(merc_tile & t, const char * data, std::size_t size)
{
    using ds_ptr = std::shared_ptr<mapnik::vector_tile_impl::tile_datasource_pbf>;
    protozero::pbf_reader tile_msg(data, size);
    std::vector<ds_ptr> ds_vec;
    while (tile_msg.next())
    {
        switch (tile_msg.tag())
        {
            case Tile_Encoding::LAYERS:
                {
                    auto layer_data = tile_msg.get_data();
                    protozero::pbf_reader layer_valid_msg(layer_data);
                    std::set<validity_error> errors;             
                    std::string layer_name;
                    std::uint32_t version = 1;
                    layer_is_valid(layer_valid_msg, errors, layer_name, version);
                    if (!errors.empty())
                    {   
                        std::string layer_errors;
                        validity_error_to_string(errors, layer_errors);
                        throw std::runtime_error(layer_errors);
                    }
                    if (t.has_layer(layer_name))
                    {
                        continue;
                    }
                    if (version == 1)
                    {
                        // v1 tiles will be convereted to v2
                        protozero::pbf_reader layer_msg(layer_data);
                        ds_vec.push_back(std::make_shared<mapnik::vector_tile_impl::tile_datasource_pbf>(
                                    layer_msg,
                                    t.x(),
                                    t.y(),
                                    t.z(),
                                    true));
                    }
                    else
                    {
                        t.append_layer_buffer(layer_data.first, layer_data.second, layer_name);
                    }
                }
                break;
            default:
                throw std::runtime_error("Vector Tile Buffer contains invalid tag");
                break;
        }
    }
    if (ds_vec.empty())
    {
        return;
    }
    // Convert v1 tiles to v2
    std::int32_t prev_buffer_size = t.buffer_size();
    t.buffer_size(4096); // very large buffer so we don't miss any buffered points
    mapnik::Map map(t.tile_size(), t.tile_size(), "+init=epsg:3857");
    for (auto ds : ds_vec)
    {    
        ds->set_envelope(t.get_buffered_extent());
        mapnik::layer lyr(ds->get_name(), "+init=epsg:3857");
        lyr.set_datasource(ds);
        map.add_layer(lyr);
    }
    mapnik::vector_tile_impl::processor ren(map);
    ren.set_multi_polygon_union(true);
    ren.set_fill_type(mapnik::vector_tile_impl::even_odd_fill);
    ren.set_process_all_rings(true);
    //ren.set_simplify_distance(4.0);
    ren.update_tile(t);
    t.buffer_size(prev_buffer_size);
}

void merge_from_compressed_buffer(merc_tile & t, const char * data, std::size_t size)
{
    if (mapnik::vector_tile_impl::is_gzip_compressed(data,size) ||
        mapnik::vector_tile_impl::is_zlib_compressed(data,size))
    {
        std::string decompressed;
        mapnik::vector_tile_impl::zlib_decompress(data, size, decompressed);
        return merge_from_buffer(t, decompressed.data(), decompressed.size());
    }
    else
    {
        return merge_from_buffer(t, data, size); 
    }
}

void add_image_buffer_as_tile_layer(merc_tile & t, std::string const& layer_name, const char * data, std::size_t size)
{
    std::string layer_buffer;
    protozero::pbf_writer layer_writer(layer_buffer);
    layer_writer.add_uint32(Layer_Encoding::VERSION, 2);
    layer_writer.add_string(Layer_Encoding::NAME, layer_name);
    layer_writer.add_uint32(Layer_Encoding::EXTENT, 4096);
    {
        protozero::pbf_writer feature_writer(layer_writer, Layer_Encoding::FEATURES);
        feature_writer.add_bytes(Feature_Encoding::RASTER, data, size);
    }
    t.append_layer_buffer(layer_buffer.data(), layer_buffer.size(), layer_name);
}

} // end ns vector_tile_impl

} // end ns mapnik

#endif // __MAPNIK_VECTOR_TILE_LOAD_TILE_H__
