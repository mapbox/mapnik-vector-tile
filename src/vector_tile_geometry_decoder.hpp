#ifndef __MAPNIK_VECTOR_TILE_GEOMETRY_DECODER_H__
#define __MAPNIK_VECTOR_TILE_GEOMETRY_DECODER_H__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include "vector_tile.pb.h"
#pragma GCC diagnostic pop

#include <protozero/pbf_reader.hpp>

#include <mapnik/util/is_clockwise.hpp>
//std
#include <algorithm>

#if defined(DEBUG)
#include <mapnik/debug.hpp>
#endif

namespace mapnik { namespace vector_tile_impl {

// NOTE: this object is for one-time use.  Once you've progressed to the end
//       by calling next(), to re-iterate, you must construct a new object
template <typename ValueType>
class Geometry {

public:
    inline explicit Geometry(vector_tile::Tile_Feature const& f,
                             ValueType tile_x, ValueType tile_y,
                             double scale_x, double scale_y);

    enum command : uint8_t {
        end = 0,
        move_to = 1,
        line_to = 2,
        close = 7
    };

    inline command next(ValueType & rx, ValueType & ry, std::uint32_t & len);

private:
    vector_tile::Tile_Feature const& f_;
    double scale_x_;
    double scale_y_;
    uint32_t k;
    uint32_t geoms_;
    uint8_t cmd;
    uint32_t length;
    ValueType x, y;
    ValueType ox, oy;
};

// NOTE: this object is for one-time use.  Once you've progressed to the end
//       by calling next(), to re-iterate, you must construct a new object
template <typename ValueType>
class GeometryPBF {

public:
    inline explicit GeometryPBF(std::pair< protozero::pbf_reader::const_uint32_iterator, protozero::pbf_reader::const_uint32_iterator > const& geo_iterator,
                             ValueType tile_x, ValueType tile_y,
                             double scale_x, double scale_y);

    enum command : uint8_t
    {
        end = 0,
        move_to = 1,
        line_to = 2,
        close = 7
    };

    inline command next(ValueType & rx, ValueType & ry, std::uint32_t & len);

private:
    std::pair< protozero::pbf_reader::const_uint32_iterator, protozero::pbf_reader::const_uint32_iterator > geo_iterator_;
    double scale_x_;
    double scale_y_;
    uint8_t cmd;
    std::uint32_t length;
    ValueType x, y;
    ValueType ox, oy;
};


template <typename ValueType>
Geometry<ValueType>::Geometry(vector_tile::Tile_Feature const& f,
                   ValueType tile_x, ValueType tile_y,
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

template <typename ValueType>
typename Geometry<ValueType>::command Geometry<ValueType>::next(ValueType & rx, ValueType & ry, std::uint32_t & len)
{
    if (k < geoms_)
    {
        if (length == 0)
        {
            uint32_t cmd_length = static_cast<uint32_t>(f_.geometry(k++));
            cmd = cmd_length & 0x7;
            len = length = cmd_length >> 3;
        }

        --length;

        if (cmd == move_to || cmd == line_to)
        {
            int32_t dx = f_.geometry(k++);
            int32_t dy = f_.geometry(k++);
            dx = ((dx >> 1) ^ (-(dx & 1)));
            dy = ((dy >> 1) ^ (-(dy & 1)));
            x += static_cast<ValueType>(static_cast<double>(dx) / scale_x_);
            y += static_cast<ValueType>(static_cast<double>(dy) / scale_y_);
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

template <typename ValueType>
GeometryPBF<ValueType>::GeometryPBF(std::pair<protozero::pbf_reader::const_uint32_iterator, protozero::pbf_reader::const_uint32_iterator > const& geo_iterator,
                   ValueType tile_x, ValueType tile_y,
                   double scale_x, double scale_y)
    : geo_iterator_(geo_iterator),
      scale_x_(scale_x),
      scale_y_(scale_y),
      cmd(1),
      length(0),
      x(tile_x), y(tile_y),
      ox(0), oy(0) {}

template <typename ValueType>
typename GeometryPBF<ValueType>::command GeometryPBF<ValueType>::next(ValueType & rx, ValueType & ry, std::uint32_t & len)
{
    if (geo_iterator_.first != geo_iterator_.second)
    {
        if (length == 0)
        {
            uint32_t cmd_length = static_cast<uint32_t>(*geo_iterator_.first++);
            cmd = cmd_length & 0x7;
            len = length = cmd_length >> 3;
        }

        --length;

        if (cmd == move_to || cmd == line_to)
        {
            int32_t dx = *geo_iterator_.first++;
            int32_t dy = *geo_iterator_.first++;
            dx = ((dx >> 1) ^ (-(dx & 1)));
            dy = ((dy >> 1) ^ (-(dy & 1)));
            x += static_cast<ValueType>(static_cast<double>(dx) / scale_x_);
            y += static_cast<ValueType>(static_cast<double>(dy) / scale_y_);
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

template <typename ValueType, typename T>
void decode_point(mapnik::geometry::geometry<ValueType> & geom, T & paths, mapnik::box2d<double> const& bbox)
{
    typename T::command cmd;
    ValueType x1, y1;
    mapnik::geometry::multi_point<ValueType> mp;
    bool first = true;
    std::uint32_t len;
    while ((cmd = paths.next(x1, y1, len)) != T::end)
    {
        // TODO: consider profiling and trying to optimize this further
        // when all points are within the bbox filter then the `mp.reserve` should be
        // perfect, but when some points are thrown out we will allocate more than needed
        // the "all points intersect" case I think is going to be more common/important
        // however worth a future look to see if the "some or few points intersect" can be optimized
        if (first)
        {
            first = false;
            mp.reserve(len);
        }
        if (!bbox.intersects(x1,y1))
        {
            continue;
        }
        mp.emplace_back(x1,y1);
    }
    std::size_t num_points = mp.size();
    #if defined(DEBUG)
    if (num_points > 0 && len != num_points) {
        // BUG: https://github.com/mapbox/mapnik-vector-tile/issues/144
        MAPNIK_LOG_ERROR(decode_point) << "warning: encountered incorrectly encoded multipoint with " << num_points << " points but only " << len << " repeated commands";
    }
    #endif
    if (num_points == 0)
    {
        geom = mapnik::geometry::geometry_empty();
    }
    else if (num_points == 1)
    {
        geom = std::move(mp[0]);
    }
    else if (num_points > 1)
    {
        // return multipoint
        geom = std::move(mp);
    }
}

template <typename ValueType, typename T>
void decode_linestring(mapnik::geometry::geometry<ValueType> & geom, T & paths, mapnik::box2d<double> const& bbox)
{
    typename T::command cmd;
    ValueType x1, y1;
    mapnik::geometry::multi_line_string<ValueType> multi_line;
    multi_line.emplace_back();
    bool first = true;
    bool first_line_to = true;
    std::uint32_t len;
    #if defined(DEBUG)
    std::uint32_t pre_len;
    #endif
    mapnik::box2d<double> part_env;
    while ((cmd = paths.next(x1, y1, len)) != T::end)
    {
        if (cmd == T::move_to)
        {
            if (first)
            {
                first = false;
            }
            else
            {
                #if defined(DEBUG)
                if (multi_line.back().size() > 0 && pre_len != multi_line.back().size())
                {
                    MAPNIK_LOG_ERROR(decode_linestring) << "warning: encountered incorrectly encoded line with " << multi_line.back().size() << " points but only " << pre_len << " repeated commands";
                }
                #endif
                first_line_to = true;
                if (!bbox.intersects(part_env))
                {
                    // remove last line
                    multi_line.pop_back();
                }
                // add fresh line to start adding to
                multi_line.emplace_back();
            }
            part_env.init(x1,y1,x1,y1);
        }
        else if (first_line_to && cmd == T::line_to)
        {
            first_line_to = false;
            multi_line.back().reserve(len+1);
            #if defined(DEBUG)
            pre_len = len+1;
            #endif
        }
        if (!first)
        {
            part_env.expand_to_include(x1,y1);
        }
        multi_line.back().add_coord(x1,y1);
    }
    if (!bbox.intersects(part_env))
    {
        // remove last line
        multi_line.pop_back();
    }
    std::size_t num_lines = multi_line.size();
    if (num_lines == 0)
    {
        geom = mapnik::geometry::geometry_empty();
    }
    else if (num_lines == 1)
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

template <typename ValueType, typename T>
void read_rings(std::vector<mapnik::geometry::linear_ring<ValueType> > & rings,
                T & paths, mapnik::box2d<double> const& bbox)
{
    typename T::command cmd;
    ValueType x1, y1;
    rings.emplace_back();
    ValueType x2, y2;
    bool first = true;
    bool first_line_to = true;
    std::uint32_t len;
    #if defined(DEBUG)
    std::uint32_t pre_len;
    #endif
    mapnik::box2d<double> part_env;
    while ((cmd = paths.next(x1, y1, len)) != T::end)
    {
        if (cmd == T::move_to)
        {
            x2 = x1;
            y2 = y1;
            if (first)
            {
                first = false;
            }
            else
            {
                #if defined(DEBUG)
                // off by one is expected/okay in rare cases
                if (rings.back().size() > 0 && (rings.back().size() > pre_len || std::fabs(pre_len - rings.back().size()) > 1))
                {
                    MAPNIK_LOG_ERROR(read_rings) << "warning: encountered incorrectly encoded ring with " << rings.back().size() << " points but " << pre_len << " repeated commands";
                }
                #endif
                first_line_to = true;
                if (!bbox.intersects(part_env))
                {
                    // remove last ring
                    rings.pop_back();
                }
                rings.emplace_back();
            }
            part_env.init(x1,y1,x1,y1);
        }
        else if (first_line_to && cmd == T::line_to)
        {
            first_line_to = false;
            rings.back().reserve(len+2);
            #if defined(DEBUG)
            pre_len = len+2;
            #endif
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
        if (!first)
        {
            part_env.expand_to_include(x1,y1);
        }
        rings.back().add_coord(x1,y1);
    }
    if (!bbox.intersects(part_env))
    {
        // remove last ring
        rings.pop_back();
    }
}

template <typename ValueType, typename T>
void decode_polygons(mapnik::geometry::geometry<ValueType> & geom, T && rings)
{
    auto rings_itr = std::make_move_iterator(rings.begin());
    auto rings_end = std::make_move_iterator(rings.end());
    std::size_t num_rings = rings.size();
    if (num_rings == 0)
    {
        geom = mapnik::geometry::geometry_empty();
    }
    else if (num_rings == 1)
    {
        if (rings_itr->size() < 4) return;
        if (mapnik::util::is_clockwise(*rings_itr))
        {
            // Its clockwise, so lets reverse it.
            std::reverse(rings_itr->begin(), rings_itr->end());
        }
        // return the single polygon without interior rings
        mapnik::geometry::polygon<ValueType> poly;
        poly.set_exterior_ring(std::move(*rings_itr));
        geom = std::move(poly);
    }
    else
    {
        mapnik::geometry::multi_polygon<ValueType> multi_poly;
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
            geom = mapnik::geometry::polygon<ValueType>(std::move(*itr));
        }
        else
        {
            geom = std::move(multi_poly);
        }
    }
}

} // ns detail

template <typename ValueType, typename T>
inline mapnik::geometry::geometry<ValueType> decode_geometry(T & paths, int32_t geom_type, mapnik::box2d<double> const& bbox)
{
    mapnik::geometry::geometry<ValueType> geom; // output geometry
    switch (geom_type)
    {
    case vector_tile::Tile_GeomType_POINT:
    {
        detail::decode_point<ValueType, T>(geom, paths, bbox);
        break;
    }
    case vector_tile::Tile_GeomType_LINESTRING:
    {
        detail::decode_linestring<ValueType, T>(geom, paths, bbox);
        break;
    }
    case vector_tile::Tile_GeomType_POLYGON:
    {
        std::vector<mapnik::geometry::linear_ring<ValueType> > rings;
        detail::read_rings<ValueType, T>(rings, paths, bbox);
        if (rings.empty())
        {
            geom = mapnik::geometry::geometry_empty();
        }
        else
        {
            detail::decode_polygons(geom, rings);
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

// For back compatibility in tests / for cases where performance is not critical
// TODO: consider removing and always requiring bbox arg
template <typename ValueType, typename T>
inline mapnik::geometry::geometry<ValueType> decode_geometry(T & paths, int32_t geom_type)
{

    mapnik::box2d<double> bbox(std::numeric_limits<double>::lowest(),
                               std::numeric_limits<double>::lowest(),
                               std::numeric_limits<double>::max(),
                               std::numeric_limits<double>::max());
    return decode_geometry<ValueType>(paths,geom_type,bbox);
}

}} // end ns


#endif // __MAPNIK_VECTOR_TILE_GEOMETRY_DECODER_H__
