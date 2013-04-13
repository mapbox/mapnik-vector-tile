## mapnik-vector-tile

Mapnik API for working with vector tiles

*** Experimental: not ready for widespread use ***

## Requires

- libmapnik 2.2-pre / current master
- libprotobuf and protoc

### Ubuntu Install

    apt-get install libprotobuf7 libprotobuf-dev protobuf-compiler

### OS X Install

    brew install protobuf

## Building

Just type:

    make

This builds the protobuf C++ wrappers: `vector_tile.pb.cc` and `vector_tile.pb.h`

That is all. Include `vector_tile.pb.cc` in your code. The rest is header only.

## Tests

Run the C++ tests like:

    make test
