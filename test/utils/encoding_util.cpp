// mapnik vector tile
#include "vector_tile_geometry_decoder.hpp"
#include "vector_tile_geometry_encoder.hpp"

// mapnik
#include <mapnik/vertex.hpp>
#include <mapnik/geometry.hpp>
#include <mapnik/geometry_adapters.hpp>
#include <mapnik/vertex_processor.hpp>

// test utils
#include "encoding_util.hpp"
#include "decoding_util.hpp"

using namespace mapnik::geometry;

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

struct encoder_visitor
{
    vector_tile::Tile_Feature & feature_;
    int32_t x_;
    int32_t y_;
    encoder_visitor(vector_tile::Tile_Feature & feature) :
      feature_(feature),
      x_(0),
      y_(0) { }

    void operator() (geometry_empty const&)
    {
    }

    void operator()(mapnik::geometry::geometry_collection<std::int64_t> const& path)
    {
        for (auto const& p : path)
        {
            mapnik::util::apply_visitor((*this), p);
        }
    }

    template <typename T>
    void operator()(T const& geom)
    {
        mapnik::vector_tile_impl::encode_geometry(geom,feature_,x_,y_);
    }
};

vector_tile::Tile_Feature geometry_to_feature(mapnik::geometry::geometry<std::int64_t> const& g)
{
    vector_tile::Tile_Feature feature;
    if (g.template is<mapnik::geometry::point<std::int64_t>>() || g.template is<mapnik::geometry::multi_point<std::int64_t>>())
    {
        feature.set_type(vector_tile::Tile_GeomType_POINT);
    }
    else if (g.template is<mapnik::geometry::line_string<std::int64_t>>() || g.template is<mapnik::geometry::multi_line_string<std::int64_t>>())
    {
        feature.set_type(vector_tile::Tile_GeomType_LINESTRING);
    }
    else if (g.template is<mapnik::geometry::polygon<std::int64_t>>() || g.template is<mapnik::geometry::multi_polygon<std::int64_t>>())
    {
        feature.set_type(vector_tile::Tile_GeomType_POLYGON);
    }
    else
    {
        throw std::runtime_error("could not detect valid geometry type");
    }
    encoder_visitor ap(feature);
    mapnik::util::apply_visitor(ap,g);
    return feature;
}

template <typename T>
std::string decode_to_path_string(mapnik::geometry::geometry<T> const& g)
{
    using decode_path_type = mapnik::geometry::vertex_processor<show_path>;
    std::string out;
    show_path sp(out);
    mapnik::util::apply_visitor(decode_path_type(sp), g);
    return out;
}

std::string compare(mapnik::geometry::geometry<std::int64_t> const& g, unsigned version)
{
    vector_tile::Tile_Feature feature = geometry_to_feature(g);
    mapnik::vector_tile_impl::Geometry<double> geoms(feature,0.0,0.0,1.0,1.0);
    auto g2 = mapnik::vector_tile_impl::decode_geometry(geoms,feature.type(),version);
    return decode_to_path_string(g2);
}

std::string compare_pbf(mapnik::geometry::geometry<std::int64_t> const& g, unsigned version)
{
    vector_tile::Tile_Feature feature = geometry_to_feature(g);
    std::string feature_string = feature.SerializeAsString();
    mapnik::vector_tile_impl::GeometryPBF<double> geoms = feature_to_pbf_geometry<double>(feature_string);
    auto g2 = mapnik::vector_tile_impl::decode_geometry(geoms,feature.type(),version);
    return decode_to_path_string(g2);
}
