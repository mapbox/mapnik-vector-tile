## mapnik-vector-tile

[![Build Status](https://secure.travis-ci.org/mapbox/mapnik-vector-tile.png)](http://travis-ci.org/mapbox/mapnik-vector-tile)

A high performance library for working with vector tiles.

Provides C++ headers that support rendering geodata into vector tiles
and rendering vector tiles into images.

## Depends

 - Mapnik > = v2.2.x: `libmapnik` and `mapnik-config`
 - Protobuf: `libprotobuf` and `protoc`

## Implementation details

Vector tiles in this code represent a direct serialization of Mapnik layers optimized for space efficient storage and fast deserialization. For those familiar with the Mapnik API vector tiles here can be considered a named array of `mapnik::featureset_ptr` whose geometries have been pre-tiled.

A vector tile can consist of one or more ordered layers identified by name and containing one or more features.

Features contain attributes and geometries: either point, linestring, or polygon.

Geometries are stored as an x,y,command stream (where `command` is a rendering command like move_to or line_to). Geometries are clipped, reprojected into spherical mercator, converted to screen coordinates, and [delta](http://en.wikipedia.org/wiki/Delta_encoding) and [zigzag](https://developers.google.com/protocol-buffers/docs/encoding#types) encoded.

Feature attributes are encoded as key:value pairs which are dictionary encoded at the layer level for compact storage of any repeated keys or values. Values use variant type encoding supporting both unicode strings, boolean values, and various integer and floating point types.

Vector tiles are serialized as protobuf messages which are then zlib compressed.

The assumed projection is Spherical Mercator (`epsg:3857`).

### Ubuntu Dependencies Installation

    apt-get install libprotobuf7 libprotobuf-dev protobuf-compiler
    apt-add-repository ppa:mapnik/v2.2.0
    apt-get update && apt-get install libmapnik libmapnik-dev

### OS X Dependencies Installation

    brew install protobuf
    brew install mapnik

## Building

Just type:

    make

This builds the protobuf C++ wrappers: `vector_tile.pb.cc` and `vector_tile.pb.h`

Then include `vector_tile.pb.cc` in your code. The rest is header only.

## Tests

Run the C++ tests like:

    make test

## Examples

### C++

See examples in examples/c++

### Python

These require the protobuf python bindings and the `rtree` library
which can be installed like:

    sudo apt-get install libspatialindex-dev
    pip install protobuf rtree

To build and test the python example code do:

    make python && make python-test

Run the example code:

    python examples/python/hello-world.py

## Authors

- [Artem Pavlenko](https://github.com/artemp)
- [Dane Springmeyer](https://github.com/springmeyer)
- [Konstantin Käfer](https://github.com/kkaefer)

## See also

- http://mike.teczno.com/notes/postgreslessness-mapnik-vectiles.html
- https://github.com/mdaines/ceramic
- https://github.com/opensciencemap/VectorTileMap
