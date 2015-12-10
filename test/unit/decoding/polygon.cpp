#include "catch.hpp"

// mapnik vector tile
#include "vector_tile_geometry_decoder.hpp"

// test utils
#include "decoding_util.hpp"

// mapnik
#include <mapnik/geometry.hpp>
#include <mapnik/util/geometry_to_wkt.hpp>

// libprotobuf
#include "vector_tile.pb.h"

//
// Unit tests for geometry decoding of polygons
//

TEST_CASE( "decode polygon with only one degenerate exterior ring" )
{
    vector_tile::Tile_Feature feature;
    feature.set_type(vector_tile::Tile_GeomType_POLYGON);
    // MoveTo(1,1)
    feature.add_geometry(9); // move_to | (1 << 3)
    feature.add_geometry(protozero::encode_zigzag32(1));
    feature.add_geometry(protozero::encode_zigzag32(1));

    // since first ring is degenerate the whole polygon should be culled in
    // the encoder.
    SECTION("libprotobuf decoder")
    {
        mapnik::vector_tile_impl::Geometry<double> geoms(feature,0.0,0.0,1.0,1.0);
    
        SECTION("VT Spec v1")
        {
            CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POLYGON, 1));
        }

        SECTION("VT Spec v2")
        {
            CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POLYGON, 2));
        }
    }

    SECTION("protozero decoder")
    {
        std::string feature_string = feature.SerializeAsString();
        mapnik::vector_tile_impl::GeometryPBF<double> geoms = feature_to_pbf_geometry<double>(feature_string);
    
        SECTION("VT Spec v1")
        {
            CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POLYGON, 1));
        }
        
        SECTION("VT Spec v2")
        {
            CHECK_THROWS(mapnik::vector_tile_impl::decode_geometry<double>(geoms, vector_tile::Tile_GeomType_POLYGON, 2));
        }
    }
}

