#include <mapnik/vertex.hpp>
#include <mapnik/geometry.hpp>
#include <mapnik/geometry_adapters.hpp>
#include <mapnik/vertex_processor.hpp>
#include "vector_tile_geometry_decoder.hpp"
#include "vector_tile_geometry_encoder.hpp"
#include <mapnik/geometry_empty.hpp>

namespace {

using namespace mapnik::geometry;

struct print
{
    void operator() (geometry_empty const&) const
    {
        std::cerr << "EMPTY" << std::endl;
    }
    void operator() (geometry_collection const& collection) const
    {
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
    unsigned tolerance_;
    unsigned path_multiplier_;
    int32_t x_;
    int32_t y_;
    encode_geometry(vector_tile::Tile_Feature & feature,
             unsigned tolerance,
             unsigned path_multiplier) :
      feature_(feature),
      tolerance_(tolerance),
      path_multiplier_(path_multiplier),
      x_(0),
      y_(0) { }

    template <typename T>
    void operator()(T & path)
    {
        mapnik::vector_tile_impl::encode_geometry(path,feature_,x_,y_,tolerance_,path_multiplier_);
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

vector_tile::Tile_Feature geometry_to_feature(mapnik::geometry::geometry const& g,
                                              unsigned tolerance=0,
                                              unsigned path_multiplier=1)
{
    vector_tile::Tile_Feature feature;
    encode_geometry ap(feature,tolerance,path_multiplier);
    if (g.is<mapnik::geometry::point>() || g.is<mapnik::geometry::multi_point>())
    {
        feature.set_type(vector_tile::Tile_GeomType_POINT);
    }
    else if (g.is<mapnik::geometry::line_string>() || g.is<mapnik::geometry::multi_line_string>())
    {
        feature.set_type(vector_tile::Tile_GeomType_LINESTRING);
    }
    else if (g.is<mapnik::geometry::polygon>() || g.is<mapnik::geometry::multi_polygon>())
    {
        feature.set_type(vector_tile::Tile_GeomType_POLYGON);
    }
    else
    {
        throw std::runtime_error("could not detect valid geometry type");
    }
    mapnik::util::apply_visitor(mapnik::geometry::vertex_processor<encode_geometry>(ap),g);
    return feature;
}

std::string decode_to_path_string(mapnik::geometry::geometry const& g)
{
    //mapnik::util::apply_visitor(print(), g2);
    using decode_path_type = mapnik::geometry::vertex_processor<show_path>;
    std::string out;
    show_path sp(out);
    mapnik::util::apply_visitor(decode_path_type(sp), g);
    return out;
}

std::string compare(mapnik::geometry::geometry const& g,
                    unsigned tolerance=0,
                    unsigned path_multiplier=1)
{
    vector_tile::Tile_Feature feature = geometry_to_feature(g,tolerance,path_multiplier);
    auto g2 = mapnik::vector_tile_impl::decode_geometry(feature,0.0,0.0,1.0,1.0);
    return decode_to_path_string(g2);
}
