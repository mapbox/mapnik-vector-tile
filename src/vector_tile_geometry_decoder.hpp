#ifndef __MAPNIK_VECTOR_TILE_GEOMETRY_DECODER_H__
#define __MAPNIK_VECTOR_TILE_GEOMETRY_DECODER_H__

#include "vector_tile.pb.h"

namespace mapnik { namespace vector_tile_impl {

class Geometry {

public:
    inline explicit Geometry(vector_tile::Tile_Feature const& f,
                             double tile_x, double tile_y, double scale);

    enum command : uint8_t {
        end = 0,
        move_to = 1,
        line_to = 2,
        close = 7
    };

    inline command next(double &rx, double &ry);

private:
    vector_tile::Tile_Feature const& f_;
    double scale_;
    uint32_t k;
    uint32_t geoms_;
    uint8_t cmd;
    uint32_t length;
    double x, y;
    double ox, oy;
};


mapnik::geometry::geometry decode_geometry(vector_tile::Tile_Feature const& f,
                                           double tile_x, double tile_y, double scale)
{

    Geometry::command cmd;
    Geometry geoms(f,tile_x,tile_y,scale);
    double x1, y1;
    switch (f.type())
    {
        case vector_tile::Tile_GeomType_POINT:
        {
            mapnik::geometry::multi_point mp;
            while ((cmd = geoms.next(x1, y1)) != Geometry::end) {
                mp.emplace_back(mapnik::geometry::point(x1,y1));
            }
            std::size_t num_points = mp.size();
            if (num_points == 1)
            {
                return mp[0];
            }
            else if (num_points > 1)
            {
                return mp;
            }
            break;
        }
        case vector_tile::Tile_GeomType_LINESTRING:
        {
            mapnik::geometry::multi_line_string mp;
            mapnik::geometry::line_string line;
            while ((cmd = geoms.next(x1, y1)) != Geometry::end) {
                if (cmd == Geometry::move_to) {
                    mp.emplace_back(std::move(line));
                    line = mapnik::geometry::line_string();
                }
                line.add_coord(x1,y1);
            }
            std::size_t num_points = mp.size();
            if (num_points == 1)
            {
                return mp[0];
            }
            else if (num_points > 1)
            {
                return mp;
            }
            break;
        }
        case vector_tile::Tile_GeomType_POLYGON:
        {
            // TODO
            break;
        }
        case vector_tile::Tile_GeomType_UNKNOWN:
        default:
        {
            throw std::runtime_error("unhandled geometry type during decoding");
            break;
        }
    }
    return mapnik::geometry::geometry();
}


Geometry::Geometry(vector_tile::Tile_Feature const& f,
                   double tile_x, double tile_y, double scale)
    : f_(f),
      scale_(scale),
      k(0),
      geoms_(f_.geometry_size()),
      cmd(1),
      length(0),
      x(tile_x), y(tile_y),
      ox(0), oy(0) {}

Geometry::command Geometry::next(double &rx, double &ry) {
    if (k < geoms_) {
        if (length == 0) {
            uint32_t cmd_length = static_cast<uint32_t>(f_.geometry(k++));
            cmd = cmd_length & 0x7;
            length = cmd_length >> 3;
        }

        --length;

        if (cmd == move_to || cmd == line_to) {
            int32_t dx = f_.geometry(k++);
            int32_t dy = f_.geometry(k++);
            dx = ((dx >> 1) ^ (-(dx & 1)));
            dy = ((dy >> 1) ^ (-(dy & 1)));
            x += (static_cast<double>(dx) / scale_);
            y -= (static_cast<double>(dy) / scale_);
            rx = x;
            ry = y;
            if (cmd == move_to) {
                ox = x;
                oy = y;
                return move_to;
            } else {
                return line_to;
            }
        } else if (cmd == close) {
            rx = ox;
            ry = oy;
            return close;
        } else {
            fprintf(stderr, "unknown command: %d\n", cmd);
            return end;
        }
    } else {
        return end;
    }
}

}} // end ns


#endif // __MAPNIK_VECTOR_TILE_GEOMETRY_DECODER_H__
