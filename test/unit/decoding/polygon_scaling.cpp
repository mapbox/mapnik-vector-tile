#include "catch.hpp"

// mapnik vector tile
#include "vector_tile_geometry_decoder.hpp"

// test utils
#include "decoding_util.hpp"

// mapnik
#include <mapnik/geometry.hpp>
#include <mapnik/util/geometry_to_wkt.hpp>

// libprotobuf
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include "vector_tile.pb.h"
#pragma GCC diagnostic pop

//
// Unit tests for geometry decoding of polygons with scaling
//

TEST_CASE("decode simple polygon - scale 2")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POLYGON);
    // MoveTo(0,0)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(0,10)
    feature.add_geometry((3 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(10));
    // LineTo(-10,10)
    feature.add_geometry(protozero::encode_zigzag32(-10));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(-10,0)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(-10));
    // Close
    feature.add_geometry(15);

    SECTION("libprotobuf decoder")
    {
        mapnik::vector_tile_impl::Geometry<double> geoms(feature,0.0,0.0,2.0,2.0);
    
        SECTION("VT Spec v1")
        {
            auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(),1);
            std::string wkt0;
            CHECK( mapnik::util::to_wkt(wkt0,geom) );
            CHECK( wkt0 == "POLYGON((0 0,0 5,-5 5,-5 0,0 0))");
            CHECK( geom.is<mapnik::geometry::polygon<double> >() );
        }

        SECTION("VT Spec v2")
        {
            auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(),2);
            std::string wkt0;
            CHECK( mapnik::util::to_wkt(wkt0,geom) );
            CHECK( wkt0 == "POLYGON((0 0,0 5,-5 5,-5 0,0 0))");
            CHECK( geom.is<mapnik::geometry::polygon<double> >() );
        }
    }

    SECTION("protozero decoder")
    {
        std::string feature_string = feature.SerializeAsString();
        mapnik::vector_tile_impl::GeometryPBF<double> geoms = feature_to_pbf_geometry<double>(feature_string, 2.0, 2.0);
    
        SECTION("VT Spec v1")
        {
            auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(),1);
            std::string wkt0;
            CHECK( mapnik::util::to_wkt(wkt0,geom) );
            CHECK( wkt0 == "POLYGON((0 0,0 5,-5 5,-5 0,0 0))");
            CHECK( geom.is<mapnik::geometry::polygon<double> >() );
        }

        SECTION("VT Spec v2")
        {
            auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(),2);
            std::string wkt0;
            CHECK( mapnik::util::to_wkt(wkt0,geom) );
            CHECK( wkt0 == "POLYGON((0 0,0 5,-5 5,-5 0,0 0))");
            CHECK( geom.is<mapnik::geometry::polygon<double> >() );
        }
    }
}

TEST_CASE("decode simple polygon - scale 2 - int64 decode")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POLYGON);
    // MoveTo(0,0)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(0,10)
    feature.add_geometry((3 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(10));
    // LineTo(-10,10)
    feature.add_geometry(protozero::encode_zigzag32(-10));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(-10,0)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(-10));
    // Close
    feature.add_geometry(15);

    SECTION("libprotobuf decoder")
    {
        mapnik::vector_tile_impl::Geometry<std::int64_t> geoms(feature,0.0,0.0,2.0,2.0);
    
        SECTION("VT Spec v1")
        {
            auto geom = mapnik::vector_tile_impl::decode_geometry<std::int64_t>(geoms, feature.type(),1);
            std::string wkt0;
            CHECK( mapnik::util::to_wkt(wkt0,geom) );
            CHECK( wkt0 == "POLYGON((0 0,0 5,-5 5,-5 0,0 0))");
            CHECK( geom.is<mapnik::geometry::polygon<std::int64_t> >() );
        }

        SECTION("VT Spec v2")
        {
            auto geom = mapnik::vector_tile_impl::decode_geometry<std::int64_t>(geoms, feature.type(),2);
            std::string wkt0;
            CHECK( mapnik::util::to_wkt(wkt0,geom) );
            CHECK( wkt0 == "POLYGON((0 0,0 5,-5 5,-5 0,0 0))");
            CHECK( geom.is<mapnik::geometry::polygon<std::int64_t> >() );
        }
    }

    SECTION("protozero decoder")
    {
        std::string feature_string = feature.SerializeAsString();
        mapnik::vector_tile_impl::GeometryPBF<std::int64_t> geoms = feature_to_pbf_geometry<std::int64_t>(feature_string, 2.0, 2.0);
    
        SECTION("VT Spec v1")
        {
            auto geom = mapnik::vector_tile_impl::decode_geometry<std::int64_t>(geoms, feature.type(),1);
            std::string wkt0;
            CHECK( mapnik::util::to_wkt(wkt0,geom) );
            CHECK( wkt0 == "POLYGON((0 0,0 5,-5 5,-5 0,0 0))");
            CHECK( geom.is<mapnik::geometry::polygon<std::int64_t> >() );
        }

        SECTION("VT Spec v2")
        {
            auto geom = mapnik::vector_tile_impl::decode_geometry<std::int64_t>(geoms, feature.type(),2);
            std::string wkt0;
            CHECK( mapnik::util::to_wkt(wkt0,geom) );
            CHECK( wkt0 == "POLYGON((0 0,0 5,-5 5,-5 0,0 0))");
            CHECK( geom.is<mapnik::geometry::polygon<std::int64_t> >() );
        }
    }
}

TEST_CASE("decode simple polygon - scale 3.214")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POLYGON);
    // MoveTo(0,0)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(0,10)
    feature.add_geometry((3 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(10));
    // LineTo(-10,10)
    feature.add_geometry(protozero::encode_zigzag32(-10));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(-10,0)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(-10));
    // Close
    feature.add_geometry(15);

    SECTION("libprotobuf decoder")
    {
        mapnik::vector_tile_impl::Geometry<double> geoms(feature,0.0,0.0,3.214,3.214);
    
        SECTION("VT Spec v1")
        {
            auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(),1);
            std::string wkt0;
            CHECK( mapnik::util::to_wkt(wkt0,geom) );
            CHECK( wkt0 == "POLYGON((0 0,0 3.11138767890479,-3.11138767890479 3.11138767890479,-3.11138767890479 0,0 0))");
            CHECK( geom.is<mapnik::geometry::polygon<double> >() );
        }

        SECTION("VT Spec v2")
        {
            auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(),2);
            std::string wkt0;
            CHECK( mapnik::util::to_wkt(wkt0,geom) );
            CHECK( wkt0 == "POLYGON((0 0,0 3.11138767890479,-3.11138767890479 3.11138767890479,-3.11138767890479 0,0 0))");
            CHECK( geom.is<mapnik::geometry::polygon<double> >() );
        }
    }

    SECTION("protozero decoder")
    {
        std::string feature_string = feature.SerializeAsString();
        mapnik::vector_tile_impl::GeometryPBF<double> geoms = feature_to_pbf_geometry<double>(feature_string, 3.214, 3.214);
    
        SECTION("VT Spec v1")
        {
            auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(),1);
            std::string wkt0;
            CHECK( mapnik::util::to_wkt(wkt0,geom) );
            CHECK( wkt0 == "POLYGON((0 0,0 3.11138767890479,-3.11138767890479 3.11138767890479,-3.11138767890479 0,0 0))");
            CHECK( geom.is<mapnik::geometry::polygon<double> >() );
        }

        SECTION("VT Spec v2")
        {
            auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(),2);
            std::string wkt0;
            CHECK( mapnik::util::to_wkt(wkt0,geom) );
            CHECK( wkt0 == "POLYGON((0 0,0 3.11138767890479,-3.11138767890479 3.11138767890479,-3.11138767890479 0,0 0))");
            CHECK( geom.is<mapnik::geometry::polygon<double> >() );
        }
    }
}

TEST_CASE("decode simple polygon - scale 3.214 - int64 decode")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POLYGON);
    // MoveTo(0,0)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(0,10)
    feature.add_geometry((3 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(10));
    // LineTo(-10,10)
    feature.add_geometry(protozero::encode_zigzag32(-10));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(-10,0)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(-10));
    // Close
    feature.add_geometry(15);

    SECTION("libprotobuf decoder")
    {
        mapnik::vector_tile_impl::Geometry<std::int64_t> geoms(feature,0.0,0.0,3.214,3.214);
    
        SECTION("VT Spec v1")
        {
            auto geom = mapnik::vector_tile_impl::decode_geometry<std::int64_t>(geoms, feature.type(),1);
            std::string wkt0;
            CHECK( mapnik::util::to_wkt(wkt0,geom) );
            CHECK( wkt0 == "POLYGON((0 0,0 3,-3 3,-3 0,0 0))");
            CHECK( geom.is<mapnik::geometry::polygon<std::int64_t> >() );
        }

        SECTION("VT Spec v2")
        {
            auto geom = mapnik::vector_tile_impl::decode_geometry<std::int64_t>(geoms, feature.type(),2);
            std::string wkt0;
            CHECK( mapnik::util::to_wkt(wkt0,geom) );
            CHECK( wkt0 == "POLYGON((0 0,0 3,-3 3,-3 0,0 0))");
            CHECK( geom.is<mapnik::geometry::polygon<std::int64_t> >() );
        }
    }

    SECTION("protozero decoder")
    {
        std::string feature_string = feature.SerializeAsString();
        mapnik::vector_tile_impl::GeometryPBF<std::int64_t> geoms = feature_to_pbf_geometry<std::int64_t>(feature_string, 3.214, 3.214);
    
        SECTION("VT Spec v1")
        {
            auto geom = mapnik::vector_tile_impl::decode_geometry<std::int64_t>(geoms, feature.type(),1);
            std::string wkt0;
            CHECK( mapnik::util::to_wkt(wkt0,geom) );
            CHECK( wkt0 == "POLYGON((0 0,0 3,-3 3,-3 0,0 0))");
            CHECK( geom.is<mapnik::geometry::polygon<std::int64_t> >() );
        }

        SECTION("VT Spec v2")
        {
            auto geom = mapnik::vector_tile_impl::decode_geometry<std::int64_t>(geoms, feature.type(),2);
            std::string wkt0;
            CHECK( mapnik::util::to_wkt(wkt0,geom) );
            CHECK( wkt0 == "POLYGON((0 0,0 3,-3 3,-3 0,0 0))");
            CHECK( geom.is<mapnik::geometry::polygon<std::int64_t> >() );
        }
    }
}

TEST_CASE("decode simple polygon - scale 0.46")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POLYGON);
    // MoveTo(0,0)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(0,10)
    feature.add_geometry((3 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(10));
    // LineTo(-10,10)
    feature.add_geometry(protozero::encode_zigzag32(-10));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(-10,0)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(-10));
    // Close
    feature.add_geometry(15);

    SECTION("libprotobuf decoder")
    {
        mapnik::vector_tile_impl::Geometry<double> geoms(feature,0.0,0.0,0.46,0.46);
    
        SECTION("VT Spec v1")
        {
            auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(),1);
            std::string wkt0;
            CHECK( mapnik::util::to_wkt(wkt0,geom) );
            CHECK( wkt0 == "POLYGON((0 0,0 21.7391304347826,-21.7391304347826 21.7391304347826,-21.7391304347826 0,0 0))");
            CHECK( geom.is<mapnik::geometry::polygon<double> >() );
        }

        SECTION("VT Spec v2")
        {
            auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(),2);
            std::string wkt0;
            CHECK( mapnik::util::to_wkt(wkt0,geom) );
            CHECK( wkt0 == "POLYGON((0 0,0 21.7391304347826,-21.7391304347826 21.7391304347826,-21.7391304347826 0,0 0))");
            CHECK( geom.is<mapnik::geometry::polygon<double> >() );
        }
    }

    SECTION("protozero decoder")
    {
        std::string feature_string = feature.SerializeAsString();
        mapnik::vector_tile_impl::GeometryPBF<double> geoms = feature_to_pbf_geometry<double>(feature_string, 0.46, 0.46);
    
        SECTION("VT Spec v1")
        {
            auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(),1);
            std::string wkt0;
            CHECK( mapnik::util::to_wkt(wkt0,geom) );
            CHECK( wkt0 == "POLYGON((0 0,0 21.7391304347826,-21.7391304347826 21.7391304347826,-21.7391304347826 0,0 0))");
            CHECK( geom.is<mapnik::geometry::polygon<double> >() );
        }

        SECTION("VT Spec v2")
        {
            auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(),2);
            std::string wkt0;
            CHECK( mapnik::util::to_wkt(wkt0,geom) );
            CHECK( wkt0 == "POLYGON((0 0,0 21.7391304347826,-21.7391304347826 21.7391304347826,-21.7391304347826 0,0 0))");
            CHECK( geom.is<mapnik::geometry::polygon<double> >() );
        }
    }
}

TEST_CASE("decode simple polygon - scale 0.46 - int64 decode")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POLYGON);
    // MoveTo(0,0)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(0,10)
    feature.add_geometry((3 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(10));
    // LineTo(-10,10)
    feature.add_geometry(protozero::encode_zigzag32(-10));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(-10,0)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(-10));
    // Close
    feature.add_geometry(15);

    SECTION("libprotobuf decoder")
    {
        mapnik::vector_tile_impl::Geometry<std::int64_t> geoms(feature,0.0,0.0,0.46,0.46);
    
        SECTION("VT Spec v1")
        {
            auto geom = mapnik::vector_tile_impl::decode_geometry<std::int64_t>(geoms, feature.type(),1);
            std::string wkt0;
            CHECK( mapnik::util::to_wkt(wkt0,geom) );
            CHECK( wkt0 == "POLYGON((0 0,0 22,-22 22,-22 0,0 0))");
            CHECK( geom.is<mapnik::geometry::polygon<std::int64_t> >() );
        }

        SECTION("VT Spec v2")
        {
            auto geom = mapnik::vector_tile_impl::decode_geometry<std::int64_t>(geoms, feature.type(),2);
            std::string wkt0;
            CHECK( mapnik::util::to_wkt(wkt0,geom) );
            CHECK( wkt0 == "POLYGON((0 0,0 22,-22 22,-22 0,0 0))");
            CHECK( geom.is<mapnik::geometry::polygon<std::int64_t> >() );
        }
    }

    SECTION("protozero decoder")
    {
        std::string feature_string = feature.SerializeAsString();
        mapnik::vector_tile_impl::GeometryPBF<std::int64_t> geoms = feature_to_pbf_geometry<std::int64_t>(feature_string, 0.46, 0.46);
    
        SECTION("VT Spec v1")
        {
            auto geom = mapnik::vector_tile_impl::decode_geometry<std::int64_t>(geoms, feature.type(),1);
            std::string wkt0;
            CHECK( mapnik::util::to_wkt(wkt0,geom) );
            CHECK( wkt0 == "POLYGON((0 0,0 22,-22 22,-22 0,0 0))");
            CHECK( geom.is<mapnik::geometry::polygon<std::int64_t> >() );
        }

        SECTION("VT Spec v2")
        {
            auto geom = mapnik::vector_tile_impl::decode_geometry<std::int64_t>(geoms, feature.type(),2);
            std::string wkt0;
            CHECK( mapnik::util::to_wkt(wkt0,geom) );
            CHECK( wkt0 == "POLYGON((0 0,0 22,-22 22,-22 0,0 0))");
            CHECK( geom.is<mapnik::geometry::polygon<std::int64_t> >() );
        }
    }
}

TEST_CASE("decode simple polygon - inverted y axis")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POLYGON);
    // MoveTo(0,0)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(0,10)
    feature.add_geometry((3 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(10));
    // LineTo(-10,10)
    feature.add_geometry(protozero::encode_zigzag32(-10));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(-10,0)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(-10));
    // Close
    feature.add_geometry(15);

    SECTION("libprotobuf decoder")
    {
        mapnik::vector_tile_impl::Geometry<double> geoms(feature,0.0,0.0,1.0,-1.0);
    
        SECTION("VT Spec v1")
        {
            auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(),1);
            std::string wkt0;
            CHECK( mapnik::util::to_wkt(wkt0,geom) );
            CHECK( wkt0 == "POLYGON((0 0,-10 0,-10 -10,0 -10,0 0))");
            CHECK( geom.is<mapnik::geometry::polygon<double> >() );
        }

        SECTION("VT Spec v2")
        {
            auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(),2);
            std::string wkt0;
            CHECK( mapnik::util::to_wkt(wkt0,geom) );
            CHECK( wkt0 == "POLYGON((0 0,-10 0,-10 -10,0 -10,0 0))");
            CHECK( geom.is<mapnik::geometry::polygon<double> >() );
        }
    }

    SECTION("protozero decoder")
    {
        std::string feature_string = feature.SerializeAsString();
        mapnik::vector_tile_impl::GeometryPBF<double> geoms = feature_to_pbf_geometry<double>(feature_string, 1.0, -1.0);
    
        SECTION("VT Spec v1")
        {
            auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(),1);
            std::string wkt0;
            CHECK( mapnik::util::to_wkt(wkt0,geom) );
            CHECK( wkt0 == "POLYGON((0 0,-10 0,-10 -10,0 -10,0 0))");
            CHECK( geom.is<mapnik::geometry::polygon<double> >() );
        }

        SECTION("VT Spec v2")
        {
            auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(),2);
            std::string wkt0;
            CHECK( mapnik::util::to_wkt(wkt0,geom) );
            CHECK( wkt0 == "POLYGON((0 0,-10 0,-10 -10,0 -10,0 0))");
            CHECK( geom.is<mapnik::geometry::polygon<double> >() );
        }
    }
}

TEST_CASE("decode simple polygon - inverted x axis")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POLYGON);
    // MoveTo(0,0)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(0,10)
    feature.add_geometry((3 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(10));
    // LineTo(-10,10)
    feature.add_geometry(protozero::encode_zigzag32(-10));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(-10,0)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(-10));
    // Close
    feature.add_geometry(15);

    SECTION("libprotobuf decoder")
    {
        mapnik::vector_tile_impl::Geometry<double> geoms(feature,0.0,0.0,-1.0,1.0);
    
        SECTION("VT Spec v1")
        {
            auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(),1);
            std::string wkt0;
            CHECK( mapnik::util::to_wkt(wkt0,geom) );
            CHECK( wkt0 == "POLYGON((0 0,10 0,10 10,0 10,0 0))");
            CHECK( geom.is<mapnik::geometry::polygon<double> >() );
        }

        SECTION("VT Spec v2")
        {
            auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(),2);
            std::string wkt0;
            CHECK( mapnik::util::to_wkt(wkt0,geom) );
            CHECK( wkt0 == "POLYGON((0 0,10 0,10 10,0 10,0 0))");
            CHECK( geom.is<mapnik::geometry::polygon<double> >() );
        }
    }

    SECTION("protozero decoder")
    {
        std::string feature_string = feature.SerializeAsString();
        mapnik::vector_tile_impl::GeometryPBF<double> geoms = feature_to_pbf_geometry<double>(feature_string, -1.0, 1.0);
    
        SECTION("VT Spec v1")
        {
            auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(),1);
            std::string wkt0;
            CHECK( mapnik::util::to_wkt(wkt0,geom) );
            CHECK( wkt0 == "POLYGON((0 0,10 0,10 10,0 10,0 0))");
            CHECK( geom.is<mapnik::geometry::polygon<double> >() );
        }

        SECTION("VT Spec v2")
        {
            auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(),2);
            std::string wkt0;
            CHECK( mapnik::util::to_wkt(wkt0,geom) );
            CHECK( wkt0 == "POLYGON((0 0,10 0,10 10,0 10,0 0))");
            CHECK( geom.is<mapnik::geometry::polygon<double> >() );
        }
    }
}

TEST_CASE("decode simple polygon - inverted x axis and y axis")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POLYGON);
    // MoveTo(0,0)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(0,10)
    feature.add_geometry((3 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(10));
    // LineTo(-10,10)
    feature.add_geometry(protozero::encode_zigzag32(-10));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(-10,0)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(-10));
    // Close
    feature.add_geometry(15);

    SECTION("libprotobuf decoder")
    {
        mapnik::vector_tile_impl::Geometry<double> geoms(feature,0.0,0.0,-1.0,-1.0);
    
        SECTION("VT Spec v1")
        {
            auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(),1);
            std::string wkt0;
            CHECK( mapnik::util::to_wkt(wkt0,geom) );
            CHECK( wkt0 == "POLYGON((0 0,0 -10,10 -10,10 0,0 0))");
            CHECK( geom.is<mapnik::geometry::polygon<double> >() );
        }

        SECTION("VT Spec v2")
        {
            auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(),2);
            std::string wkt0;
            CHECK( mapnik::util::to_wkt(wkt0,geom) );
            CHECK( wkt0 == "POLYGON((0 0,0 -10,10 -10,10 0,0 0))");
            CHECK( geom.is<mapnik::geometry::polygon<double> >() );
        }
    }

    SECTION("protozero decoder")
    {
        std::string feature_string = feature.SerializeAsString();
        mapnik::vector_tile_impl::GeometryPBF<double> geoms = feature_to_pbf_geometry<double>(feature_string, -1.0, -1.0);
    
        SECTION("VT Spec v1")
        {
            auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(),1);
            std::string wkt0;
            CHECK( mapnik::util::to_wkt(wkt0,geom) );
            CHECK( wkt0 == "POLYGON((0 0,0 -10,10 -10,10 0,0 0))");
            CHECK( geom.is<mapnik::geometry::polygon<double> >() );
        }

        SECTION("VT Spec v2")
        {
            auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(),2);
            std::string wkt0;
            CHECK( mapnik::util::to_wkt(wkt0,geom) );
            CHECK( wkt0 == "POLYGON((0 0,0 -10,10 -10,10 0,0 0))");
            CHECK( geom.is<mapnik::geometry::polygon<double> >() );
        }
    }
}

TEST_CASE("decode simple polygon with hole - invert y axis")
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POLYGON);
    // This ring is counter clockwise
    // MoveTo(0,0)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(0,10)
    feature.add_geometry((3 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(10));
    // LineTo(-10,10)
    feature.add_geometry(protozero::encode_zigzag32(-10));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(-10,0)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(-10));
    // Close
    feature.add_geometry(15);
    // This ring is clockwise!
    // Cursor is still at -10,0
    // MoveTo(-7,7)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(3));
    feature.add_geometry(protozero::encode_zigzag32(7));
    // LineTo(-3,7)
    feature.add_geometry((3 << 3u) | 2u);
    feature.add_geometry(protozero::encode_zigzag32(4));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // LineTo(-3,3)
    feature.add_geometry(protozero::encode_zigzag32(0));
    feature.add_geometry(protozero::encode_zigzag32(-4));
    // LineTo(-7,3)
    feature.add_geometry(protozero::encode_zigzag32(-4));
    feature.add_geometry(protozero::encode_zigzag32(0));
    // Close
    feature.add_geometry(15);

    SECTION("libprotobuf decoder")
    {
        mapnik::vector_tile_impl::Geometry<double> geoms(feature,0.0,0.0,1.0,-1.0);
    
        SECTION("VT Spec v1")
        {
            auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(),1);
            std::string wkt0;
            CHECK( mapnik::util::to_wkt(wkt0,geom) );
            CHECK( wkt0 == "POLYGON((0 0,-10 0,-10 -10,0 -10,0 0),(-7 -7,-7 -3,-3 -3,-3 -7,-7 -7))");
            CHECK( geom.is<mapnik::geometry::polygon<double> >() );
        }

        SECTION("VT Spec v2")
        {
            auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(),2);
            std::string wkt0;
            CHECK( mapnik::util::to_wkt(wkt0,geom) );
            CHECK( wkt0 == "POLYGON((0 0,-10 0,-10 -10,0 -10,0 0),(-7 -7,-7 -3,-3 -3,-3 -7,-7 -7))");
            CHECK( geom.is<mapnik::geometry::polygon<double> >() );
        }
    }

    SECTION("protozero decoder")
    {
        std::string feature_string = feature.SerializeAsString();
        mapnik::vector_tile_impl::GeometryPBF<double> geoms = feature_to_pbf_geometry<double>(feature_string, 1.0, -1.0);
    
        SECTION("VT Spec v1")
        {
            auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(),1);
            std::string wkt0;
            CHECK( mapnik::util::to_wkt(wkt0,geom) );
            CHECK( wkt0 == "POLYGON((0 0,-10 0,-10 -10,0 -10,0 0),(-7 -7,-7 -3,-3 -3,-3 -7,-7 -7))");
            CHECK( geom.is<mapnik::geometry::polygon<double> >() );
        }

        SECTION("VT Spec v2")
        {
            auto geom = mapnik::vector_tile_impl::decode_geometry<double>(geoms, feature.type(),2);
            std::string wkt0;
            CHECK( mapnik::util::to_wkt(wkt0,geom) );
            CHECK( wkt0 == "POLYGON((0 0,-10 0,-10 -10,0 -10,0 0),(-7 -7,-7 -3,-3 -3,-3 -7,-7 -7))");
            CHECK( geom.is<mapnik::geometry::polygon<double> >() );
        }
    }
}

