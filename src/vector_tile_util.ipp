#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include "vector_tile.pb.h"
#pragma GCC diagnostic pop

#include <mapnik/vertex.hpp>
#include <mapnik/box2d.hpp>
#include <mapnik/geometry.hpp>

#include "vector_tile_geometry_decoder.hpp"

#include <protozero/pbf_reader.hpp>

#include <stdexcept>
#include <string>
#include <sstream>

namespace mapnik { namespace vector_tile_impl {

    // ported from http://stackoverflow.com/a/1968345/2333354
    bool line_intersects(int p0_x, int p0_y, int p1_x, int p1_y,
                         int p2_x, int p2_y, int p3_x, int p3_y)
    {
        float s1_x = p1_x - p0_x;
        float s1_y = p1_y - p0_y;
        float s2_x = p3_x - p2_x;
        float s2_y = p3_y - p2_y;
        float a = (-s1_y * (p0_x - p2_x) + s1_x * (p0_y - p2_y));
        float b = (-s2_x * s1_y + s1_x * s2_y);
        if (b == 0 ) return false;
        float s = a / b;
        float c = ( s2_x * (p0_y - p2_y) - s2_y * (p0_x - p2_x));
        float d = (-s2_x * s1_y + s1_x * s2_y);
        if (d == 0 ) return false;
        float t = c / d;
        if (s >= 0 && s <= 1 && t >= 0 && t <= 1) return true;
        return false;
    }

    bool line_intersects_box(int p0_x, int p0_y, int p1_x, int p1_y, mapnik::box2d<int> box)
    {
        // possible early return for degenerate line
        if (p0_x == p1_x && p0_y == p1_y) return false;

        // check intersections with 4 sides of box
        /*
        0,1 ------ 2,1
         |          |
         |          |
        0,3 ------ 2,3
        */
        // bottom side: 0,3,2,3
        if (line_intersects(p0_x,p0_y,p1_x,p1_y,
                            box[0],box[3],box[2],box[3])) {
            return true;
        }
        // right side: 2,3,2,1
        else if (line_intersects(p0_x,p0_y,p1_x,p1_y,
                                 box[2],box[3],box[2],box[1])) {
            return true;
        }
        // top side: 0,1,2,1
        else if (line_intersects(p0_x,p0_y,p1_x,p1_y,
                                 box[0],box[1],box[2],box[1])) {
            return true;
        }
        // left side: 0,1,0,3
        else if (line_intersects(p0_x,p0_y,p1_x,p1_y,
                                 box[0],box[1],box[0],box[3])) {
            return true;
        }
        return false;
    }

    bool is_solid_extent(vector_tile::Tile const& tile, std::string & key)
    {
        for (int i = 0; i < tile.layers_size(); i++)
        {
            vector_tile::Tile_Layer const& layer = tile.layers(i);
            unsigned extent = layer.extent();
            unsigned side = extent - 1;
            // TODO: need to account for buffer here
            // NOTE: insetting box by 2 pixels is needed to account for
            // rounding issues (at least on right and bottom)
            mapnik::box2d<int> container(2, 2, extent-2, extent-2);
            double extent_area = side * side;
            for (int j = 0; j < layer.features_size(); j++)
            {
                vector_tile::Tile_Feature const& feature = layer.features(j);
                int cmd = -1;
                const int cmd_bits = 3;
                unsigned length = 0;
                bool first = true;
                mapnik::box2d<int> box;
                int32_t x1 = 0;
                int32_t y1 = 0;
                int32_t x0 = 0;
                int32_t y0 = 0;
                for (int k = 0; k < feature.geometry_size();)
                {
                    if (!length) {
                        unsigned cmd_length = feature.geometry(k++);
                        cmd = cmd_length & ((1 << cmd_bits) - 1);
                        length = cmd_length >> cmd_bits;
                    }
                    if (length > 0) {
                        length--;
                        if (cmd == mapnik::SEG_MOVETO || cmd == mapnik::SEG_LINETO)
                        {
                            int32_t dx = feature.geometry(k++);
                            int32_t dy = feature.geometry(k++);
                            dx = ((dx >> 1) ^ (-(dx & 1)));
                            dy = ((dy >> 1) ^ (-(dy & 1)));
                            x1 += dx;
                            y1 += dy;
                            if ((x1 > 0 && x1 < static_cast<int>(side)) && (y1 > 0 && y1 < static_cast<int>(side))) {
                                // We can abort early if this feature has a vertex that is
                                // inside the bbox.
                                return false;
                            } else if (!first && line_intersects_box(x0,y0,x1,y1,container)) {
                                // or if the last line segment intersects with the expected bounding rectangle
                                return false;
                            }
                            x0 = x1;
                            y0 = y1;
                            if (first)
                            {
                                box.init(x1,y1,x1,y1);
                                first = false;
                            }
                            else
                            {
                                box.expand_to_include(x1,y1);
                            }
                        }
                        else if (cmd == (mapnik::SEG_CLOSE & ((1 << cmd_bits) - 1)))
                        {
                            // pass
                        }
                        else
                        {
                            std::stringstream msg;
                            msg << "Unknown command type (is_solid_extent): "
                                << cmd;
                            throw std::runtime_error(msg.str());
                        }
                    }
                }
                // Once we have only one clipped result polygon, we can compare the
                // areas and return early if they don't match.
                int geom_area = box.width() * box.height();
                if (geom_area < (extent_area - 32) )
                {
                    return false;
                }
                if (i == 0) {
                    key = layer.name();
                } else if (i > 0) {
                    key += std::string("-") + layer.name();
                }
            }
        }

        // It's either empty or doesn't have features that have vertices that are
        // not on the border of the bbox.
        return true;
    }

    bool is_solid_extent(std::string const& tile, std::string & key)
    {
        protozero::pbf_reader item(tile);
        unsigned i = 0;
        while (item.next(3)) {
            protozero::pbf_reader layer_msg = item.get_message();
            unsigned extent = 0;
            std::string name;
            std::vector<protozero::pbf_reader> feature_collection;
            while (layer_msg.next())
            {
                switch(layer_msg.tag())
                {
                    case 1:
                        name = layer_msg.get_string();
                        break;
                    case 2:
                        feature_collection.push_back(layer_msg.get_message());
                        break;
                    case 5:
                        extent = layer_msg.get_uint32();
                        break;
                    default:
                        layer_msg.skip();
                        break;
                }
            }
            unsigned side = extent - 1;
            mapnik::box2d<int> container(2, 2, extent-2, extent-2);
            double extent_area = side * side;
            for (auto & features : feature_collection)
            {
                while (features.next(4)) {
                    mapnik::vector_tile_impl::GeometryPBF<double> paths(features.get_packed_uint32(), 0, 0, 1, 1);
                    mapnik::vector_tile_impl::GeometryPBF<double>::command cmd;
                    double x0, y0, x1, y1;
                    mapnik::box2d<int> box;
                    bool first = true;
                    std::uint32_t len;
                    while ((cmd = paths.next(x1, y1, len)) != mapnik::vector_tile_impl::GeometryPBF<double>::end)
                    {
                        if (cmd == mapnik::vector_tile_impl::GeometryPBF<double>::move_to || cmd == mapnik::vector_tile_impl::GeometryPBF<double>::line_to)
                        {
                            if ((x1 > 0 && x1 < static_cast<int>(side)) && (y1 > 0 && y1 < static_cast<int>(side)))
                            {
                                // We can abort early if this feature has a vertex that is
                                // inside the bbox.
                                return false;
                            }
                            else if (!first && line_intersects_box(x0,y0,x1,y1,container))
                            {
                                // or if the last line segment intersects with the expected bounding rectangle
                                return false;
                            }
                            x0 = x1;
                            y0 = y1;
                            if (first)
                            {
                                box.init(x1,y1,x1,y1);
                                first = false;
                            }
                            else
                            {
                                box.expand_to_include(x1,y1);
                            }
                        }
                    }
                    // Once we have only one clipped result polygon, we can compare the
                    // areas and return early if they don't match.
                    int geom_area = box.width() * box.height();
                    if (geom_area < (extent_area - 32) )
                    {
                        return false;
                    }
                    if (i == 0) {
                        key = name;
                    } else if (i > 0) {
                        key += std::string("-") + name;
                    }
                }
            }
            ++i;
        }
        return true;
    }

}} // end ns
