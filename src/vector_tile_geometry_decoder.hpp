#ifndef __MAPNIK_VECTOR_TILE_GEOMETRY_DECODER_H__
#define __MAPNIK_VECTOR_TILE_GEOMETRY_DECODER_H__

#include "vector_tile.pb.h"

#include <mapnik/util/is_clockwise.hpp>

//std
#include <algorithm>

namespace mapnik { namespace vector_tile_impl {

class Geometry {

public:
    inline explicit Geometry(vector_tile::Tile_Feature const& f,
                             double tile_x, double tile_y,
                             double scale_x, double scale_y);

    enum command : uint8_t {
        end = 0,
            move_to = 1,
            line_to = 2,
            close = 7
            };

    inline command next(double& rx, double& ry);

private:
    vector_tile::Tile_Feature const& f_;
    double scale_x_;
    double scale_y_;
    uint32_t k;
    uint32_t geoms_;
    uint8_t cmd;
    uint32_t length;
    double x, y;
    double ox, oy;
};

Geometry::Geometry(vector_tile::Tile_Feature const& f,
                   double tile_x, double tile_y,
                   double scale_x, double scale_y)
    : f_(f),
      scale_x_(scale_x),
      scale_y_(scale_y),
      k(0),
      geoms_(f_.geometry_size()),
      cmd(1),
      length(0),
      x(tile_x), y(tile_y),
      ox(0), oy(0) {}

Geometry::command Geometry::next(double& rx, double& ry) {
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
            x += (static_cast<double>(dx) / scale_x_);
            y += (static_cast<double>(dy) / scale_y_);
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

inline mapnik::geometry::geometry<double> decode_geometry(vector_tile::Tile_Feature const& f,
                                                  double tile_x, double tile_y,
                                                  double scale_x, double scale_y,
                                                  bool treat_all_rings_as_exterior=false)
{
    Geometry::command cmd;
    Geometry geoms(f,tile_x,tile_y,scale_x,scale_y);
    double x1, y1;
    mapnik::geometry::geometry<double> geom; // output geometry

    switch (f.type())
    {
    case vector_tile::Tile_GeomType_POINT:
    {
        mapnik::geometry::multi_point<double> mp;
        while ((cmd = geoms.next(x1, y1)) != Geometry::end)
        {
            mp.emplace_back(mapnik::geometry::point<double>(x1,y1));
        }
        std::size_t num_points = mp.size();
        if (num_points == 1)
        {
            // return the single point
            geom = std::move(mp[0]);
            return geom;
        }
        else if (num_points > 1)
        {
            // return multipoint
            geom = std::move(mp);
            return geom;
        }
        break;
    }
    case vector_tile::Tile_GeomType_LINESTRING:
    {
        mapnik::geometry::multi_line_string<double> multi_line;
        multi_line.emplace_back();
        bool first = true;
        while ((cmd = geoms.next(x1, y1)) != Geometry::end)
        {
            if (cmd == Geometry::move_to)
            {
                if (first)
                {
                    first = false;
                }
                else
                {
                    multi_line.emplace_back();
                }
            }
            multi_line.back().add_coord(x1,y1);
        }
        if (multi_line.empty())
        {
            return geom;
        }
        std::size_t num_lines = multi_line.size();
        if (num_lines == 1)
        {
            // return the single line
            auto itr = std::make_move_iterator(multi_line.begin());
            if (itr->size() > 1)
            {
                geom = std::move(*itr);
            }
            return geom;
        }
        else if (num_lines > 1)
        {
            // return multiline
            geom = std::move(multi_line);
            return geom;
        }
        break;
    }
    case vector_tile::Tile_GeomType_POLYGON:
    {
        std::vector<mapnik::geometry::linear_ring<double>> rings;
        rings.emplace_back();
        double x2,y2;
        bool first = true;
        while ((cmd = geoms.next(x1, y1)) != Geometry::end)
        {
            if (cmd == Geometry::move_to)
            {
                x2 = x1;
                y2 = y1;
                if (first)
                {
                    first = false;
                }
                else
                {
                    rings.emplace_back();
                }
            }
            else if (cmd == Geometry::close)
            {
                rings.back().add_coord(x2,y2);
                continue;
            }
            rings.back().add_coord(x1,y1);
        }

        auto rings_itr = std::make_move_iterator(rings.begin());
        auto rings_end = std::make_move_iterator(rings.end());
        std::size_t num_rings = rings.size();
        if (num_rings == 1)
        {
            if (rings_itr->size() < 4)
            {
                return geom;
            }
            if (mapnik::util::is_clockwise(*rings_itr))
            {
                // Its clockwise, so lets reverse it.
                std::reverse(rings_itr->begin(), rings_itr->end());
            }
            // return the single polygon without interior rings
            mapnik::geometry::polygon<double> poly;
            poly.set_exterior_ring(std::move(*rings_itr));
            geom = std::move(poly);
            return geom;
        }

        // Multiple rings represent either:
        //  1) a polygon with interior ring(s)
        //  2) a multipolygon with polygons with no interior ring(s)
        //  3) a multipolygon with polygons with interior ring(s)
        mapnik::geometry::multi_polygon<double> multi_poly;
        first = true;
        // back compatibility mode to previous Mapnik (pre new geometry)
        // which pushed all rings into single path
        if (treat_all_rings_as_exterior)
        {
            for (; rings_itr != rings_end; ++rings_itr)
            {
                bool degenerate_ring = (rings_itr->size() < 4);
                if (degenerate_ring) continue;
                multi_poly.emplace_back();
                if (mapnik::util::is_clockwise(*rings_itr))
                {
                    // Its clockwise, so lets reverse it.
                    std::reverse(rings_itr->begin(), rings_itr->end());
                }
                multi_poly.back().set_exterior_ring(std::move(*rings_itr));
            }
        }
        else
        {
            bool exterior_was_degenerate = false;
            bool first_winding_order = true;
            for (; rings_itr != rings_end; ++rings_itr)
            {
                bool degenerate_ring = (rings_itr->size() < 4);
                if (first)
                {
                    if (degenerate_ring)
                    {
                        exterior_was_degenerate = true;
                        continue;
                    }
                    first_winding_order = mapnik::util::is_clockwise(*rings_itr);
                    // first ring always exterior and sets all future winding order
                    multi_poly.emplace_back();
                    if (first_winding_order)
                    {
                        // Going into mapnik we want the outer ring to be CCW
                        std::reverse(rings_itr->begin(), rings_itr->end());
                    }
                    multi_poly.back().set_exterior_ring(std::move(*rings_itr));
                    first = false;
                }
                else if (first_winding_order == mapnik::util::is_clockwise(*rings_itr))
                {
                    if (degenerate_ring) continue;
                    // hit a new exterior ring, so start a new polygon
                    multi_poly.emplace_back(); // start new polygon
                    if (first_winding_order)
                    {
                        // Going into mapnik we want the outer ring to be CCW,
                        // since first winding order was CW, we need to reverse
                        // these rings.
                        std::reverse(rings_itr->begin(), rings_itr->end());
                    }
                    multi_poly.back().set_exterior_ring(std::move(*rings_itr));
                    exterior_was_degenerate = false;
                }
                else
                {
                    if (exterior_was_degenerate || degenerate_ring) continue;
                    if (first_winding_order)
                    {
                        // Going into mapnik we want the inner ring to be CW,
                        // since first winding order of the outer ring CW, we 
                        // need to reverse these rings as they are CCW.
                        std::reverse(rings_itr->begin(), rings_itr->end());
                    }
                    multi_poly.back().add_hole(std::move(*rings_itr));
                }
            }
        }
        auto num_poly = multi_poly.size();
        if (num_poly == 0)
        {
            return geom;
        }
        else if (num_poly == 1)
        {
            auto itr = std::make_move_iterator(multi_poly.begin());
            geom = std::move(mapnik::geometry::polygon<double>(std::move(*itr)));
            return geom;
        }
        else
        {
            geom = std::move(multi_poly);
            return geom;
        }
        break;
    }
    case vector_tile::Tile_GeomType_UNKNOWN:
    default:
    {
        throw std::runtime_error("unhandled geometry type during decoding");
        break;
    }
    }
    return geom;
}

}} // end ns


#endif // __MAPNIK_VECTOR_TILE_GEOMETRY_DECODER_H__
