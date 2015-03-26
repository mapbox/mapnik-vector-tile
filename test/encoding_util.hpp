#include <mapnik/vertex.hpp>
#include <mapnik/geometry.hpp>
#include <mapnik/vertex_processor.hpp>
#include "vector_tile_geometry_decoder.hpp"
#include "vector_tile_geometry_encoder.hpp"

void decode_geometry(vector_tile::Tile_Feature const& f,
                     mapnik::geometry::geometry & geom,
                     double x,
                     double y,
                     double scale)
{
    geom = mapnik::vector_tile_impl::decode_geometry(f,x,y,scale);
}

struct add_path
{
    vector_tile::Tile_Feature feature_;
    unsigned tolerance_;
    unsigned path_multiplier_;
    int32_t x_;
    int32_t y_;
    add_path(unsigned tolerance,
             unsigned path_multiplier) :
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
    std::string str_;
    show_path() {}

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
        str_ = s.str();
    }
};

std::string compare(mapnik::geometry::geometry const& g,
                    unsigned tolerance=0,
                    unsigned path_multiplier=1)
{
    using namespace mapnik::vector_tile_impl;
    // encode
    using add_path_type = mapnik::geometry::vertex_processor<add_path>;
    add_path ap(tolerance,path_multiplier);
    if (g.is<mapnik::geometry::point>() || g.is<mapnik::geometry::multi_point>())
    {
        ap.feature_.set_type(vector_tile::Tile_GeomType_POINT);
    }
    else if (g.is<mapnik::geometry::line_string>() || g.is<mapnik::geometry::multi_line_string>())
    {
        ap.feature_.set_type(vector_tile::Tile_GeomType_LINESTRING);
    }
    else if (g.is<mapnik::geometry::polygon>() || g.is<mapnik::geometry::multi_polygon>())
    {
        ap.feature_.set_type(vector_tile::Tile_GeomType_POLYGON);
    }
    mapnik::util::apply_visitor(add_path_type(ap),g);
    // decode
    mapnik::geometry::geometry g2;
    double x0 = 0;
    double y0 = 0;
    decode_geometry(ap.feature_,g2,x0,y0,path_multiplier);
    using decode_path_type = mapnik::geometry::vertex_processor<show_path>;
    show_path sp;
    mapnik::util::apply_visitor(decode_path_type(sp),g2);
    return sp.str_;

}
