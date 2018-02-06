## mapnik-vector-tile

A Mapnik implemention of [Mapbox Vector Tile specification](https://github.com/mapbox/vector-tile-spec).

Provides C++ headers that support rendering geodata into vector tiles and rendering vector tiles into images.

 - Master: [![Build Status](https://travis-ci.org/mapbox/mapnik-vector-tile.svg?branch=master)](https://travis-ci.org/mapbox/mapnik-vector-tile)
 - 0.6.x series: [![Build Status](https://secure.travis-ci.org/mapbox/mapnik-vector-tile.svg?branch=0.6.x)](http://travis-ci.org/mapbox/mapnik-vector-tile)

[![codecov](https://codecov.io/gh/mapbox/mapnik-vector-tile/branch/master/graph/badge.svg)](https://codecov.io/gh/mapbox/mapnik-vector-tile)

## Depends

## Implementation details

Vector tiles in this code represent a direct serialization of Mapnik layers optimized for space efficient storage and fast deserialization. For those familiar with the Mapnik API vector tiles here can be considered a named array of `mapnik::featureset_ptr` whose geometries have been pre-tiled.

For more details see [vector-tile-spec](https://github.com/mapbox/vector-tile-spec).

## Building from source

If you do not need to build against an external mapnik, just type:

    make

This will download all deps (including Mapnik) and compile against them.

To build and test in debug mode do:

    make debug test-debug

If you have Mapnik, libprotobuf, and all the Mapnik deps already installed on your system then you can build against them with:

    make release_base

Note: SSE optimizations are enabled by default. If you want to turn them off do:

```
SSE_MATH=false make
```

If building against an external Mapnik please know that Mapnik Vector Tile does not currently support Mapnik 3.1.x.

 - mapnik-vector-tile >=1.4.x depends on Mapnik >=v3.0.14
 - mapnik-vector-tile >=1.0.x depends on Mapnik >=v3.0.11
 - mapnik-vector-tile 1.0.0 to 0.7.x depends on Mapnik v3.0.x (until 3.0.0 is released this means latest mapnik HEAD)
 - mapnik-vector-tile 0.6.x and previous work with Mapnik v2.2.x or v2.3.x
 - You will need `libmapnik` and `mapnik-config` available
 - Protobuf: `libprotobuf` and `protoc`


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
