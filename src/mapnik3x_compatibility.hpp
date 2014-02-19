#ifndef __MAPNIK_VECTOR_TILE_COMPATIBILITY_H__
#define __MAPNIK_VECTOR_TILE_COMPATIBILITY_H__

#include <mapnik/version.hpp>

#if MAPNIK_VERSION >= 300000
    #define MAPNIK_UNIQUE_PTR std::unique_ptr
    #define MAPNIK_SHARED_INCLUDE <memory>
    #define MAPNIK_MAKE_SHARED_INCLUDE <memory>
    #define MAPNIK_MAKE_SHARED std::make_shared
    #define MAPNIK_SHARED_PTR std::shared_ptr
    #define MAPNIK_ADD_LAYER add_layer
    #define MAPNIK_GET std::get
    #define MAPNIK_GEOM_TYPE mapnik::geometry_type::types
    #define MAPNIK_POINT mapnik::geometry_type::types::Point
    #define MAPNIK_POLYGON mapnik::geometry_type::types::Polygon
    #define MAPNIK_LINESTRING mapnik::geometry_type::types::LineString
    #define MAPNIK_UNKNOWN mapnik::geometry_type::types::Unknown
#else
    #define MAPNIK_UNIQUE_PTR std::auto_ptr
    #define MAPNIK_SHARED_INCLUDE <boost/shared_ptr.hpp>
    #define MAPNIK_MAKE_SHARED_INCLUDE <boost/make_shared.hpp>
    #define MAPNIK_MAKE_SHARED boost::make_shared
    #define MAPNIK_SHARED_PTR boost::shared_ptr
    #define MAPNIK_ADD_LAYER addLayer
    #define MAPNIK_GET boost::get
    #define MAPNIK_GEOM_TYPE mapnik::eGeomType
    #define MAPNIK_POINT mapnik::Point
    #define MAPNIK_POLYGON mapnik::Polygon
    #define MAPNIK_LINESTRING mapnik::LineString
    #define MAPNIK_UNKNOWN mapnik::Unknown
#endif

#endif
