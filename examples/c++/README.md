
# tileinfo

A commandline tool to dump details about what layers, features, and geometries exist in a vector tile.

Vector tiles can be either zlib deflated (compressed) or not.


## Depends

 - C++ compiler
 - libprotobuf

Install these dependencies on Ubuntu:

    apt-get install pkg-config libprotobuf7 libprotobuf-dev protobuf-compiler build-essential g++

Install these dependencies on OS X:

    brew install pkg-config protobuf

## Installation

    make

## Usage

    tileinfo ../data/14_8716_8015.vector.pbf
