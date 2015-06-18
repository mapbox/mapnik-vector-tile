#include <mapnik/vertex.hpp>
#include <mapnik/geometry.hpp>
#include <mapnik/geometry_adapters.hpp>
#include <mapnik/vertex_processor.hpp>
#include "vector_tile_geometry_decoder.hpp"
#include "vector_tile_geometry_encoder.hpp"

namespace {

using namespace mapnik::geometry;

struct print
{
    void operator() (geometry_empty const&) const
    {
        std::cerr << "EMPTY" << std::endl;
    }
    template <typename T>
    void operator() (geometry_collection<T> const&) const
    {
        std::cerr << "COLLECTION" << std::endl;
    }
    template <typename T>
    void operator() (T const& geom) const
    {
        std::cerr << boost::geometry::wkt(geom) << std::endl;
    }
};

}

struct encode_geometry
{
    vector_tile::Tile_Feature & feature_;
    int32_t x_;
    int32_t y_;
    encode_geometry(vector_tile::Tile_Feature & feature) :
      feature_(feature),
      x_(0),
      y_(0) { }

    void operator() (geometry_empty const&)
    {
    }
    
    template <typename T>
    void operator()(T const& path)
    {
        mapnik::vector_tile_impl::encode_geometry(path,feature_,x_,y_);
    }
    
    void operator()(mapnik::geometry::multi_point<std::int64_t> const & path)
    {
        for (auto const& pt : path)
        {
            mapnik::vector_tile_impl::encode_geometry(pt,feature_,x_,y_);
        }
    }

    void operator()(mapnik::geometry::multi_line_string<std::int64_t> const & path)
    {
        for (auto const& ls : path)
        {
            mapnik::vector_tile_impl::encode_geometry(ls,feature_,x_,y_);
        }
    }
    
    void operator()(mapnik::geometry::multi_polygon<std::int64_t> const& path)
    {
        for (auto const& p : path)
        {
            mapnik::vector_tile_impl::encode_geometry(p,feature_,x_,y_);
        }
    }
    
    void operator()(mapnik::geometry::geometry_collection<std::int64_t> const& path)
    {
        for (auto const& p : path)
        {
            mapnik::util::apply_visitor((*this), p);
        }
    }
};

struct show_path
{
    std::string & str_;
    show_path(std::string & out) :
      str_(out) {}

    template <typename T>
    void operator()(T & path)
    {
        unsigned cmd = -1;
        double x = 0;
        double y = 0;
        std::ostringstream s;
        path.rewind(0);
        while ((cmd = path.vertex(&x, &y)) != mapnik::SEG_END)
        {
            switch (cmd)
            {
                case mapnik::SEG_MOVETO: s << "move_to("; break;
                case mapnik::SEG_LINETO: s << "line_to("; break;
                case mapnik::SEG_CLOSE: s << "close_path("; break;
                default: std::clog << "unhandled cmd " << cmd << "\n"; break;
            }
            s << x << "," << y << ")\n";
        }
        str_ += s.str();
    }
};

template <typename T>
vector_tile::Tile_Feature geometry_to_feature(mapnik::geometry::geometry<T> const& g)
{
    vector_tile::Tile_Feature feature;
    encode_geometry ap(feature);
    if (g.template is<mapnik::geometry::point<T> >() || g.template is<mapnik::geometry::multi_point<T> >())
    {
        feature.set_type(vector_tile::Tile_GeomType_POINT);
    }
    else if (g.template is<mapnik::geometry::line_string<T> >() || g.template is<mapnik::geometry::multi_line_string<T> >())
    {
        feature.set_type(vector_tile::Tile_GeomType_LINESTRING);
    }
    else if (g.template is<mapnik::geometry::polygon<T> >() || g.template is<mapnik::geometry::multi_polygon<T> >())
    {
        feature.set_type(vector_tile::Tile_GeomType_POLYGON);
    }
    else
    {
        throw std::runtime_error("could not detect valid geometry type");
    }
    mapnik::util::apply_visitor(ap,g);
    return feature;
}

template <typename T>
std::string decode_to_path_string(mapnik::geometry::geometry<T> const& g)
{
    //mapnik::util::apply_visitor(print(), g2);
    using decode_path_type = mapnik::geometry::vertex_processor<show_path>;
    std::string out;
    show_path sp(out);
    mapnik::util::apply_visitor(decode_path_type(sp), g);
    return out;
}

template <typename T>
std::string compare(mapnik::geometry::geometry<T> const& g)
{
    vector_tile::Tile_Feature feature = geometry_to_feature(g);
    mapnik::vector_tile_impl::Geometry geoms(feature,0.0,0.0,1.0,1.0);
    auto g2 = mapnik::vector_tile_impl::decode_geometry(geoms,feature.type());
    return decode_to_path_string(g2);
}
