#ifndef __MAPNIK_VECTOR_TILE_GEOMETRY_DECODER_H__
#define __MAPNIK_VECTOR_TILE_GEOMETRY_DECODER_H__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include "vector_tile.pb.h"
#pragma GCC diagnostic pop

#include "pbf_reader.hpp"

#include <mapnik/util/is_clockwise.hpp>
//std
#include <algorithm>

namespace mapnik { namespace vector_tile_impl {

// NOTE: this object is for one-time use.  Once you've progressed to the end
//       by calling next(), to re-iterate, you must construct a new object
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

// NOTE: this object is for one-time use.  Once you've progressed to the end
//       by calling next(), to re-iterate, you must construct a new object
class GeometryPBF {

public:
    inline explicit GeometryPBF(std::pair< mapbox::util::pbf::const_uint32_iterator, mapbox::util::pbf::const_uint32_iterator > const& geo_iterator,
                             double tile_x, double tile_y,
                             double scale_x, double scale_y);

    enum command : uint8_t
    {
        end = 0,
        move_to = 1,
        line_to = 2,
        close = 7
    };

    inline command next(double& rx, double& ry);

private:
    std::pair< mapbox::util::pbf::const_uint32_iterator, mapbox::util::pbf::const_uint32_iterator > geo_iterator_;
    double scale_x_;
    double scale_y_;
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

Geometry::command Geometry::next(double& rx, double& ry)
{
    if (k < geoms_)
    {
        if (length == 0)
        {
            uint32_t cmd_length = static_cast<uint32_t>(f_.geometry(k++));
            cmd = cmd_length & 0x7;
            length = cmd_length >> 3;
        }

        --length;

        if (cmd == move_to || cmd == line_to)
        {
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
        }
        else if (cmd == close)
        {
            rx = ox;
            ry = oy;
            return close;
        }
        else
        {
            fprintf(stderr, "unknown command: %d\n", cmd);
            return end;
        }
    } else {
        return end;
    }
}

GeometryPBF::GeometryPBF(std::pair<mapbox::util::pbf::const_uint32_iterator, mapbox::util::pbf::const_uint32_iterator > const& geo_iterator,
                   double tile_x, double tile_y,
                   double scale_x, double scale_y)
    : geo_iterator_(geo_iterator),
      scale_x_(scale_x),
      scale_y_(scale_y),
      cmd(1),
      length(0),
      x(tile_x), y(tile_y),
      ox(0), oy(0) {}

GeometryPBF::command GeometryPBF::next(double& rx, double& ry)
{
    if (geo_iterator_.first != geo_iterator_.second)
    {
        if (length == 0)
        {
            uint32_t cmd_length = static_cast<uint32_t>(*geo_iterator_.first++);
            cmd = cmd_length & 0x7;
            length = cmd_length >> 3;
        }

        --length;

        if (cmd == move_to || cmd == line_to)
        {
            int32_t dx = *geo_iterator_.first++;
            int32_t dy = *geo_iterator_.first++;
            dx = ((dx >> 1) ^ (-(dx & 1)));
            dy = ((dy >> 1) ^ (-(dy & 1)));
            x += (static_cast<double>(dx) / scale_x_);
            y += (static_cast<double>(dy) / scale_y_);
            rx = x;
            ry = y;
            if (cmd == move_to)
            {
                ox = x;
                oy = y;
                return move_to;
            }
            else
            {
                return line_to;
            }
        }
        else if (cmd == close)
        {
            rx = ox;
            ry = oy;
            return close;
        }
        else
        {
            fprintf(stderr, "unknown command: %d\n", cmd);
            return end;
        }
    }
    else
    {
        return end;
    }
}

namespace detail {

template <typename T>
void decode_point(mapnik::geometry::geometry<double> & geom, T & paths)
{
    typename T::command cmd;
    double x1, y1;
    mapnik::geometry::multi_point<double> mp;
    while ((cmd = paths.next(x1, y1)) != T::end)
    {
        mp.emplace_back(mapnik::geometry::point<double>(x1,y1));
    }
    std::size_t num_points = mp.size();
    if (num_points == 1)
    {
        geom = std::move(mp[0]);
    }
    else if (num_points > 1)
    {
        // return multipoint
        geom = std::move(mp);
    }
}

template <typename T>
void decode_linestring(mapnik::geometry::geometry<double> & geom, T & paths)
{
    typename T::command cmd;
    double x1, y1;
    mapnik::geometry::multi_line_string<double> multi_line;
    multi_line.emplace_back();
    bool first = true;
    while ((cmd = paths.next(x1, y1)) != T::end)
    {
        if (cmd == T::move_to)
        {
            if (first) first = false;
            else multi_line.emplace_back();
        }
        multi_line.back().add_coord(x1,y1);
    }
    std::size_t num_lines = multi_line.size();
    if (num_lines == 1)
    {
        auto itr = std::make_move_iterator(multi_line.begin());
        if (itr->size() > 1)
        {
            geom = std::move(*itr);
        }
    }
    else if (num_lines > 1)
    {
        geom = std::move(multi_line);
    }
}

template <typename T>
std::vector<mapnik::geometry::linear_ring<double>> read_rings(T & paths)
{
    typename T::command cmd;
    double x1, y1;
    std::vector<mapnik::geometry::linear_ring<double>> rings;
    rings.emplace_back();
    double x2,y2;
    bool first = true;
    while ((cmd = paths.next(x1, y1)) != T::end)
    {
        if (cmd == T::move_to)
        {
            x2 = x1;
            y2 = y1;
            if (first) first = false;
            else rings.emplace_back();
        }
        else if (cmd == T::close)
        {
            auto & ring = rings.back();
            if (ring.size() > 2 && !(ring.back().x == x2 && ring.back().y == y2))
            {
                ring.add_coord(x2,y2);
            }
            continue;
        }
        rings.back().add_coord(x1,y1);
    }
    return rings;
}

template <typename T>
void decode_polygons(mapnik::geometry::geometry<double> & geom, T && rings)
{
    auto rings_itr = std::make_move_iterator(rings.begin());
    auto rings_end = std::make_move_iterator(rings.end());
    std::size_t num_rings = rings.size();
    if (num_rings == 1)
    {
        if (rings_itr->size() < 4) return;
        if (mapnik::util::is_clockwise(*rings_itr))
        {
            // Its clockwise, so lets reverse it.
            std::reverse(rings_itr->begin(), rings_itr->end());
        }
        // return the single polygon without interior rings
        mapnik::geometry::polygon<double> poly;
        poly.set_exterior_ring(std::move(*rings_itr));
        geom = std::move(poly);
    }
    else
    {
        mapnik::geometry::multi_polygon<double> multi_poly;
        bool first = true;
        bool is_clockwise = true;
        for (; rings_itr != rings_end; ++rings_itr)
        {
            if (rings_itr->size() < 4) continue; // skip degenerate rings
            if (first)
            {
                is_clockwise = mapnik::util::is_clockwise(*rings_itr);
                // first ring always exterior and sets all future winding order
                multi_poly.emplace_back();
                if (is_clockwise)
                {
                    // Going into mapnik we want the outer ring to be CCW
                    std::reverse(rings_itr->begin(), rings_itr->end());
                }
                multi_poly.back().set_exterior_ring(std::move(*rings_itr));
                first = false;
            }
            else if (is_clockwise == mapnik::util::is_clockwise(*rings_itr))
            {
                // hit a new exterior ring, so start a new polygon
                multi_poly.emplace_back(); // start new polygon
                if (is_clockwise)
                {
                    // Going into mapnik we want the outer ring to be CCW,
                    // since first winding order was CW, we need to reverse
                    // these rings.
                    std::reverse(rings_itr->begin(), rings_itr->end());
                }
                multi_poly.back().set_exterior_ring(std::move(*rings_itr));
            }
            else
            {
                if (is_clockwise)
                {
                    // Going into mapnik we want the inner ring to be CW,
                    // since first winding order of the outer ring CW, we
                    // need to reverse these rings as they are CCW.
                    std::reverse(rings_itr->begin(), rings_itr->end());
                }
                multi_poly.back().add_hole(std::move(*rings_itr));
            }
        }

        auto num_poly = multi_poly.size();
        if (num_poly == 1)
        {
            auto itr = std::make_move_iterator(multi_poly.begin());
            geom = mapnik::geometry::polygon<double>(std::move(*itr));
        }
        else
        {
            geom = std::move(multi_poly);
        }
    }
}

} // ns detail

template <typename T>
inline mapnik::geometry::geometry<double> decode_geometry(T & paths, int32_t geom_type)
{
    mapnik::geometry::geometry<double> geom; // output geometry
    switch (geom_type)
    {
    case vector_tile::Tile_GeomType_POINT:
    {
        detail::decode_point(geom, paths);
        break;
    }
    case vector_tile::Tile_GeomType_LINESTRING:
    {
        detail::decode_linestring(geom, paths);
        break;
    }
    case vector_tile::Tile_GeomType_POLYGON:
    {
        auto rings = detail::read_rings(paths);
        detail::decode_polygons(geom, rings);
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
