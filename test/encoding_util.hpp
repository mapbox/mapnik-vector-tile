#include <mapnik/vertex.hpp>
#include <mapnik/geometry.hpp>
#include "vector_tile_geometry_encoder.hpp"

void decode_geometry(vector_tile::Tile_Feature const& f,
                     mapnik::geometry_type & geom,
                     double & x,
                     double & y,
                     double scale)
{
    int cmd = -1;
    const int cmd_bits = 3;
    unsigned length = 0;
    for (int k = 0; k < f.geometry_size();)
    {
        if (!length) {
            unsigned cmd_length = f.geometry(k++);
            cmd = cmd_length & ((1 << cmd_bits) - 1);
            length = cmd_length >> cmd_bits;
        }
        if (length > 0) {
            length--;
            if (cmd == mapnik::SEG_MOVETO || cmd == mapnik::SEG_LINETO)
            {
                int32_t dx = f.geometry(k++);
                int32_t dy = f.geometry(k++);
                dx = ((dx >> 1) ^ (-(dx & 1)));
                dy = ((dy >> 1) ^ (-(dy & 1)));
                x += (static_cast<double>(dx) / scale);
                y += (static_cast<double>(dy) / scale);
                geom.push_vertex(x, y, static_cast<mapnik::CommandType>(cmd));
            }
            else if (cmd == (mapnik::SEG_CLOSE & ((1 << cmd_bits) - 1)))
            {
                geom.push_vertex(0, 0, mapnik::SEG_CLOSE);
            }
            else
            {
                std::stringstream msg;
                msg << "Unknown command type (decode_geometry): "
                    << cmd;
                throw std::runtime_error(msg.str());
            }
        }
    }
}

template <typename T>
std::string show_path(T & path)
{
    unsigned cmd = -1;
    double x = 0;
    double y = 0;
    std::ostringstream s;
    path.rewind(0);
    while ((cmd = path.vertex(&x, &y)) != mapnik::SEG_END)
    {
        switch (cmd)
        {
            case mapnik::SEG_MOVETO: s << "move_to("; break;
            case mapnik::SEG_LINETO: s << "line_to("; break;
            case mapnik::SEG_CLOSE: s << "close_path("; break;
            default: std::clog << "unhandled cmd " << cmd << "\n"; break;
        }
        s << x << "," << y << ")\n";
    }
    return s.str();
}

std::string compare(mapnik::geometry_type const& g,
                    unsigned tolerance=0,
                    unsigned path_multiplier=1)
{
    using namespace mapnik::vector_tile_impl;
    // encode
    vector_tile::Tile_Feature feature;
    int32_t x = 0;
    int32_t y = 0;
    mapnik::vertex_adapter va(g);
    encode_geometry(va,(vector_tile::Tile_GeomType)g.type(),feature,x,y,tolerance,path_multiplier);
    // decode
    mapnik::geometry_type g2(mapnik::geometry_type::types::Polygon);
    double x0 = 0;
    double y0 = 0;
    decode_geometry(feature,g2,x0,y0,path_multiplier);
    mapnik::vertex_adapter va2(g2);
    return show_path(va2);
}
