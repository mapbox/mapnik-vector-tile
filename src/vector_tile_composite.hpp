#pragma once

// mapnik-vector-tile
#include "vector_tile_merc_tile.hpp"
#include "vector_tile_datasource_pbf.hpp"
#include "vector_tile_processor.hpp"
#include "vector_tile_config.hpp"

// mapnik
#include <mapnik/map.hpp>
#include <mapnik/layer.hpp>

//protozero
#include <protozero/pbf_reader.hpp>

namespace mapnik
{

namespace vector_tile_impl
{

inline void composite(merc_tile & target_vt,
               std::vector<merc_tile_ptr> const& vtiles,
               mapnik::Map & map,
               processor & ren, // processor should be on map object provided
               double scale_denominator = 0.0,
               unsigned offset_x = 0,
               unsigned offset_y = 0,
               bool reencode = false)
{
    if (target_vt.tile_size() <= 0)
    {
        throw std::runtime_error("Vector tile size must be great than zero");
    }

    for (auto const& vt : vtiles)
    {
        if (vt->tile_size() <= 0)
        {
            throw std::runtime_error("Vector tile size must be great than zero");
        }

        if (vt->is_empty())
        {
            continue;
        }

        bool reencode_tile = reencode;
        if (!reencode_tile && !target_vt.same_extent(*vt))
        {
            reencode_tile = true;
        }
        
        protozero::pbf_reader tile_message(vt->get_reader());
        // loop through the layers of the tile!
        while (tile_message.next(Tile_Encoding::LAYERS))
        {
            bool reencode_layer = reencode_tile;
            const auto data_view = tile_message.get_view();
            protozero::pbf_reader layer_message(data_view);
            if (!layer_message.next(Layer_Encoding::NAME))
            {
                continue;
            }

            std::string layer_name = layer_message.get_string();
            
            if (!reencode_layer)
            {
                target_vt.append_layer_buffer(data_view.data(), data_view.size(), layer_name);
            }
            
            if (target_vt.has_layer(layer_name))
            {
                continue;
            }

            protozero::pbf_reader layer_pbf(data_view);
            auto ds = std::make_shared<mapnik::vector_tile_impl::tile_datasource_pbf>(
                        layer_pbf,
                        vt->x(),
                        vt->y(),
                        vt->z(),
                        true);
            mapnik::layer lyr(layer_name,"+init=epsg:3857");
            ds->set_envelope(vt->get_buffered_extent());
            lyr.set_datasource(ds);
            map.add_layer(lyr);
        }
    }
    if (!map.layers().empty())
    {
        ren.update_tile(target_vt,
                        scale_denominator,
                        offset_x,
                        offset_y);
    }
}

} // end ns vector_tile_impl

} // end ns mapnik
