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

unsigned tile_size = 256;

TEST_CASE( "cannot create datasource from layer pbf without name" )
{
    std::string buffer;
    vector_tile::Tile_Layer layer;

    SECTION("VT Spec v1")
    {
        layer.set_version(1);
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
        layer.set_version(2);
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
    std::string buffer;
    vector_tile::Tile_Layer layer;
    layer.set_name("test_name");

    SECTION("VT Spec v1")
    {
        layer.set_version(1);
        layer.SerializePartialToString(&buffer);
        protozero::pbf_reader pbf_layer(buffer);

        mapnik::vector_tile_impl::tile_datasource_pbf ds(pbf_layer,0,0,0,tile_size);

        CHECK(ds.get_name() == "test_name");
    }

    SECTION("VT Spec v2")
    {
        layer.set_version(2);
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
    std::string buffer;
    vector_tile::Tile_Layer layer;
    layer.set_name("test_name");
    layer.set_extent(4096);

    SECTION("VT Spec v1")
    {
        layer.set_version(1);
        layer.SerializePartialToString(&buffer);
        protozero::pbf_reader pbf_layer(buffer);

        mapnik::vector_tile_impl::tile_datasource_pbf ds(pbf_layer,0,0,0,tile_size);

        CHECK(ds.get_name() == "test_name");
    }

    SECTION("VT Spec v2")
    {
        layer.set_version(2);
        layer.SerializePartialToString(&buffer);
        protozero::pbf_reader pbf_layer(buffer);

        mapnik::vector_tile_impl::tile_datasource_pbf ds(pbf_layer,0,0,0,tile_size);

        CHECK(ds.get_name() == "test_name");
    }
}

TEST_CASE( "datasource of empty layer pbf returns a null featureset pointer" )
{
    // From the spec: A layer SHOULD contain at least one feature. Unknown behavior when
    // that is not the case. Current behavior is to return a null featureset.
    std::string buffer;
    vector_tile::Tile_Layer layer;
    layer.set_name("test_name");
    layer.set_extent(4096);

    SECTION("VT Spec v1")
    {
        layer.set_version(1);
        layer.SerializePartialToString(&buffer);
        protozero::pbf_reader pbf_layer(buffer);

        mapnik::vector_tile_impl::tile_datasource_pbf ds(pbf_layer,0,0,0,tile_size);

        mapnik::query q(ds.get_tile_extent());
        mapnik::featureset_ptr featureset = ds.features(q);

        CHECK(!featureset);
    }

    SECTION("VT Spec v2")
    {
        layer.set_version(2);
        layer.SerializePartialToString(&buffer);
        protozero::pbf_reader pbf_layer(buffer);

        mapnik::vector_tile_impl::tile_datasource_pbf ds(pbf_layer,0,0,0,tile_size);

        mapnik::query q(ds.get_tile_extent());
        mapnik::featureset_ptr featureset = ds.features(q);

        CHECK(!featureset);
    }
}

TEST_CASE( "datasource of pbf with unkown version returns a null featureset pointer" )
{
    std::string buffer;
    vector_tile::Tile_Layer layer;
    layer.set_name("test_name");
    layer.set_extent(4096);

    layer.set_version(3);
    layer.SerializePartialToString(&buffer);
    protozero::pbf_reader pbf_layer(buffer);

    mapnik::vector_tile_impl::tile_datasource_pbf ds(pbf_layer,0,0,0,tile_size);

    mapnik::query q(ds.get_tile_extent());
    mapnik::featureset_ptr featureset = ds.features(q);

    CHECK(!featureset);
}
