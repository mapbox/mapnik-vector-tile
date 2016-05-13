## mapnik-vector-tile

A Mapnik implemention of [Mapbox Vector Tile specification](https://github.com/mapbox/vector-tile-spec).

Provides C++ headers that support rendering geodata into vector tiles and rendering vector tiles into images.

 - Master: [![Build Status](https://travis-ci.org/mapbox/mapnik-vector-tile.svg?branch=master)](https://travis-ci.org/mapbox/mapnik-vector-tile)
 - 0.6.x series: [![Build Status](https://secure.travis-ci.org/mapbox/mapnik-vector-tile.svg?branch=0.6.x)](http://travis-ci.org/mapbox/mapnik-vector-tile)

[![Coverage Status](https://coveralls.io/repos/mapbox/mapnik-vector-tile/badge.svg?branch=master&service=github)](https://coveralls.io/github/mapbox/mapnik-vector-tile?branch=master)

## Depends

 - mapnik-vector-tile >=1.0.x depends on Mapnik >=v3.0.11
 - mapnik-vector-tile 1.0.0 to 0.7.x depends on Mapnik v3.0.x (until 3.0.0 is released this means latest mapnik HEAD)
 - mapnik-vector-tile 0.6.x and previous work with Mapnik v2.2.x or v2.3.x
 - You will need `libmapnik` and `mapnik-config` available
 - Protobuf: `libprotobuf` and `protoc`

## Implementation details

Vector tiles in this code represent a direct serialization of Mapnik layers optimized for space efficient storage and fast deserialization. For those familiar with the Mapnik API vector tiles here can be considered a named array of `mapnik::featureset_ptr` whose geometries have been pre-tiled.

For more details see [vector-tile-spec](https://github.com/mapbox/vector-tile-spec).

### Ubuntu Dependencies Installation

    sudo apt-get install -y libprotobuf7 libprotobuf-dev protobuf-compiler
    sudo apt-add-repository --yes ppa:mapnik/nightly-2.3
    sudo apt-get update -y
    sudo apt-get -y install libmapnik=2.3.0* mapnik-utils=2.3.0* libmapnik-dev=2.3.0* mapnik-input-plugin*=2.3.0*

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
