## mapnik-vector-tile

[![Build Status](https://secure.travis-ci.org/mapbox/mapnik-vector-tile.png)](http://travis-ci.org/mapbox/mapnik-vector-tile)

A Mapnik implemention of [Mapbox Vector Tile specification](https://github.com/mapbox/vector-tile-spec).

Provides C++ headers that support rendering geodata into vector tiles and rendering vector tiles into images.

## Depends

 - Mapnik > = v2.2.x: `libmapnik` and `mapnik-config`
 - Protobuf: `libprotobuf` and `protoc`

## Implementation details

Vector tiles in this code represent a direct serialization of Mapnik layers optimized for space efficient storage and fast deserialization. For those familiar with the Mapnik API vector tiles here can be considered a named array of `mapnik::featureset_ptr` whose geometries have been pre-tiled.

For more details see [vector-tile-spec](https://github.com/mapbox/vector-tile-spec).

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

## Authors

- [Artem Pavlenko](https://github.com/artemp)
- [Dane Springmeyer](https://github.com/springmeyer)
- [Konstantin Käfer](https://github.com/kkaefer)

## See also

- http://mike.teczno.com/notes/postgreslessness-mapnik-vectiles.html
- https://github.com/jones139/ceramic
- https://github.com/opensciencemap/vtm
