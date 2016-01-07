// std
#include <exception>

namespace mapnik
{

namespace vector_tile_impl
{

tile::tile()
    : tile_(new vector_tile::Tile()),
      empty_layers_(),
      painted_layers_()
{
}

tile::tile(std::string const& str)
    : tile_(new vector_tile::Tile()),
      empty_layers_(),
      painted_layers_()
{
    if(!tile_->ParseFromString(str))
    {
        throw std::runtime_error("Invalid vector tile buffer provided");
    }
    for (auto const& layer : tile_->layers())
    {
        if (layer.features_size() > 0)
        {
            std::string const& layer_name = layer.name();
            empty_layers_.emplace_back(layer_name);
            painted_layers_.emplace_back(layer_name);
        }
    }
}

MAPNIK_VECTOR_INLINE bool tile::add_layer(std::unique_ptr<vector_tile::Tile_Layer> & vt_layer,
                                          bool layer_is_painted,
                                          bool layer_is_empty)
{
    std::string const& new_name = vt_layer->name();
    for (int i = 0; i < tile_->layers_size(); ++i)
    {
        vector_tile::Tile_Layer const& layer = tile_->layers(i);
        std::string const& layer_name = layer.name();
        if (new_name == layer_name)
        {
            return false;
        }
    }
    if (layer_is_empty)
    {
        empty_layers_.emplace_back(new_name);
        if (layer_is_painted)
        {
            painted_layers_.emplace_back(new_name);
        }
    }
    else
    {
        painted_layers_.emplace_back(new_name);
        // the Tile_Layer's pointer is now going to be owned by the tile_.
        tile_->mutable_layers()->AddAllocated(vt_layer.release());
    }
    return true;
}

} // end ns vector_tile_impl

} // end ns mapnik
