#ifndef __MAPNIK_VECTOR_TILE_GEOMETRY_DECODER_H__
#define __MAPNIK_VECTOR_TILE_GEOMETRY_DECODER_H__

//libprotobuf
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include "vector_tile.pb.h"
#pragma GCC diagnostic pop

//protozero
#include <protozero/pbf_reader.hpp>

//mapnik
#include <mapnik/box2d.hpp>
#include <mapnik/geometry.hpp>
#include <mapnik/util/is_clockwise.hpp>
#if defined(DEBUG)
#include <mapnik/debug.hpp>
#endif

//std
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace mapnik 
{ 
    
namespace vector_tile_impl 
{

template <typename ValueType>
inline void move_cursor(ValueType & x, ValueType & y, std::int32_t dx, std::int32_t dy, double scale_x_, double scale_y_)
{
    x += static_cast<ValueType>(std::round(static_cast<double>(dx) / scale_x_));
    y += static_cast<ValueType>(std::round(static_cast<double>(dy) / scale_y_));
}

template <>
inline void move_cursor<double>(double & x, double & y, std::int32_t dx, std::int32_t dy, double scale_x_, double scale_y_)
{
    x += static_cast<double>(dx) / scale_x_;
    y += static_cast<double>(dy) / scale_y_;
}

// NOTE: this object is for one-time use.  Once you've progressed to the end
//       by calling next(), to re-iterate, you must construct a new object
template <typename ValueType>
class Geometry {

public:
    explicit Geometry(vector_tile::Tile_Feature const& f,
                             ValueType tile_x, ValueType tile_y,
                             double scale_x, double scale_y);

    enum command : uint8_t {
        end = 0,
        move_to = 1,
        line_to = 2,
        close = 7
    };

    bool scaling_reversed_orientation() const
    {
        return (scale_x_ * scale_y_) < 0;
    }

    uint32_t get_length() const
    {
        return length;
    }

    command point_next(ValueType & rx, ValueType & ry);
    command line_next(ValueType & rx, ValueType & ry, bool skip_lineto_zero);
    command ring_next(ValueType & rx, ValueType & ry, bool skip_lineto_zero);
    
private:
    vector_tile::Tile_Feature const& f_;
    double scale_x_;
    double scale_y_;
    uint32_t k;
    uint32_t geoms_;
    ValueType x, y;
    ValueType ox, oy;
    uint32_t length;
    uint8_t cmd;
    #if defined(DEBUG)
public:
    bool already_had_error;
    #endif
};

// NOTE: this object is for one-time use.  Once you've progressed to the end
//       by calling next(), to re-iterate, you must construct a new object
template <typename ValueType>
class GeometryPBF {

public:
    explicit GeometryPBF(std::pair< protozero::pbf_reader::const_uint32_iterator, protozero::pbf_reader::const_uint32_iterator > const& geo_iterator,
                             ValueType tile_x, ValueType tile_y,
                             double scale_x, double scale_y);

    enum command : uint8_t
    {
        end = 0,
        move_to = 1,
        line_to = 2,
        close = 7
    };


    bool scaling_reversed_orientation() const
    {
        return (scale_x_ * scale_y_) < 0;
    }

    uint32_t get_length() const
    {
        return length;
    }

    command point_next(ValueType & rx, ValueType & ry);
    command line_next(ValueType & rx, ValueType & ry, bool skip_lineto_zero);
    command ring_next(ValueType & rx, ValueType & ry, bool skip_lineto_zero);

private:
    std::pair< protozero::pbf_reader::const_uint32_iterator, protozero::pbf_reader::const_uint32_iterator > geo_iterator_;
    double scale_x_;
    double scale_y_;
    ValueType x, y;
    ValueType ox, oy;
    uint32_t length;
    uint8_t cmd;
    #if defined(DEBUG)
public:
    bool already_had_error;
    #endif
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
      x(tile_x), y(tile_y),
      ox(0), oy(0),
      length(0),
      cmd(move_to)
    {
    #if defined(DEBUG)
    already_had_error = false;
    #endif
    }

template <typename ValueType>
typename Geometry<ValueType>::command Geometry<ValueType>::point_next(ValueType & rx, ValueType & ry)
{
    if (length == 0)
    {
        if (k < geoms_)
        {
            uint32_t cmd_length = static_cast<uint32_t>(f_.geometry(k++));
            cmd = cmd_length & 0x7;
            length = cmd_length >> 3;
            if (cmd == move_to)
            {
                if (length == 0 || k + (length * 2) > geoms_)
                {
                    throw std::runtime_error("Vector Tile has geometry with MOVETO command that is not followed by a proper number of parameters");
                }
            }
            else
            {
                if (cmd == line_to)
                {
                    throw std::runtime_error("Vector Tile has POINT type geometry with a LINETO command.");
                }
                else if (cmd == close)
                {
                    throw std::runtime_error("Vector Tile has POINT type geometry with a CLOSE command.");
                }
                else
                {
                    throw std::runtime_error("Vector Tile has POINT type geometry with an unknown command.");
                }
            }
        }
        else
        {
            return end;
        }
    }

    --length;
    int32_t dx = protozero::decode_zigzag32(f_.geometry(k++));
    int32_t dy = protozero::decode_zigzag32(f_.geometry(k++));
    move_cursor(x, y, dx, dy, scale_x_, scale_y_);
    rx = x;
    ry = y;
    return move_to;
}

template <typename ValueType>
typename Geometry<ValueType>::command Geometry<ValueType>::line_next(ValueType & rx, 
                                                                     ValueType & ry, 
                                                                     bool skip_lineto_zero)
{
    if (length == 0)
    {
        if (k < geoms_)
        {
            uint32_t cmd_length = static_cast<uint32_t>(f_.geometry(k++));
            cmd = cmd_length & 0x7;
            length = cmd_length >> 3;

            if (cmd == move_to)
            {
                if (length != 1 || k + 2 > geoms_)
                {
                    throw std::runtime_error("Vector Tile has LINESTRING with a MOVETO command that is given more then one pair of parameters or not enough parameters are provided");
                }
                --length;
                int32_t dx = protozero::decode_zigzag32(f_.geometry(k++));
                int32_t dy = protozero::decode_zigzag32(f_.geometry(k++));
                move_cursor(x, y, dx, dy, scale_x_, scale_y_);
                rx = x;
                ry = y;
                return move_to;
            }
            else if (cmd == line_to)
            {
                if (length == 0 || k + (length * 2) > geoms_)
                {
                    throw std::runtime_error("Vector Tile has geometry with LINETO command that is not followed by a proper number of parameters");
                }
            }
            else
            {
                if (cmd == close)
                {
                    throw std::runtime_error("Vector Tile has LINESTRING type geometry with a CLOSE command.");
                }
                else
                {
                    throw std::runtime_error("Vector Tile has LINESTRING type geometry with an unknown command.");
                }
            }
        }
        else
        {
            return end;
        }
    }

    --length;
    int32_t dx = protozero::decode_zigzag32(f_.geometry(k++));
    int32_t dy = protozero::decode_zigzag32(f_.geometry(k++));
    if (skip_lineto_zero && dx == 0 && dy == 0)
    {
        // We are going to skip this vertex as the point doesn't move call line_next again
        return line_next(rx,ry,true);
    }
    move_cursor(x, y, dx, dy, scale_x_, scale_y_);
    rx = x;
    ry = y;
    return line_to;
}

template <typename ValueType>
typename Geometry<ValueType>::command Geometry<ValueType>::ring_next(ValueType & rx, 
                                                                     ValueType & ry, 
                                                                     bool skip_lineto_zero)
{
    if (length == 0)
    {
        if (k < geoms_)
        {
            uint32_t cmd_length = static_cast<uint32_t>(f_.geometry(k++));
            cmd = cmd_length & 0x7;
            length = cmd_length >> 3;

            if (cmd == move_to)
            {
                if (length != 1 || k + 2 > geoms_)
                {
                    throw std::runtime_error("Vector Tile has POLYGON with a MOVETO command that is given more then one pair of parameters or not enough parameters are provided");
                }
                --length;
                int32_t dx = protozero::decode_zigzag32(f_.geometry(k++));
                int32_t dy = protozero::decode_zigzag32(f_.geometry(k++));
                move_cursor(x, y, dx, dy, scale_x_, scale_y_);
                rx = x;
                ry = y;
                ox = x;
                oy = y;
                return move_to;
            }
            else if (cmd == line_to)
            {
                if (length == 0 || k + (length * 2) > geoms_)
                {
                    throw std::runtime_error("Vector Tile has geometry with LINETO command that is not followed by a proper number of parameters");
                }
            }
            else if (cmd == close)
            {
                // Just set length in case a close command provides an invalid number here.
                // While we could throw because V2 of the spec declares it incorrect, this is not
                // difficult to fix and has no effect on the results.
                length = 0;
                rx = ox;
                ry = oy;
                return close;
            }
            else
            {
                throw std::runtime_error("Vector Tile has POLGYON type geometry with an unknown command.");
            }
        }
        else
        {
            return end;
        }
    }

    --length;
    int32_t dx = protozero::decode_zigzag32(f_.geometry(k++));
    int32_t dy = protozero::decode_zigzag32(f_.geometry(k++));
    if (skip_lineto_zero && dx == 0 && dy == 0)
    {
        // We are going to skip this vertex as the point doesn't move call ring_next again
        return ring_next(rx,ry,true);
    }
    move_cursor(x, y, dx, dy, scale_x_, scale_y_);
    rx = x;
    ry = y;
    return line_to;
}

template <typename ValueType>
GeometryPBF<ValueType>::GeometryPBF(std::pair<protozero::pbf_reader::const_uint32_iterator, protozero::pbf_reader::const_uint32_iterator > const& geo_iterator,
                   ValueType tile_x, ValueType tile_y,
                   double scale_x, double scale_y)
    : geo_iterator_(geo_iterator),
      scale_x_(scale_x),
      scale_y_(scale_y),
      x(tile_x), y(tile_y),
      ox(0), oy(0),
      length(0),
      cmd(move_to)
    {
    #if defined(DEBUG)
    already_had_error = false;
    #endif
    }

template <typename ValueType>
typename GeometryPBF<ValueType>::command GeometryPBF<ValueType>::point_next(ValueType & rx, ValueType & ry)
{
    if (length == 0) 
    {
        if (geo_iterator_.first != geo_iterator_.second)
        {
            uint32_t cmd_length = static_cast<uint32_t>(*geo_iterator_.first++);
            cmd = cmd_length & 0x7;
            length = cmd_length >> 3;
            if (cmd == move_to)
            {
                if (length == 0)
                {
                    throw std::runtime_error("Vector Tile has POINT geometry with a MOVETO command that has a command count of zero");
                }
            }
            else
            {
                if (cmd == line_to)
                {
                    throw std::runtime_error("Vector Tile has POINT type geometry with a LINETO command.");
                }
                else if (cmd == close)
                {
                    throw std::runtime_error("Vector Tile has POINT type geometry with a CLOSE command.");
                }
                else
                {
                    throw std::runtime_error("Vector Tile has POINT type geometry with an unknown command.");
                }
            }
        }
        else 
        {
            return end;
        }
    }

    --length;
    // It is possible for the next to lines to throw because we can not check the length
    // of the buffer to ensure that it is long enough.
    // If an exception occurs it will likely be a end_of_buffer_exception with the text:
    // "end of buffer exception"
    // While this error message is not verbose a try catch here would slow down processing.
    int32_t dx = protozero::decode_zigzag32(*geo_iterator_.first++);
    int32_t dy = protozero::decode_zigzag32(*geo_iterator_.first++);
    move_cursor(x, y, dx, dy, scale_x_, scale_y_);
    rx = x;
    ry = y;
    return move_to;
}

template <typename ValueType>
typename GeometryPBF<ValueType>::command GeometryPBF<ValueType>::line_next(ValueType & rx, 
                                                                           ValueType & ry, 
                                                                           bool skip_lineto_zero)
{
    if (length == 0)
    {
        if (geo_iterator_.first != geo_iterator_.second)
        {
            uint32_t cmd_length = static_cast<uint32_t>(*geo_iterator_.first++);
            cmd = cmd_length & 0x7;
            length = cmd_length >> 3;
            if (cmd == move_to)
            {
                if (length != 1)
                {
                    throw std::runtime_error("Vector Tile has LINESTRING with a MOVETO command that is given more then one pair of parameters or not enough parameters are provided");
                }
                --length;
                // It is possible for the next to lines to throw because we can not check the length
                // of the buffer to ensure that it is long enough.
                // If an exception occurs it will likely be a end_of_buffer_exception with the text:
                // "end of buffer exception"
                // While this error message is not verbose a try catch here would slow down processing.
                int32_t dx = protozero::decode_zigzag32(*geo_iterator_.first++);
                int32_t dy = protozero::decode_zigzag32(*geo_iterator_.first++);
                move_cursor(x, y, dx, dy, scale_x_, scale_y_);
                rx = x;
                ry = y;
                return move_to;
            }
            else if (cmd == line_to)
            {
                if (length == 0)
                {
                    throw std::runtime_error("Vector Tile has geometry with LINETO command that is not followed by a proper number of parameters");
                }
            }
            else
            {
                if (cmd == close)
                {
                    throw std::runtime_error("Vector Tile has LINESTRING type geometry with a CLOSE command.");
                }
                else
                {
                    throw std::runtime_error("Vector Tile has LINESTRING type geometry with an unknown command.");
                }
            }
        }
        else
        {
            return end;
        }
    }

    --length;
    // It is possible for the next to lines to throw because we can not check the length
    // of the buffer to ensure that it is long enough.
    // If an exception occurs it will likely be a end_of_buffer_exception with the text:
    // "end of buffer exception"
    // While this error message is not verbose a try catch here would slow down processing.
    int32_t dx = protozero::decode_zigzag32(*geo_iterator_.first++);
    int32_t dy = protozero::decode_zigzag32(*geo_iterator_.first++);
    if (skip_lineto_zero && dx == 0 && dy == 0)
    {
        // We are going to skip this vertex as the point doesn't move call line_next again
        return line_next(rx,ry,true);
    }
    move_cursor(x, y, dx, dy, scale_x_, scale_y_);
    rx = x;
    ry = y;
    return line_to;
}

template <typename ValueType>
typename GeometryPBF<ValueType>::command GeometryPBF<ValueType>::ring_next(ValueType & rx, 
                                                                           ValueType & ry, 
                                                                           bool skip_lineto_zero)
{
    if (length == 0)
    {
        if (geo_iterator_.first != geo_iterator_.second)
        {
            uint32_t cmd_length = static_cast<uint32_t>(*geo_iterator_.first++);
            cmd = cmd_length & 0x7;
            length = cmd_length >> 3;
            if (cmd == move_to)
            {
                if (length != 1)
                {
                    throw std::runtime_error("Vector Tile has POLYGON with a MOVETO command that is given more then one pair of parameters or not enough parameters are provided");
                }
                --length;
                // It is possible for the next to lines to throw because we can not check the length
                // of the buffer to ensure that it is long enough.
                // If an exception occurs it will likely be a end_of_buffer_exception with the text:
                // "end of buffer exception"
                // While this error message is not verbose a try catch here would slow down processing.
                int32_t dx = protozero::decode_zigzag32(*geo_iterator_.first++);
                int32_t dy = protozero::decode_zigzag32(*geo_iterator_.first++);
                move_cursor(x, y, dx, dy, scale_x_, scale_y_);
                rx = x;
                ry = y;
                ox = x;
                oy = y;
                return move_to;
            }
            else if (cmd == line_to)
            {
                if (length == 0)
                {
                    throw std::runtime_error("Vector Tile has geometry with LINETO command that is not followed by a proper number of parameters");
                }
            }
            else if (cmd == close)
            {
                // Just set length in case a close command provides an invalid number here.
                // While we could throw because V2 of the spec declares it incorrect, this is not
                // difficult to fix and has no effect on the results.
                length = 0;
                rx = ox;
                ry = oy;
                return close;
            }
            else
            {
                throw std::runtime_error("Vector Tile has POLYGON type geometry with an unknown command.");
            }
        }
        else
        {
            return end;
        }
    }

    --length;
    // It is possible for the next to lines to throw because we can not check the length
    // of the buffer to ensure that it is long enough.
    // If an exception occurs it will likely be a end_of_buffer_exception with the text:
    // "end of buffer exception"
    // While this error message is not verbose a try catch here would slow down processing.
    int32_t dx = protozero::decode_zigzag32(*geo_iterator_.first++);
    int32_t dy = protozero::decode_zigzag32(*geo_iterator_.first++);
    if (skip_lineto_zero && dx == 0 && dy == 0)
    {
        // We are going to skip this vertex as the point doesn't move call ring_next again
        return ring_next(rx,ry,true);
    }
    move_cursor(x, y, dx, dy, scale_x_, scale_y_);
    rx = x;
    ry = y;
    return line_to;
}

namespace detail {

template <typename ValueType, typename T>
void decode_point(mapnik::geometry::geometry<ValueType> & geom, T & paths, mapnik::box2d<double> const& bbox)
{
    typename T::command cmd;
    ValueType x1, y1;
    mapnik::geometry::multi_point<ValueType> mp;
    #if defined(DEBUG)
    std::uint32_t previous_len = 0;
    #endif
    // Find first moveto inside bbox and then reserve points from size of geometry.
    while (true)
    {
        cmd = paths.point_next(x1, y1);
        if (cmd == T::end)
        {
            geom = mapnik::geometry::geometry_empty();
            return;
        } 
        else if (bbox.intersects(x1,y1))
        {
            #if defined(DEBUG)
            if (previous_len <= paths.get_length() && !paths.already_had_error)
            {
                MAPNIK_LOG_WARN(decode_point) << "warning: encountered POINT geometry that might have MOVETO commands repeated that could be fewer commands";
                paths.already_had_error = true;
            }
            previous_len = paths.get_length();
            #endif
            mp.reserve(paths.get_length());
            mp.emplace_back(x1,y1);
            break;
        }
    }
    while ((cmd = paths.point_next(x1, y1)) != T::end)
    {
        #if defined(DEBUG)
        if (previous_len <= paths.get_length() && !paths.already_had_error)
        {
            MAPNIK_LOG_WARN(decode_point) << "warning: encountered POINT geometry that might have MOVETO commands repeated that could be fewer commands";
            paths.already_had_error = true;
        }
        previous_len = paths.get_length();
        #endif
        // TODO: consider profiling and trying to optimize this further
        // when all points are within the bbox filter then the `mp.reserve` should be
        // perfect, but when some points are thrown out we will allocate more than needed
        // the "all points intersect" case I think is going to be more common/important
        // however worth a future look to see if the "some or few points intersect" can be optimized
        if (!bbox.intersects(x1,y1))
        {
            continue;
        }
        mp.emplace_back(x1,y1);
    }
    std::size_t num_points = mp.size();
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
void decode_linestring(mapnik::geometry::geometry<ValueType> & geom, T & paths, 
                       mapnik::box2d<double> const& bbox,
                       unsigned version)
{
    typename T::command cmd;
    ValueType x0, y0;
    ValueType x1, y1;
    mapnik::geometry::multi_line_string<ValueType> multi_line;
    #if defined(DEBUG)
    std::uint32_t previous_len = 0;
    #endif
    mapnik::box2d<double> part_env;
    cmd = paths.line_next(x0, y0, false);
    if (cmd == T::end)
    {
        geom = mapnik::geometry::geometry_empty();
        return;
    }
    else if (cmd != T::move_to)
    {
        throw std::runtime_error("Vector Tile has LINESTRING type geometry where the first command is not MOVETO.");
    }

    while (true)
    {
        cmd = paths.line_next(x1, y1, true);
        if (cmd != T::line_to)
        {
            if (cmd == T::move_to)
            {
                if (version == 1)
                {
                    // Version 1 of the spec wasn't clearly defined and therefore
                    // we shouldn't be strict on the reading of a tile that has two
                    // moveto commands that are repeated, lets ignore the previous moveto.
                    x0 = x1;
                    y0 = y1;
                    continue;
                }
                else
                {
                    throw std::runtime_error("Vector Tile has LINESTRING type geometry with repeated MOVETO commands.");
                }
            }
            else //cmd == T::end
            {   
                if (version == 1)
                {
                    // Version 1 of the spec wasn't clearly defined and therefore
                    // we shouldn't be strict on the reading of a tile that has only a moveto 
                    // command. So lets just ignore this moveto command.
                    break;
                }
                else
                {
                    throw std::runtime_error("Vector Tile has LINESTRING type geometry with a MOVETO command with no LINETO following.");
                }
            }
        }
        // add fresh line
        multi_line.emplace_back();
        auto & line = multi_line.back();
        // reserve prior
        line.reserve(paths.get_length() + 1);
        // add moveto command position
        line.add_coord(x0, y0);
        part_env.init(x0, y0, x0, y0);
        // add first lineto
        line.add_coord(x1, y1);
        part_env.expand_to_include(x1, y1);
        #if defined(DEBUG)
        previous_len = paths.get_length();
        #endif
        while ((cmd = paths.line_next(x1, y1, true)) == T::line_to)
        {
            line.add_coord(x1, y1);
            part_env.expand_to_include(x1, y1);
            #if defined(DEBUG)
            if (previous_len <= paths.get_length() && !paths.already_had_error)
            {
                MAPNIK_LOG_WARN(decode_linestring) << "warning: encountered LINESTRING geometry that might have LINETO commands repeated that could be fewer commands";
                paths.already_had_error = true;
            }
            previous_len = paths.get_length();
            #endif
        }
        if (!bbox.intersects(part_env))
        {
            // remove last linestring
            multi_line.pop_back();
        }
        if (cmd == T::end)
        {
            break;
        } 
        // else we are gauranteed it is a moveto
        x0 = x1;
        y0 = y1;
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
                T & paths, mapnik::box2d<double> const& bbox,
                unsigned version)
{
    typename T::command cmd;
    ValueType x0, y0;
    ValueType x1, y1;
    ValueType x2, y2;
    #if defined(DEBUG)
    std::uint32_t previous_len;
    #endif
    mapnik::box2d<double> part_env;
    cmd = paths.ring_next(x0, y0, false);
    if (cmd == T::end)
    {
        return;
    }
    else if (cmd != T::move_to)
    {
        throw std::runtime_error("Vector Tile has POLYGON type geometry where the first command is not MOVETO.");
    }

    while (true)
    {
        cmd = paths.ring_next(x1, y1, true);
        if (cmd != T::line_to)
        {
            if (cmd == T::close && version == 1)
            {
                // Version 1 of the specification we were not clear on the command requirements for polygons
                // lets just to recover from this situation.
                cmd = paths.ring_next(x0, y0, false);
                if (cmd == T::end)
                {
                    break;
                }
                else if (cmd == T::move_to)
                {
                    continue;
                }
                else if (cmd == T::close)
                {
                    throw std::runtime_error("Vector Tile has POLYGON type geometry where a CLOSE is followed by a CLOSE.");
                }
                else // cmd == T::line_to
                {
                    throw std::runtime_error("Vector Tile has POLYGON type geometry where a CLOSE is followed by a LINETO.");
                }
            }
            else // cmd == end || cmd == move_to
            {
                throw std::runtime_error("Vector Tile has POLYGON type geometry with a MOVETO command with out at least two LINETOs and CLOSE following.");
            }
        }
        #if defined(DEBUG)
        previous_len = paths.get_length();
        #endif
        cmd = paths.ring_next(x2, y2, true);
        if (cmd != T::line_to)
        {
            if (cmd == T::close && version == 1)
            {
                // Version 1 of the specification we were not clear on the command requirements for polygons
                // lets just to recover from this situation.
                cmd = paths.ring_next(x0, y0, false);
                if (cmd == T::end)
                {
                    break;
                }
                else if (cmd == T::move_to)
                {
                    continue;
                }
                else if (cmd == T::close)
                {
                    throw std::runtime_error("Vector Tile has POLYGON type geometry where a CLOSE is followed by a CLOSE.");
                }
                else // cmd == T::line_to
                {
                    throw std::runtime_error("Vector Tile has POLYGON type geometry where a CLOSE is followed by a LINETO.");
                }
            }
            else // cmd == end || cmd == move_to
            {
                throw std::runtime_error("Vector Tile has POLYGON type geometry with a MOVETO command with out at least two LINETOs and CLOSE following.");
            }
        }
        // add new ring to start adding to
        rings.emplace_back();
        auto & ring = rings.back();
        // reserve prior
        ring.reserve(paths.get_length() + 2);
        // add moveto command position
        ring.add_coord(x0, y0);
        part_env.init(x0, y0, x0, y0);
        // add first lineto
        ring.add_coord(x1, y1);
        part_env.expand_to_include(x1, y1);
        // add second lineto
        ring.add_coord(x2, y2);
        part_env.expand_to_include(x2, y2);
        #if defined(DEBUG)
        if (previous_len <= paths.get_length() && !paths.already_had_error)
        {
            MAPNIK_LOG_WARN(read_rings) << "warning: encountered POLYGON geometry that might have LINETO commands repeated that could be fewer commands";
            paths.already_had_error = true;
        }
        previous_len = paths.get_length();
        #endif
        while ((cmd = paths.ring_next(x1, y1, true)) == T::line_to)
        {
            ring.add_coord(x1,y1);
            part_env.expand_to_include(x1,y1);
            #if defined(DEBUG)
            if (previous_len <= paths.get_length() && !paths.already_had_error)
            {
                MAPNIK_LOG_WARN(read_rings) << "warning: encountered POLYGON geometry that might have LINETO commands repeated that could be fewer commands";
                paths.already_had_error = true;
            }
            previous_len = paths.get_length();
            #endif
        }
        // Make sure we are now on a close command
        if (cmd != T::close)
        {
            throw std::runtime_error("Vector Tile has POLYGON type geometry with a ring not closed by a CLOSE command.");
        }
        if (ring.back().x != x0 || ring.back().y != y0)
        {
            // If the previous lineto didn't already close the polygon (WHICH IT SHOULD NOT)
            // close out the polygon ring.
            ring.add_coord(x0,y0);
        }

        if (!bbox.intersects(part_env))
        {
            // remove last linestring
            rings.pop_back();
        }

        cmd = paths.ring_next(x0, y0, false);
        if (cmd == T::end)
        {
            break;
        }
        else if (cmd != T::move_to)
        {
            if (cmd == T::close)
            {
                throw std::runtime_error("Vector Tile has POLYGON type geometry where a CLOSE is followed by a CLOSE.");
            }
            else // cmd == T::line_to
            {
                throw std::runtime_error("Vector Tile has POLYGON type geometry where a CLOSE is followed by a LINETO.");
            }
        }
    }
}

template <typename ValueType, typename T>
void decode_polygons(mapnik::geometry::geometry<ValueType> & geom, T && rings, unsigned version, bool scaling_reversed_orientation)
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
        if (rings_itr->size() < 4)
        {
            if (version == 1)
            {
                geom = mapnik::geometry::geometry_empty();
                return;
            }
            else
            {
                throw std::runtime_error("Vector Tile has POLYGON has ring with too few points. It is not valid according to v2 of VT spec.");
            }
        }
        
        // We are going to check if the current ring is clockwise, keeping in mind that
        // the orientation could have flipped due to scaling.
        if (mapnik::util::is_clockwise(*rings_itr))
        {
            if (scaling_reversed_orientation)
            {
                // Because the way we scaled changed the orientation of the ring (most likely due to a flipping of axis)
                // we now have an exterior ring that clockwise. Internally in mapnik we want this to be counter clockwise
                // so we must flip this.
                std::reverse(rings_itr->begin(), rings_itr->end());
            }
            else
            {
                if (version == 1)
                {
                    std::reverse(rings_itr->begin(), rings_itr->end());
                }
                else
                {
                    throw std::runtime_error("Vector Tile has POLYGON with first ring clockwise. It is not valid according to v2 of VT spec.");
                }
            }
        }
        else if (scaling_reversed_orientation) // ring is counter clockwise
        {
            // ring is counter clockwise but scaling reversed it.
            // this means in the vector tile it is incorrectly orientated.
            if (version == 1)
            {
                std::reverse(rings_itr->begin(), rings_itr->end());
            }
            else
            {
                throw std::runtime_error("Vector Tile has POLYGON with first ring clockwise. It is not valid according to v2 of VT spec.");
            }
        }
        // else ring is counter clockwise and scaling didn't reverse orientation.

        // return the single polygon without interior rings
        mapnik::geometry::polygon<ValueType> poly;
        poly.set_exterior_ring(std::move(*rings_itr));
        geom = std::move(poly);
    }
    else if (version == 1)
    {
        // Version 1 didn't specify the winding order, so we are forced
        // to do a best guess. This means assuming the first ring is an exterior
        // ring and then basing all winding order off that. For this we can
        // ignore if the scaling reversed the orientation.
        mapnik::geometry::multi_polygon<ValueType> multi_poly;
        bool first = true;
        bool first_is_clockwise = false;
        for (; rings_itr != rings_end; ++rings_itr)
        {
            if (rings_itr->size() < 4)
            {
                continue;
            }
            if (first)
            {
                first_is_clockwise = mapnik::util::is_clockwise(*rings_itr);
                // first ring always exterior and sets all future winding order
                multi_poly.emplace_back();
                if (first_is_clockwise)
                {
                    // Going into mapnik we want the outer ring to be CCW
                    std::reverse(rings_itr->begin(), rings_itr->end());
                }
                multi_poly.back().set_exterior_ring(std::move(*rings_itr));
                first = false;
            }
            else if (first_is_clockwise == mapnik::util::is_clockwise(*rings_itr))
            {
                // hit a new exterior ring, so start a new polygon
                multi_poly.emplace_back(); // start new polygon
                if (first_is_clockwise)
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
                if (first_is_clockwise)
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
    else // if (version == 2)
    {
        mapnik::geometry::multi_polygon<ValueType> multi_poly;
        bool first = true;
        for (; rings_itr != rings_end; ++rings_itr)
        {
            if (rings_itr->size() < 4)
            {
                throw std::runtime_error("Vector Tile has POLYGON has ring with too few points. It is not valid according to v2 of VT spec.");
            }

            if (first)
            {
                if (mapnik::util::is_clockwise(*rings_itr))
                {
                    if (scaling_reversed_orientation)
                    {
                        // Need to reverse ring so that it is proper going into mapnik.
                        // as mapnik needs CCW exterior rings
                        std::reverse(rings_itr->begin(), rings_itr->end());
                    }
                    else
                    {
                        throw std::runtime_error("Vector Tile has POLYGON with first ring clockwise. It is not valid according to v2 of VT spec.");
                    }
                } 
                else if (scaling_reversed_orientation)
                {
                    throw std::runtime_error("Vector Tile has POLYGON with first ring clockwise. It is not valid according to v2 of VT spec.");
                }
                multi_poly.emplace_back();
                multi_poly.back().set_exterior_ring(std::move(*rings_itr));
                first = false;
            }
            else if (mapnik::util::is_clockwise(*rings_itr)) // interior ring
            {
                if (scaling_reversed_orientation)
                {
                    // This is an exterior ring but needs to be reversed.
                    std::reverse(rings_itr->begin(), rings_itr->end());
                    multi_poly.emplace_back(); // start new polygon
                    multi_poly.back().set_exterior_ring(std::move(*rings_itr));
                }
                else
                {
                    // this is an interior ring
                    multi_poly.back().add_hole(std::move(*rings_itr));
                }
            }
            else 
            {
                if (scaling_reversed_orientation)
                {
                    // This is interior ring and it must be reversed.
                    std::reverse(rings_itr->begin(), rings_itr->end());
                    multi_poly.back().add_hole(std::move(*rings_itr));
                }
                else
                {
                    // hit a new exterior ring, so start a new polygon
                    multi_poly.emplace_back(); // start new polygon
                    multi_poly.back().set_exterior_ring(std::move(*rings_itr));
                }
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
inline mapnik::geometry::geometry<ValueType> decode_geometry(T & paths, 
                                                             int32_t geom_type, 
                                                             mapnik::box2d<double> const& bbox, 
                                                             unsigned version)
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
        detail::decode_linestring<ValueType, T>(geom, paths, bbox, version);
        break;
    }
    case vector_tile::Tile_GeomType_POLYGON:
    {
        std::vector<mapnik::geometry::linear_ring<ValueType> > rings;
        detail::read_rings<ValueType, T>(rings, paths, bbox, version);
        if (rings.empty())
        {
            geom = mapnik::geometry::geometry_empty();
        }
        else
        {
            detail::decode_polygons(geom, rings, version, paths.scaling_reversed_orientation());
        }
        break;
    }
    case vector_tile::Tile_GeomType_UNKNOWN:
    default:
    {
        // This was changed to not throw as unknown according to v2 of spec can simply be ignored and doesn't require
        // it failing the processing
        geom = mapnik::geometry::geometry_empty();
        break;
    }
    }
    return geom;
}

// For back compatibility in tests / for cases where performance is not critical
// TODO: consider removing and always requiring bbox arg
template <typename ValueType, typename T>
inline mapnik::geometry::geometry<ValueType> decode_geometry(T & paths, int32_t geom_type, unsigned version)
{

    mapnik::box2d<double> bbox(std::numeric_limits<double>::lowest(),
                               std::numeric_limits<double>::lowest(),
                               std::numeric_limits<double>::max(),
                               std::numeric_limits<double>::max());
    return decode_geometry<ValueType>(paths, geom_type, bbox, version);
}

}} // end ns


#endif // __MAPNIK_VECTOR_TILE_GEOMETRY_DECODER_H__
