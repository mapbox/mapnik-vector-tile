#ifndef __MAPNIK_VECTOR_TILE_UTIL_H__
#define __MAPNIK_VECTOR_TILE_UTIL_H__

#include "vector_tile.pb.h"
#include <mapnik/vertex.hpp>
#include <mapnik/box2d.hpp>
#include <mapnik/geometry.hpp>
#include <stdexcept>
#include <string>

#ifdef CONV_CLIPPER
#include "clipper.hpp"
#endif

namespace mapnik { namespace vector {

#ifdef CONV_CLIPPER
    bool is_solid_clipper(mapnik::vector::tile const& tile, std::string & key)
    {
        ClipperLib::Clipper clipper;

        for (int i = 0; i < tile.layers_size(); i++)
        {
            mapnik::vector::tile_layer const& layer = tile.layers(i);
            unsigned extent = layer.extent();
            unsigned side = extent - 1;
            double extent_area = side * side;

            ClipperLib::Polygon clip_box;
            clip_box.push_back(ClipperLib::IntPoint(0, 0));
            clip_box.push_back(ClipperLib::IntPoint(side, 0));
            clip_box.push_back(ClipperLib::IntPoint(side, side));
            clip_box.push_back(ClipperLib::IntPoint(0, side));
            clip_box.push_back(ClipperLib::IntPoint(0, 0));

            for (int j = 0; j < layer.features_size(); j++)
            {
                mapnik::vector::tile_feature const& feature = layer.features(j);

                int cmd = -1;
                const int cmd_bits = 3;
                unsigned length = 0;
                int32_t x = 0, y = 0;
                int32_t start_x = 0, start_y = 0;

                ClipperLib::Polygons geometry;
                ClipperLib::Polygon polygon;

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
                            x += ((dx >> 1) ^ (-(dx & 1)));
                            y += ((dy >> 1) ^ (-(dy & 1)));

                            // We can abort early if this feature has a vertex that is
                            // inside the bbox.
                            if ((x > 0 && x < static_cast<int>(side)) && (y > 0 && y < static_cast<int>(side))) {
                                return false;
                            }

                            if (cmd == mapnik::SEG_MOVETO) {
                                start_x = x;
                                start_y = y;
                                geometry.push_back(polygon);
                                polygon.clear();
                            }

                            polygon.push_back(ClipperLib::IntPoint(x, y));
                        }
                        else if (cmd == (mapnik::SEG_CLOSE & ((1 << cmd_bits) - 1)))
                        {
                            polygon.push_back(ClipperLib::IntPoint(start_x, start_y));
                            geometry.push_back(polygon);
                            polygon.clear();
                        }
                        else
                        {
                            std::stringstream msg;
                            msg << "Unknown command type (is_solid_clipper): "
                                << cmd;
                            throw std::runtime_error(msg.str());
                        }
                    }
                }

                ClipperLib::Polygons solution;

                clipper.Clear();
                clipper.AddPolygons(geometry, ClipperLib::ptSubject);
                clipper.AddPolygon(clip_box, ClipperLib::ptClip);
                clipper.Execute(ClipperLib::ctIntersection, solution, ClipperLib::pftNonZero, ClipperLib::pftNonZero);

                // If there are more than one result polygons, it can't be covered by
                // a single feature. Similarly, if there's no result, this geometry
                // is completely outside the bounding box.
                if (solution.size() != 1) {
                    return false;
                }

                // Once we have only one clipped result polygon, we can compare the
                // areas and return early if they don't match.
                double area = ClipperLib::Area(solution.front());
                if (area != extent_area) {
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
#endif

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
        if (s > 1 || s < 1) return false;
        float c = ( s2_x * (p0_y - p2_y) - s2_y * (p0_x - p2_x));
        float d = (-s2_x * s1_y + s1_x * s2_y);
        if (d == 0 ) return false;
        float t = c / d;
        if (t > 1 || t < 1) return false;
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

    bool is_solid_extent(mapnik::vector::tile const& tile, std::string & key)
    {
        for (int i = 0; i < tile.layers_size(); i++)
        {
            mapnik::vector::tile_layer const& layer = tile.layers(i);
            unsigned extent = layer.extent();
            unsigned side = extent - 1;
            // TODO: need to account for buffer here
            // NOTE: insetting box by 2 pixels is needed to account for
            // rounding issues (at least on right and bottom)
            mapnik::box2d<int> container(2, 2, extent-2, extent-2);
            double extent_area = side * side;
            for (int j = 0; j < layer.features_size(); j++)
            {
                mapnik::vector::tile_feature const& feature = layer.features(j);
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

    }} // end ns

#endif // __MAPNIK_VECTOR_TILE_UTIL_H__
