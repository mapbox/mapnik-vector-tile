#include "catch.hpp"

// mapnik vector tile
#include "vector_tile_datasource_pbf.hpp"
#include "vector_tile_projection.hpp"

// libprotobuf
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include "vector_tile.pb.h"
#pragma GCC diagnostic pop

// Boost
#include <boost/optional.hpp>

TEST_CASE( "cannot create datasource from layer pbf without name" )
{
    unsigned tile_size = 256;

    SECTION("VT Spec v1")
    {
        vector_tile::Tile_Layer layer;
        layer.set_version(1);

        std::string buffer;
        layer.SerializePartialToString(&buffer);
        protozero::pbf_reader pbf_layer(buffer);

        try
        {
            mapnik::vector_tile_impl::tile_datasource_pbf ds(pbf_layer,0,0,0,tile_size);
            FAIL( "expected exception" );
        }
        catch(std::exception const& ex)
        {
            CHECK(std::string(ex.what()) == "The required name field is missing in a vector tile layer.");
        }
    }

    SECTION("VT Spec v2")
    {
        vector_tile::Tile_Layer layer;
        layer.set_version(2);

        std::string buffer;
        layer.SerializePartialToString(&buffer);
        protozero::pbf_reader pbf_layer(buffer);

        try
        {
            mapnik::vector_tile_impl::tile_datasource_pbf ds(pbf_layer,0,0,0,tile_size);
            FAIL( "expected exception" );
        }
        catch(std::exception const& ex)
        {
            CHECK(std::string(ex.what()) == "The required name field is missing in a vector tile layer. Tile does not comply with Version 2 of the Mapbox Vector Tile Specification.");
        }
    }
}

TEST_CASE( "can create datasource from layer pbf with name but without extent" )
{
    unsigned tile_size = 256;

    SECTION("VT Spec v1")
    {
        vector_tile::Tile_Layer layer;
        layer.set_version(1);
        layer.set_name("test_name");

        std::string buffer;
        layer.SerializePartialToString(&buffer);
        protozero::pbf_reader pbf_layer(buffer);

        mapnik::vector_tile_impl::tile_datasource_pbf ds(pbf_layer,0,0,0,tile_size);
        
        CHECK(ds.get_name() == "test_name");
    }

    SECTION("VT Spec v2")
    {
        vector_tile::Tile_Layer layer;
        layer.set_version(2);
        layer.set_name("test_name");

        std::string buffer;
        layer.SerializePartialToString(&buffer);
        protozero::pbf_reader pbf_layer(buffer);

        try
        {
            mapnik::vector_tile_impl::tile_datasource_pbf ds(pbf_layer,0,0,0,tile_size);
            FAIL( "expected exception" );
        }
        catch(std::exception const& ex)
        {
            CHECK(std::string(ex.what()) == "The required extent field is missing in the layer test_name. Tile does not comply with Version 2 of the Mapbox Vector Tile Specification.");
        }
    }
}

TEST_CASE( "can create datasource from layer pbf with name and extent" )
{
    unsigned tile_size = 256;

    SECTION("VT Spec v1")
    {
        vector_tile::Tile_Layer layer;
        layer.set_version(1);
        layer.set_name("test_name");
        layer.set_extent(4096);

        std::string buffer;
        layer.SerializePartialToString(&buffer);
        protozero::pbf_reader pbf_layer(buffer);

        mapnik::vector_tile_impl::tile_datasource_pbf ds(pbf_layer,0,0,0,tile_size);
        
        CHECK(ds.get_name() == "test_name");
    }

    SECTION("VT Spec v2")
    {
        vector_tile::Tile_Layer layer;
        layer.set_version(2);
        layer.set_name("test_name");
        layer.set_extent(4096);

        std::string buffer;
        layer.SerializePartialToString(&buffer);
        protozero::pbf_reader pbf_layer(buffer);

        mapnik::vector_tile_impl::tile_datasource_pbf ds(pbf_layer,0,0,0,tile_size);
        
        CHECK(ds.get_name() == "test_name");
    }
}

TEST_CASE( "datasource of empty layer pbf returns a featureset pointer whose first feature is null" )
{
    unsigned tile_size = 256;

    SECTION("VT Spec v1")
    {
        vector_tile::Tile_Layer layer;
        layer.set_version(1);
        layer.set_name("test_name");
        layer.set_extent(4096);

        std::string buffer;
        layer.SerializePartialToString(&buffer);
        protozero::pbf_reader pbf_layer(buffer);

        mapnik::vector_tile_impl::tile_datasource_pbf ds(pbf_layer,0,0,0,tile_size);
        
        mapnik::query q(ds.get_tile_extent());
        mapnik::featureset_ptr features = ds.features(q);

        CHECK(features);

        mapnik::feature_ptr feature = features->next();
        CHECK(!feature);
    }

    SECTION("VT Spec v2")
    {
        vector_tile::Tile_Layer layer;
        layer.set_version(2);
        layer.set_name("test_name");
        layer.set_extent(4096);

        std::string buffer;
        layer.SerializePartialToString(&buffer);
        protozero::pbf_reader pbf_layer(buffer);

        mapnik::vector_tile_impl::tile_datasource_pbf ds(pbf_layer,0,0,0,tile_size);

        mapnik::query q(ds.get_tile_extent());
        mapnik::featureset_ptr features = ds.features(q);

        CHECK(features);

        mapnik::feature_ptr feature = features->next();
        CHECK(!feature);
    }
}
