#include "catch.hpp"

// mapnik vector tile
#include "vector_tile_geometry_decoder.hpp"
#include "vector_tile_geometry_encoder_pbf.hpp"

// mapnik
#include <mapnik/vertex.hpp>
#include <mapnik/geometry.hpp>
#include <mapnik/vertex_processor.hpp>

// test utils
#include "encoding_util.hpp"
#include "decoding_util.hpp"

#include <iostream>

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

template <typename T>
std::string decode_to_path_string(mapnik::geometry::geometry<T> const& g)
{
    using decode_path_type = mapnik::geometry::vertex_processor<show_path>;
    std::string out;
    show_path sp(out);
    mapnik::util::apply_visitor(decode_path_type(sp), g);
    return out;
}

std::string compare_pbf(mapnik::geometry::geometry<std::int64_t> const& g, unsigned version)
{
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::string feature_str;
    protozero::pbf_writer feature_writer(feature_str);
    REQUIRE(mapnik::vector_tile_impl::encode_geometry_pbf(g, feature_writer, x, y));
    protozero::pbf_reader feature_reader(feature_str);
    int32_t geometry_type = mapnik::vector_tile_impl::Geometry_Type::UNKNOWN; 
    mapnik::vector_tile_impl::GeometryPBF::pbf_itr geom_itr;
    while (feature_reader.next())
    {
        if (feature_reader.tag() == mapnik::vector_tile_impl::Feature_Encoding::GEOMETRY)
        {
            geom_itr = feature_reader.get_packed_uint32();
        }
        else if (feature_reader.tag() == mapnik::vector_tile_impl::Feature_Encoding::TYPE)
        {
            geometry_type = feature_reader.get_enum();
        }
        else
        {
            feature_reader.skip();
        }
    }
    mapnik::vector_tile_impl::GeometryPBF geoms(geom_itr);
    auto g2 = mapnik::vector_tile_impl::decode_geometry<double>(geoms, geometry_type, version, 0.0, 0.0, 1.0, 1.0);
    return decode_to_path_string(g2);
}
