#ifndef MAPNIK_VECTOR_TILE_STRATEGY_HPP
#define MAPNIK_VECTOR_TILE_STRATEGY_HPP

// mapnik
#include <mapnik/config.hpp>
#include <mapnik/util/noncopyable.hpp>
#include <mapnik/proj_transform.hpp>
#include <mapnik/view_transform.hpp>
#include <mapnik/geometry_envelope.hpp>

#include "clipper.hpp"

#pragma GCC diagnostic push
#include <mapnik/warning_ignore.hpp>
#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/geometry/core/access.hpp>
#pragma GCC diagnostic pop

namespace mapnik {

namespace vector_tile_impl {

static constexpr double coord_max = static_cast<double>(ClipperLib::hiRange);
static constexpr double coord_min = -1 * static_cast<double>(ClipperLib::hiRange);

struct vector_tile_strategy
{
    vector_tile_strategy(view_transform const& tr)
        : tr_(tr) {}

    template <typename P1, typename P2>
    inline bool apply(P1 const& p1, P2 & p2) const
    {
        using p2_type = typename boost::geometry::coordinate_type<P2>::type;
        double x = boost::geometry::get<0>(p1);
        double y = boost::geometry::get<1>(p1);
        tr_.forward(&x,&y);
        x = std::round(x);
        y = std::round(y);
        if (x <= coord_min || x >= coord_max ||
            y <= coord_min || y >= coord_max) return false;
        boost::geometry::set<0>(p2, static_cast<p2_type>(x));
        boost::geometry::set<1>(p2, static_cast<p2_type>(y));
        return true;
    }

    template <typename P1, typename P2>
    inline P2 execute(P1 const& p1, bool & status) const
    {
        P2 p2;
        status = apply(p1, p2);
        return p2;
    }

    view_transform const& tr_;
};

struct vector_tile_strategy_proj
{
    vector_tile_strategy_proj(proj_transform const& prj_trans,
                              view_transform const& tr)
        : prj_trans_(prj_trans),
          tr_(tr) {}

    template <typename P1, typename P2>
    inline bool apply(P1 const& p1, P2 & p2) const
    {
        using p2_type = typename boost::geometry::coordinate_type<P2>::type;
        double x = boost::geometry::get<0>(p1);
        double y = boost::geometry::get<1>(p1);
        double z = 0.0;
        if (!prj_trans_.backward(x, y, z)) return false;
        tr_.forward(&x,&y);
        x = std::round(x);
        y = std::round(y);
        if (x <= coord_min || x >= coord_max ||
            y <= coord_min || y >= coord_max) return false;
        boost::geometry::set<0>(p2, static_cast<p2_type>(x));
        boost::geometry::set<1>(p2, static_cast<p2_type>(y));
        return true;
    }
    
    template <typename P1, typename P2>
    inline P2 execute(P1 const& p1, bool & status) const
    {
        P2 p2;
        status = apply(p1, p2);
        return p2;
    }

    proj_transform const& prj_trans_;
    view_transform const& tr_;
};

template <typename T>
struct geom_out_visitor
{
    mapnik::geometry::geometry<T> geom;

    template <typename T1>
    void operator() (T1 const& g)
    {
        geom = mapnik::geometry::geometry<T>(g);
    }
};

// TODO - avoid creating degenerate polygons when first/last point of ring is skipped
template <typename TransformType, typename NextProcessor>
struct transform_visitor
{
    TransformType const& tr_;
    NextProcessor & next_;
    box2d<double> const& target_clipping_extent_;

    transform_visitor(TransformType const& tr, 
                      box2d<double> const& target_clipping_extent,
                      NextProcessor & next) :
      tr_(tr),
      next_(next),
      target_clipping_extent_(target_clipping_extent) {}

    inline void operator() (mapnik::geometry::point<double> const& geom)
    {
        if (!target_clipping_extent_.intersects(geom.x,geom.y))
        {
            return;
        }
        mapnik::geometry::point<std::int64_t> new_geom;
        if (!tr_.apply(geom,new_geom))
        {
            return;
        }
        return next_(new_geom);
    }

    inline void operator() (mapnik::geometry::multi_point<double> const& geom)
    {
        mapnik::geometry::multi_point<std::int64_t> new_geom;
        new_geom.reserve(geom.size());
        for (auto const& pt : geom)
        {
            mapnik::geometry::point<std::int64_t> pt2;
            if (target_clipping_extent_.intersects(pt.x, pt.y) && tr_.apply(pt,pt2))
            {
                new_geom.push_back(std::move(pt2));
            }
        }
        if (new_geom.empty())
        {
            return;
        }
        return next_(new_geom);
    }

    inline void operator() (mapnik::geometry::line_string<double> const& geom)
    {
        mapnik::box2d<double> geom_bbox = mapnik::geometry::envelope(geom);
        if (!target_clipping_extent_.intersects(geom_bbox)) 
        {
            return;
        }
        mapnik::geometry::line_string<std::int64_t> new_geom;
        new_geom.reserve(geom.size());
        for (auto const& pt : geom)
        {
            mapnik::geometry::point<std::int64_t> pt2;
            if (tr_.apply(pt,pt2))
            {
                new_geom.push_back(std::move(pt2));
            }
        }
        return next_(new_geom);
    }

    inline void operator() (mapnik::geometry::multi_line_string<double> const& geom)
    {
        mapnik::geometry::multi_line_string<std::int64_t> new_geom;
        new_geom.reserve(geom.size());
        for (auto const& line : geom)
        {
            mapnik::box2d<double> line_bbox = mapnik::geometry::envelope(line);
            if (!target_clipping_extent_.intersects(line_bbox)) continue;
            mapnik::geometry::line_string<std::int64_t> new_line;
            new_line.reserve(line.size());
            for (auto const& pt : line)
            {
                mapnik::geometry::point<std::int64_t> pt2;
                if (tr_.apply(pt,pt2))
                {
                    new_line.push_back(std::move(pt2));
                }
            }
            new_geom.push_back(std::move(new_line));
        }
        if (new_geom.empty())
        {
            return;
        }
        return next_(new_geom);
    }

    inline void operator() (mapnik::geometry::polygon<double> const& geom)
    {
        mapnik::box2d<double> ext_bbox = mapnik::geometry::envelope(geom);
        if (!target_clipping_extent_.intersects(ext_bbox))
        {
            return;
        }
        mapnik::geometry::polygon<std::int64_t> new_geom;
        new_geom.exterior_ring.reserve(geom.exterior_ring.size());
        for (auto const& pt : geom.exterior_ring)
        {
            mapnik::geometry::point<std::int64_t> pt2;
            if (tr_.apply(pt,pt2))
            {
                new_geom.exterior_ring.push_back(std::move(pt2));
            }
        }
        for (auto const& ring : geom.interior_rings)
        {
            mapnik::box2d<double> ring_bbox = mapnik::geometry::envelope(static_cast<mapnik::geometry::line_string<double> const&>(ring));
            if (!target_clipping_extent_.intersects(ring_bbox)) 
            {
                continue;
            }
            mapnik::geometry::linear_ring<std::int64_t> new_ring;
            new_ring.reserve(ring.size());
            for (auto const& pt : ring)
            {
                mapnik::geometry::point<std::int64_t> pt2;
                if (tr_.apply(pt,pt2))
                {
                    new_ring.push_back(std::move(pt2));
                }
            }
            new_geom.interior_rings.push_back(std::move(new_ring));
        }
        return next_(new_geom);
    }

    inline void operator() (mapnik::geometry::multi_polygon<double> const& geom)
    {
        mapnik::geometry::multi_polygon<std::int64_t> new_geom;
        new_geom.reserve(geom.size());
        for (auto const& poly : geom)
        {
            mapnik::box2d<double> ext_bbox = mapnik::geometry::envelope(poly);
            if (!target_clipping_extent_.intersects(ext_bbox)) 
            {
                continue;
            }
            mapnik::geometry::polygon<std::int64_t> new_poly;
            new_poly.exterior_ring.reserve(poly.exterior_ring.size());
            for (auto const& pt : poly.exterior_ring)
            {
                mapnik::geometry::point<std::int64_t> pt2;
                if (tr_.apply(pt,pt2))
                {
                    new_poly.exterior_ring.push_back(std::move(pt2));
                }
            }
            for (auto const& ring : poly.interior_rings)
            {
                mapnik::box2d<double> ring_bbox = mapnik::geometry::envelope(static_cast<mapnik::geometry::line_string<double> const&>(ring));
                if (!target_clipping_extent_.intersects(ring_bbox))
                {
                    continue;
                }
                mapnik::geometry::linear_ring<std::int64_t> new_ring;
                new_ring.reserve(ring.size());
                for (auto const& pt : ring)
                {
                    mapnik::geometry::point<std::int64_t> pt2;
                    if (tr_.apply(pt,pt2))
                    {
                        new_ring.push_back(std::move(pt2));
                    }
                }
                new_poly.interior_rings.push_back(std::move(new_ring));
            }
            new_geom.push_back(std::move(new_poly));
        }
        if (new_geom.empty())
        {
            return;
        }
        return next_(new_geom);
    }

    inline void operator() (mapnik::geometry::geometry_collection<double> const& geom)
    {
        for (auto const& g : geom)
        {
            mapnik::util::apply_visitor((*this), g);
        }
     }

    inline void operator() (mapnik::geometry::geometry_empty const&)
    {
        return;
    }
};

}
}

#endif // MAPNIK_VECTOR_TILE_STRATEGY_HPP
