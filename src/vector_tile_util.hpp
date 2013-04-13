#ifndef __MAPNIK_VECTOR_TILE_UTIL_H__
#define __MAPNIK_VECTOR_TILE_UTIL_H__

#include "clipper.hpp"
#include "vector_tile.pb.h"
#include <mapnik/vertex.hpp>
#include <mapnik/box2d.hpp>
#include <mapnik/geometry.hpp>
#include <stdexcept>
#include <string>

namespace mapnik { namespace vector {

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
                        throw std::runtime_error("Unknown command type");
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

bool is_solid_extent(mapnik::vector::tile const& tile, std::string & key)
{
    for (int i = 0; i < tile.layers_size(); i++)
    {
        mapnik::vector::tile_layer const& layer = tile.layers(i);
        unsigned extent = layer.extent();
        unsigned side = extent - 1;
        double extent_area = side * side;
        for (int j = 0; j < layer.features_size(); j++)
        {
            mapnik::vector::tile_feature const& feature = layer.features(j);
            mapnik::eGeomType g_type = static_cast<mapnik::eGeomType>(feature.type());
            mapnik::geometry_type geom(g_type);
            int cmd = -1;
            const int cmd_bits = 3;
            unsigned length = 0;
            int32_t x = 0, y = 0;
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
                        geom.push_vertex(x, y, static_cast<mapnik::CommandType>(cmd));
                    }
                    else if (cmd == (mapnik::SEG_CLOSE & ((1 << cmd_bits) - 1)))
                    {
                        geom.push_vertex(0, 0, mapnik::SEG_CLOSE);
                    }
                    else
                    {
                        throw std::runtime_error("Unknown command type");
                    }
                }
            }
            // Once we have only one clipped result polygon, we can compare the
            // areas and return early if they don't match.
            mapnik::box2d<double> box = geom.envelope();
            double geom_area = box.width() * box.height();
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
