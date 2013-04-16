## mapnik-vector-tile

A high performance library for working with vector tiles from the
team at [MapBox](http://mapbox.com/about/team/).

Provides C++ headers that support rendering geodata into vector tiles
and rendering vector tiles into images.

Many efforts at vector tiles use [GeoJSON](http://www.geojson.org/) or other
text based formats that are easy to parse in the browser.

This approach is different: tiles are encoded as protobuf messages and
storage is tightly designed around the [Mapnik rendering engine](http://mapnik.org).
In this case Mapnik is the client rather than the browser. This provides a geodata
format that is a drop-in replacement for datasources like postgis and shapefiles
without compromising on speed and can be styled using existing design tools
like [TileMill](http://tilemill.com).


## Implementation details

Vector tiles in this code represent a direct serialization of Mapnik layers
optimized for space efficient storage and fast deserialization directly back
into Mapnik objects. For those familiar with the Mapnik API vector tiles
here can be considered a named array of `mapnik::featureset_ptr` whose geometries
have been pre-tiled.

A vector tile can consist of one or more ordered layers identified by name
and containing one or more features.

Features contain attributes and geometries: either point, linestring, or polygon.
Features expect single geometries so multipolygons or multilinestrings are represented
as multiple features, each storing a single geometry part.

Geometries are stored as an x,y,command stream (where `command` is a rendering command
like move_to or line_to). Geometries are clipped, reprojected into the map srs,
converted to screen coordinates, and [delta](http://en.wikipedia.org/wiki/Delta_encoding)
and [zigzag](http://en.wikipedia.org/wiki/Delta_encoding) encoded.

Feature attributes are encoded as key:value pairs which are dictionary encoded
at the layer level for compact storage of any repeated keys or values. Values use variant
type encoding supporting both unicode strings, boolean values, and various integer and
floating point types.

Vector tiles are serialized as protobuf messages which are then zlib compressed.

The assumed projection is Spherical Mercator (`epsg:3957`).

## Requires

- libmapnik 2.2-pre / current master
- libprotobuf and protoc

### Ubuntu Dependencies Installation

    apt-get install libprotobuf7 libprotobuf-dev protobuf-compiler
    apt-add-repository mapnik::nightly-trunk
    apt-get update && apt-get install libmapnik libmapnik-dev

### OS X Dependencies Installation

    brew install protobuf
    brew install mapnik --HEAD

## Building

Just type:

    make

This builds the protobuf C++ wrappers: `vector_tile.pb.cc` and `vector_tile.pb.h`

Then include `vector_tile.pb.cc` in your code. The rest is header only.

## Tests

Run the C++ tests like:

    make test

## Authors

- [Artem Pavlenko](https://github.com/artemp)
- [Dane Springmeyer](https://github.com/springmeyer)
- [Konstantin Käfer](https://github.com/kkaefer)

## See also

- http://mike.teczno.com/notes/postgreslessness-mapnik-vectiles.html
- https://github.com/mdaines/ceramic
- https://github.com/opensciencemap/VectorTileMap