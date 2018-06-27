// mapnik-vector-tile
#include "vector_tile_config.hpp"
#include "vector_tile_layer.hpp"

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

MAPNIK_VECTOR_INLINE bool tile::add_layer(tile_layer const& layer)
{
    std::string const& new_name = layer.name();
    if (layer.is_empty())
    {
        empty_layers_.insert(new_name);
        if (layer.is_painted())
        {
            painted_layers_.insert(new_name);
        }
    }
    else
    {
        painted_layers_.insert(new_name);
        auto p = layers_set_.insert(new_name);
        if (!p.second)
        {
            // Layer already in tile
            return false;
        }
        layers_.push_back(new_name);
        protozero::pbf_writer tile_writer(*buffer_);
        tile_writer.add_message(Tile_Encoding::LAYERS, layer.get_data());
        auto itr = empty_layers_.find(new_name);
        if (itr != empty_layers_.end())
        {
            empty_layers_.erase(itr);
        }
    }
    return true;
}

MAPNIK_VECTOR_INLINE bool tile::append_layer_buffer(const char * data, std::size_t size, std::string const& name)
{
    painted_layers_.insert(name);
    auto p = layers_set_.insert(name);
    if (!p.second)
    {
        // Layer already in tile
        return false;
    }
    layers_.push_back(name);
    protozero::pbf_writer writer(*buffer_);
    writer.add_message(3, data, size);
    auto itr = empty_layers_.find(name);
    if (itr != empty_layers_.end())
    {
        empty_layers_.erase(itr);
    }
    return true;
}

MAPNIK_VECTOR_INLINE bool tile::layer_reader(std::string const& name, protozero::pbf_reader & layer_msg) const
{
    protozero::pbf_reader item(buffer_->data(), buffer_->size());
    while (item.next(Tile_Encoding::LAYERS))
    {
        layer_msg = item.get_message();
        protozero::pbf_reader lay(layer_msg);
        while (lay.next(Layer_Encoding::NAME))
        {
            if (lay.get_string() == name)
            {
                return true;
            }
        }
    }
    return false;
}

MAPNIK_VECTOR_INLINE bool tile::layer_reader(std::size_t index, protozero::pbf_reader & layer_msg) const
{
    protozero::pbf_reader item(buffer_->data(), buffer_->size());
    std::size_t idx = 0;
    while (item.next(Tile_Encoding::LAYERS))
    {
        if (idx == index)
        {
            layer_msg = item.get_message();
            return true;
        }
        ++idx;
        item.skip();
    }
    return false;
}

} // end ns vector_tile_impl

} // end ns mapnik
