#pragma once

#include <mapbox/geometry/box.hpp>
#include <mapbox/geometry/line_string.hpp>
#include <mapbox/geometry/multi_line_string.hpp>
#include <mapbox/geometry/multi_polygon.hpp>
#include <mapbox/geometry/multi_point.hpp>
#include <mapbox/geometry/point.hpp>
#include <mapbox/geometry/polygon.hpp>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wshadow"
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#endif
#pragma GCC diagnostic ignored "-Wfloat-equal"
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wdeprecated-register"
#pragma GCC diagnostic ignored "-Wshorten-64-to-32"
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#include <boost/geometry/geometries/register/linestring.hpp>
#include <boost/geometry/geometries/register/point.hpp>
#include <boost/geometry/geometries/register/ring.hpp>
#include <boost/geometry.hpp>
#pragma GCC diagnostic pop

#include <boost/range/iterator_range_core.hpp>

#include <cassert>

BOOST_GEOMETRY_REGISTER_POINT_2D (mapbox::geometry::point<double>, double, boost::geometry::cs::cartesian, x, y)
BOOST_GEOMETRY_REGISTER_POINT_2D (mapbox::geometry::point<std::int64_t>, std::int64_t, boost::geometry::cs::cartesian, x, y)

namespace boost {

template <typename CoordinateType>
struct range_iterator<mapbox::geometry::multi_point<CoordinateType> >
{
    using type = typename mapbox::geometry::multi_point<CoordinateType>::iterator;
};

template <typename CoordinateType>
struct range_const_iterator<mapbox::geometry::multi_point<CoordinateType> >
{
    using type = typename mapbox::geometry::multi_point<CoordinateType>::const_iterator;
};

template <typename CoordinateType>
inline typename mapbox::geometry::multi_point<CoordinateType>::iterator
range_begin(mapbox::geometry::multi_point<CoordinateType> & mp) {return mp.begin();}

template <typename CoordinateType>
inline typename mapbox::geometry::multi_point<CoordinateType>::iterator
range_end(mapbox::geometry::multi_point<CoordinateType> & mp) {return mp.end();}

template <typename CoordinateType>
inline typename mapbox::geometry::multi_point<CoordinateType>::const_iterator
range_begin(mapbox::geometry::multi_point<CoordinateType> const& mp) {return mp.begin();}

template <typename CoordinateType>
inline typename mapbox::geometry::multi_point<CoordinateType>::const_iterator
range_end(mapbox::geometry::multi_point<CoordinateType> const& mp) {return mp.end();}

template <typename CoordinateType>
struct range_iterator<mapbox::geometry::line_string<CoordinateType> >
{
    using type = typename mapbox::geometry::line_string<CoordinateType>::iterator;
};

template <typename CoordinateType>
struct range_const_iterator<mapbox::geometry::line_string<CoordinateType> >
{
    using type = typename mapbox::geometry::line_string<CoordinateType>::const_iterator;
};

template <typename CoordinateType>
inline typename mapbox::geometry::line_string<CoordinateType>::iterator
range_begin(mapbox::geometry::line_string<CoordinateType> & line) {return line.begin();}

template <typename CoordinateType>
inline typename mapbox::geometry::line_string<CoordinateType>::iterator
range_end(mapbox::geometry::line_string<CoordinateType> & line) {return line.end();}

template <typename CoordinateType>
inline typename mapbox::geometry::line_string<CoordinateType>::const_iterator
range_begin(mapbox::geometry::line_string<CoordinateType> const& line) {return line.begin();}

template <typename CoordinateType>
inline typename mapbox::geometry::line_string<CoordinateType>::const_iterator
range_end(mapbox::geometry::line_string<CoordinateType> const& line) {return line.end();}

namespace geometry {
namespace traits {

template <typename CoordinateType>
struct tag<mapbox::geometry::box<CoordinateType>> {
    using type = box_tag;
};

template <typename CoordinateType>
struct indexed_access<mapbox::geometry::box<CoordinateType>, min_corner, 0>
{
    static inline CoordinateType get(mapbox::geometry::box<CoordinateType> const& b) { return b.min.x;}
    static inline void set(mapbox::geometry::box<CoordinateType> &b, CoordinateType const& value) { b.min.x = value; }
};

template <typename CoordinateType>
struct indexed_access<mapbox::geometry::box<CoordinateType>, min_corner, 1>
{
    static inline CoordinateType get(mapbox::geometry::box<CoordinateType> const& b) { return b.min.y;}
    static inline void set(mapbox::geometry::box<CoordinateType> &b, CoordinateType const& value) { b.min.y = value; }
};

template <typename CoordinateType>
struct indexed_access<mapbox::geometry::box<CoordinateType>, max_corner, 0>
{
    static inline CoordinateType get(mapbox::geometry::box<CoordinateType> const& b) { return b.max.x;}
    static inline void set(mapbox::geometry::box<CoordinateType> &b, CoordinateType const& value) { b.max.x = value; }
};

template <typename CoordinateType>
struct indexed_access<mapbox::geometry::box<CoordinateType>, max_corner, 1>
{
    static inline CoordinateType get(mapbox::geometry::box<CoordinateType> const& b) { return b.max.y;}
    static inline void set(mapbox::geometry::box<CoordinateType> &b, CoordinateType const& value) { b.max.y = value; }
};

template <typename CoordinateType>
struct coordinate_type<mapbox::geometry::point<CoordinateType>> {
    using type = CoordinateType;
};

template <typename CoordinateType>
struct coordinate_system<mapbox::geometry::point<CoordinateType>> {
    using type = boost::geometry::cs::cartesian;
};

template <typename CoordinateType>
struct dimension<mapbox::geometry::point<CoordinateType>> : boost::mpl::int_<2> {};

template <typename CoordinateType>
struct access<mapbox::geometry::point<CoordinateType>, 0> {
    static CoordinateType get(mapbox::geometry::point<CoordinateType> const& p) {
        return p.x;
    }

    static void set(mapbox::geometry::point<CoordinateType>& p, CoordinateType x) {
        p.x = x;
    }
};

template <typename CoordinateType>
struct access<mapbox::geometry::point<CoordinateType>, 1> {
    static CoordinateType get(mapbox::geometry::point<CoordinateType> const& p) {
        return p.y;
    }

    static void set(mapbox::geometry::point<CoordinateType>& p, CoordinateType y) {
        p.y = y;
    }
};

template <typename CoordinateType>
struct tag<mapbox::geometry::multi_point<CoordinateType>> {
    using type = multi_point_tag;
};

template <typename CoordinateType>
struct tag<mapbox::geometry::line_string<CoordinateType>> {
    using type = linestring_tag;
};

template <typename CoordinateType>
struct tag<mapbox::geometry::multi_line_string<CoordinateType>> {
    using type = multi_linestring_tag;
};

template <typename CoordinateType>
struct tag<mapbox::geometry::linear_ring<CoordinateType>> {
    using type = ring_tag;
};

template <typename CoordinateType>
struct point_order<mapbox::geometry::linear_ring<CoordinateType> >
{
    static const order_selector value = counterclockwise;
};

template <typename CoordinateType>
struct tag<mapbox::geometry::polygon<CoordinateType>> {
    using type = polygon_tag;
};

template <typename CoordinateType>
struct ring_mutable_type<mapbox::geometry::multi_point<CoordinateType>> {
    using type = mapbox::geometry::point<CoordinateType>&;
};

template <typename CoordinateType>
struct ring_const_type<mapbox::geometry::multi_point<CoordinateType>> {
    using type = mapbox::geometry::point<CoordinateType> const&;
};

template <typename CoordinateType>
struct ring_mutable_type<mapbox::geometry::polygon<CoordinateType>> {
    using type = mapbox::geometry::linear_ring<CoordinateType>&;
};

template <typename CoordinateType>
struct ring_const_type<mapbox::geometry::polygon<CoordinateType>> {
    using type = mapbox::geometry::linear_ring<CoordinateType> const&;
};

template <typename CoordinateType>
struct interior_mutable_type<mapbox::geometry::polygon<CoordinateType>> {
    using type =
        boost::iterator_range<typename mapbox::geometry::polygon<CoordinateType>::iterator>;
};

template <typename CoordinateType>
struct interior_const_type<mapbox::geometry::polygon<CoordinateType>> {
    using type =
        boost::iterator_range<typename mapbox::geometry::polygon<CoordinateType>::const_iterator>;
};

template <typename CoordinateType>
struct exterior_ring<mapbox::geometry::polygon<CoordinateType>> {
    static mapbox::geometry::linear_ring<CoordinateType>&
    get(mapbox::geometry::polygon<CoordinateType>& p) {
        return p.at(0);
    }

    static mapbox::geometry::linear_ring<CoordinateType> const&
    get(mapbox::geometry::polygon<CoordinateType> const& p) {
        return p.at(0);
    }
};

template <typename CoordinateType>
struct interior_rings<mapbox::geometry::polygon<CoordinateType>> {
    static boost::iterator_range<typename mapbox::geometry::polygon<CoordinateType>::iterator>
    get(mapbox::geometry::polygon<CoordinateType>& p) {
        return boost::make_iterator_range(p.begin() + 1, p.end());
    }

    static boost::iterator_range<typename mapbox::geometry::polygon<CoordinateType>::const_iterator>
    get(mapbox::geometry::polygon<CoordinateType> const& p) {
        return boost::make_iterator_range(p.begin() + 1, p.end());
    }
};

template <typename CoordinateType>
struct tag<mapbox::geometry::multi_polygon<CoordinateType>> {
    using type = multi_polygon_tag;
};
}
}
}

