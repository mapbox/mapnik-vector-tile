#include "catch.hpp"

// mapnik vector tile
#include "vector_tile_datasource_pbf.hpp"
#include "vector_tile_projection.hpp"

// mapnik
#include <mapnik/util/geometry_to_wkt.hpp>

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
    std::string buffer;
    vector_tile::Tile_Layer layer;

    SECTION("VT Spec v1")
    {
        layer.set_version(1);
        layer.SerializePartialToString(&buffer);
        protozero::pbf_reader pbf_layer(buffer);

        try
        {
            mapnik::vector_tile_impl::tile_datasource_pbf ds(pbf_layer,0,0,0);
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
            mapnik::vector_tile_impl::tile_datasource_pbf ds(pbf_layer,0,0,0);
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

        mapnik::vector_tile_impl::tile_datasource_pbf ds(pbf_layer,0,0,0);

        CHECK(ds.get_name() == "test_name");
    }

    SECTION("VT Spec v2")
    {
        layer.set_version(2);
        layer.SerializePartialToString(&buffer);
        protozero::pbf_reader pbf_layer(buffer);

        try
        {
            mapnik::vector_tile_impl::tile_datasource_pbf ds(pbf_layer,0,0,0);
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

        mapnik::vector_tile_impl::tile_datasource_pbf ds(pbf_layer,0,0,0);

        CHECK(ds.get_name() == "test_name");
    }

    SECTION("VT Spec v2")
    {
        layer.set_version(2);
        layer.SerializePartialToString(&buffer);
        protozero::pbf_reader pbf_layer(buffer);

        mapnik::vector_tile_impl::tile_datasource_pbf ds(pbf_layer,0,0,0);

        CHECK(ds.get_name() == "test_name");
    }
}

TEST_CASE( "extent of a tile effects the scale of features" )
{
    std::string buffer;
    vector_tile::Tile_Layer layer;
    layer.set_name("test_name");

    // Add feature to layer
    vector_tile::Tile_Feature * new_feature = layer.add_features();
    new_feature->set_type(vector_tile::Tile_GeomType_POINT);
    // MoveTo(5,5)
    new_feature->add_geometry(9); // move_to | (1 << 3)
    new_feature->add_geometry(protozero::encode_zigzag32(5));
    new_feature->add_geometry(protozero::encode_zigzag32(5));

    SECTION("default for v1 is 4096")
    {
        layer.set_version(1);
        layer.SerializePartialToString(&buffer);
        protozero::pbf_reader pbf_layer(buffer);

        mapnik::vector_tile_impl::tile_datasource_pbf ds(pbf_layer,0,0,0);

        mapnik::query q(ds.get_tile_extent());
        mapnik::featureset_ptr featureset = ds.features(q);
        REQUIRE(featureset);
        mapnik::feature_ptr feature = featureset->next();
        REQUIRE(feature);
        std::string wkt0;
        mapnik::util::to_wkt(wkt0, feature->get_geometry());
        CHECK(wkt0 == "POINT(-19988588.6446867 19988588.6446867)");
    }

    SECTION("geometry coordinates change if layer has different extent")
    {
        layer.set_extent(2048);
        layer.SerializePartialToString(&buffer);
        protozero::pbf_reader pbf_layer(buffer);

        mapnik::vector_tile_impl::tile_datasource_pbf ds(pbf_layer,0,0,0);

        mapnik::query q(ds.get_tile_extent());
        mapnik::featureset_ptr featureset = ds.features(q);
        REQUIRE(featureset);
        mapnik::feature_ptr feature = featureset->next();
        REQUIRE(feature);
        std::string wkt0;
        mapnik::util::to_wkt(wkt0, feature->get_geometry());
        CHECK(wkt0 == "POINT(-19939668.9465842 19939668.9465842)");
    }
}

TEST_CASE( "datasource of empty layer pbf returns a null featureset pointer" )
{
    // From the spec: A layer SHOULD contain at least one feature.
    // Unknown behavior when that is not the case. Current behavior is to
    // return a null featureset.
    std::string buffer;
    vector_tile::Tile_Layer layer;
    layer.set_name("test_name");
    layer.set_extent(4096);

    SECTION("VT Spec v1")
    {
        layer.set_version(1);
        layer.SerializePartialToString(&buffer);
        protozero::pbf_reader pbf_layer(buffer);

        mapnik::vector_tile_impl::tile_datasource_pbf ds(pbf_layer,0,0,0);

        mapnik::query q(ds.get_tile_extent());
        mapnik::featureset_ptr featureset = ds.features(q);

        CHECK(!featureset);
    }

    SECTION("VT Spec v2")
    {
        layer.set_version(2);
        layer.SerializePartialToString(&buffer);
        protozero::pbf_reader pbf_layer(buffer);

        mapnik::vector_tile_impl::tile_datasource_pbf ds(pbf_layer,0,0,0);

        mapnik::query q(ds.get_tile_extent());
        mapnik::featureset_ptr featureset = ds.features(q);

        CHECK(!featureset);
    }
}

TEST_CASE( "datasource of pbf with unkown version returns a null featureset pointer" )
{
    // From spec:
    // When a Vector Tile consumer encounters a Vector Tile layer with an unknown
    // version, it MAY make a best-effort attempt to interpret the layer, or it MAY
    // skip the layer. In either case it SHOULD continue to process subsequent layers
    // in the Vector Tile.
    std::string buffer;
    vector_tile::Tile_Layer layer;
    layer.set_name("test_name");
    layer.set_extent(4096);

    layer.set_version(3);
    layer.SerializePartialToString(&buffer);
    protozero::pbf_reader pbf_layer(buffer);

    mapnik::vector_tile_impl::tile_datasource_pbf ds(pbf_layer,0,0,0);

    mapnik::query q(ds.get_tile_extent());
    mapnik::featureset_ptr featureset = ds.features(q);

    CHECK(!featureset);
}

TEST_CASE( "datasource of empty layer pbf returns a null featureset pointer for features_at_point query" )
{
    // From the spec: A layer SHOULD contain at least one feature.
    // Unknown behavior when that is not the case. Current behavior is to
    // return a null featureset.
    std::string buffer;
    vector_tile::Tile_Layer layer;
    layer.set_name("test_name");
    layer.set_extent(4096);

    SECTION("VT Spec v1")
    {
        layer.set_version(1);
        layer.SerializePartialToString(&buffer);
        protozero::pbf_reader pbf_layer(buffer);

        mapnik::vector_tile_impl::tile_datasource_pbf ds(pbf_layer,0,0,0);

        mapnik::featureset_ptr featureset = ds.features_at_point(mapnik::coord2d(0.0,0.0),0.0001);

        CHECK(!featureset);
    }

    SECTION("VT Spec v2")
    {
        layer.set_version(2);
        layer.SerializePartialToString(&buffer);
        protozero::pbf_reader pbf_layer(buffer);

        mapnik::vector_tile_impl::tile_datasource_pbf ds(pbf_layer,0,0,0);

        mapnik::featureset_ptr featureset = ds.features_at_point(mapnik::coord2d(0.0,0.0),0.0001);

        CHECK(!featureset);
    }
}

TEST_CASE( "datasource of pbf with unknown version returns a null featureset pointer" )
{
    // From spec:
    // When a Vector Tile consumer encounters a Vector Tile layer with an unknown
    // version, it MAY make a best-effort attempt to interpret the layer, or it MAY
    // skip the layer. In either case it SHOULD continue to process subsequent layers
    // in the Vector Tile.
    std::string buffer;
    vector_tile::Tile_Layer layer;
    layer.set_name("test_name");
    layer.set_extent(4096);

    layer.set_version(3);
    layer.SerializePartialToString(&buffer);
    protozero::pbf_reader pbf_layer(buffer);

    mapnik::vector_tile_impl::tile_datasource_pbf ds(pbf_layer,0,0,0);

    mapnik::featureset_ptr featureset = ds.features_at_point(mapnik::coord2d(0.0,0.0),0.0001);

    CHECK(!featureset);
}
