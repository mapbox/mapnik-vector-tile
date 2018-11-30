// mapnik-vector-tile
#include "vector_tile_config.hpp"

// mapbox
#include <mapbox/geometry/geometry.hpp>

// protozero
#include <protozero/varint.hpp>
#include <protozero/pbf_writer.hpp>

// std
#include <cstdlib>
#include <cmath>

namespace mapnik
{

namespace vector_tile_impl
{

namespace detail_pbf
{

template <typename Geometry>
inline std::size_t repeated_point_count(Geometry const& geom)
{
    if (geom.size() < 2)
    {
        return 0;
    }
    std::size_t count = 0;
    auto itr = geom.begin();

    for (auto prev_itr = itr++; itr != geom.end(); ++prev_itr, ++itr)
    {
        if (itr->x == prev_itr->x && itr->y == prev_itr->y)
        {
            count++;
        }
    }
    return count;
}

inline unsigned encode_length(unsigned len)
{
    return (len << 3u) | 2u;
}

struct encoder_visitor
{
    protozero::pbf_writer & feature_;
    int32_t & x_;
    int32_t & y_;

    encoder_visitor(protozero::pbf_writer & feature,
                    int32_t & x,
                    int32_t & y)
        : feature_(feature),
          x_(x),
          y_(y) {}

    bool operator() (mapbox::geometry::geometry_collection<std::int64_t> const&)
    {
        throw std::runtime_error("Geometry collections can not be encoded as they may contain different geometry types");
    }
    template <typename T>
    bool operator() (T const& geom)
    {
        return encode_geometry_pbf(geom, feature_, x_, y_);
    }
};

inline bool encode_linestring(mapbox::geometry::line_string<std::int64_t> const& line,
                              protozero::packed_field_uint32 & geometry,
                              int32_t & start_x,
                              int32_t & start_y)
{
    std::size_t line_size = line.size();
    line_size -= detail_pbf::repeated_point_count(line);
    if (line_size < 2)
    {
        return false;
    }

    unsigned line_to_length = static_cast<unsigned>(line_size) - 1;

    auto pt = line.begin();
    geometry.add_element(9); // move_to | (1 << 3)
    geometry.add_element(protozero::encode_zigzag32(pt->x - start_x));
    geometry.add_element(protozero::encode_zigzag32(pt->y - start_y));
    start_x = pt->x;
    start_y = pt->y;
    geometry.add_element(detail_pbf::encode_length(line_to_length));
    for (++pt; pt != line.end(); ++pt)
    {
        int32_t dx = pt->x - start_x;
        int32_t dy = pt->y - start_y;
        if (dx == 0 && dy == 0)
        {
            continue;
        }
        geometry.add_element(protozero::encode_zigzag32(dx));
        geometry.add_element(protozero::encode_zigzag32(dy));
        start_x = pt->x;
        start_y = pt->y;
    }
    return true;
}

inline bool encode_linearring(mapbox::geometry::linear_ring<std::int64_t> const& ring,
                              protozero::packed_field_uint32 & geometry,
                              int32_t & start_x,
                              int32_t & start_y)
{
    std::size_t ring_size = ring.size();
    ring_size -= detail_pbf::repeated_point_count(ring);
    if (ring_size < 3)
    {
        return false;
    }
    auto last_itr = ring.end();
    if (ring.front() == ring.back())
    {
        --last_itr;
        --ring_size;
        if (ring_size < 3)
        {
            return false;
        }
    }

    unsigned line_to_length = static_cast<unsigned>(ring_size) - 1;
    auto pt = ring.begin();
    geometry.add_element(9); // move_to | (1 << 3)
    geometry.add_element(protozero::encode_zigzag32(pt->x - start_x));
    geometry.add_element(protozero::encode_zigzag32(pt->y - start_y));
    start_x = pt->x;
    start_y = pt->y;
    geometry.add_element(detail_pbf::encode_length(line_to_length));
    for (++pt; pt != last_itr; ++pt)
    {
        int32_t dx = pt->x - start_x;
        int32_t dy = pt->y - start_y;
        if (dx == 0 && dy == 0)
        {
            continue;
        }
        geometry.add_element(protozero::encode_zigzag32(dx));
        geometry.add_element(protozero::encode_zigzag32(dy));
        start_x = pt->x;
        start_y = pt->y;
    }
    geometry.add_element(15); // close_path
    return true;
}

inline bool encode_polygon(mapbox::geometry::polygon<std::int64_t> const& poly,
                           protozero::packed_field_uint32 & geometry,
                           int32_t & start_x,
                           int32_t & start_y)
{
    if (poly.empty()) {
        return false;
    }
    bool first = true;
    for (auto const& ring : poly)
    {
        if (first) {
            first = false;
            if (!encode_linearring(ring, geometry, start_x, start_y))
            {
                return false;
            }
        } else {
            encode_linearring(ring, geometry, start_x, start_y);
        }
    }
    return true;
}

} // end ns detail

template <typename T>
MAPNIK_VECTOR_INLINE bool encode_geometry_pbf(T const& pt,
                                              protozero::pbf_writer & current_feature,
                                              int32_t & start_x,
                                              int32_t & start_y) { return false;}

template <>
MAPNIK_VECTOR_INLINE bool encode_geometry_pbf(mapbox::geometry::point<std::int64_t> const& pt,
                                              protozero::pbf_writer & current_feature,
                                              int32_t & start_x,
                                              int32_t & start_y)
{
    current_feature.add_enum(Feature_Encoding::TYPE, Geometry_Type::POINT);
    {
        protozero::packed_field_uint32 geometry(current_feature, Feature_Encoding::GEOMETRY);
        geometry.add_element(9);
        int32_t dx = pt.x - start_x;
        int32_t dy = pt.y - start_y;
        // Manual zigzag encoding.
        geometry.add_element(protozero::encode_zigzag32(dx));
        geometry.add_element(protozero::encode_zigzag32(dy));
        start_x = pt.x;
        start_y = pt.y;
    }
    return true;
}

template <>
MAPNIK_VECTOR_INLINE bool encode_geometry_pbf(mapbox::geometry::multi_point<std::int64_t> const& geom,
                                              protozero::pbf_writer & current_feature,
                                              int32_t & start_x,
                                              int32_t & start_y)
{
    std::size_t geom_size = geom.size();
    if (geom_size <= 0)
    {
        return false;
    }
    current_feature.add_enum(Feature_Encoding::TYPE, Geometry_Type::POINT);
    {
        protozero::packed_field_uint32 geometry(current_feature, Feature_Encoding::GEOMETRY);
        geometry.add_element(1u | (geom_size << 3)); // move_to | (len << 3)
        for (auto const& pt : geom)
        {
            int32_t dx = pt.x - start_x;
            int32_t dy = pt.y - start_y;
            // Manual zigzag encoding.
            geometry.add_element(protozero::encode_zigzag32(dx));
            geometry.add_element(protozero::encode_zigzag32(dy));
            start_x = pt.x;
            start_y = pt.y;
        }
    }
    return true;
}

template <>
MAPNIK_VECTOR_INLINE bool encode_geometry_pbf(mapbox::geometry::line_string<std::int64_t> const& line,
                                              protozero::pbf_writer & current_feature,
                                              int32_t & start_x,
                                              int32_t & start_y)
{
    bool success = false;
    current_feature.add_enum(Feature_Encoding::TYPE, Geometry_Type::LINESTRING);
    {
        protozero::packed_field_uint32 geometry(current_feature, Feature_Encoding::GEOMETRY);
        success = detail_pbf::encode_linestring(line, geometry, start_x, start_y);
    }
    return success;
}

template <>
MAPNIK_VECTOR_INLINE bool encode_geometry_pbf(mapbox::geometry::multi_line_string<std::int64_t> const& geom,
                                              protozero::pbf_writer & current_feature,
                                              int32_t & start_x,
                                              int32_t & start_y)
{
    bool success = false;
    current_feature.add_enum(Feature_Encoding::TYPE, Geometry_Type::LINESTRING);
    {
        protozero::packed_field_uint32 geometry(current_feature, Feature_Encoding::GEOMETRY);
        for (auto const& line : geom)
        {
            if (detail_pbf::encode_linestring(line, geometry, start_x, start_y))
            {
                success = true;
            }
        }
    }
    return success;
}
template <>
MAPNIK_VECTOR_INLINE bool encode_geometry_pbf(mapbox::geometry::polygon<std::int64_t> const& poly,
                                              protozero::pbf_writer & current_feature,
                                              int32_t & start_x,
                                              int32_t & start_y)
{
    bool success = false;
    current_feature.add_enum(Feature_Encoding::TYPE, Geometry_Type::POLYGON);
    {
        protozero::packed_field_uint32 geometry(current_feature, Feature_Encoding::GEOMETRY);
        success = detail_pbf::encode_polygon(poly, geometry, start_x, start_y);
    }
    return success;
}

template <>
MAPNIK_VECTOR_INLINE bool encode_geometry_pbf(mapbox::geometry::multi_polygon<std::int64_t> const& geom,
                                              protozero::pbf_writer & current_feature,
                                              int32_t & start_x,
                                              int32_t & start_y)
{
    bool success = false;
    current_feature.add_enum(Feature_Encoding::TYPE, Geometry_Type::POLYGON);
    {
        protozero::packed_field_uint32 geometry(current_feature, Feature_Encoding::GEOMETRY);
        for (auto const& poly : geom)
        {
            if (detail_pbf::encode_polygon(poly, geometry, start_x, start_y))
            {
                success = true;
            }
        }
    }
    return success;
}

template <>
MAPNIK_VECTOR_INLINE bool encode_geometry_pbf(mapbox::geometry::geometry<std::int64_t> const& geom,
                                              protozero::pbf_writer & current_feature,
                                              int32_t & start_x,
                                              int32_t & start_y)
{
    detail_pbf::encoder_visitor ap(current_feature, start_x, start_y);
    return mapbox::util::apply_visitor(ap, geom);
}

} // end ns vector_tile_impl

} // end ns mapnik
