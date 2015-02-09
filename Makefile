PROTOBUF_CXXFLAGS=$(shell pkg-config protobuf --cflags)
PROTOBUF_LDFLAGS=$(shell pkg-config protobuf --libs-only-L) -lprotobuf-lite
MAPNIK_CXXFLAGS=$(shell mapnik-config --cflags)
MAPNIK_LDFLAGS=$(shell mapnik-config --libs --ldflags --dep-libs)
COMMON_FLAGS = -std=c++11 -Wall -pedantic -Wno-c++11-long-long -Wno-c++11-extensions -Wno-unknown-pragmas
# -Wshadow -Wsign-compare -Wsign-conversion -Wunused-parameter
# inherit from env
CXX := $(CXX)
CXXFLAGS := $(CXXFLAGS)
LDFLAGS := $(LDFLAGS)
MAPNIK_PLUGINDIR := $(shell mapnik-config --input-plugins)

all: mapnik-vector-tile

mapnik-vector-tile: src/vector_tile.pb.cc Makefile

src/vector_tile.pb.cc: proto/vector_tile.proto
	protoc -Iproto/ --cpp_out=./src proto/vector_tile.proto

test/run-test: Makefile src/vector_tile.pb.cc test/vector_tile.cpp test/test_utils.hpp src/*
	$(CXX) -o ./test/run-test test/vector_tile.cpp src/vector_tile.pb.cc -I./src $(CXXFLAGS) $(MAPNIK_CXXFLAGS) $(PROTOBUF_CXXFLAGS) $(COMMON_FLAGS) $(MAPNIK_LDFLAGS) $(PROTOBUF_LDFLAGS) $(LDFLAGS) -Wno-unused-private-field

test: test/run-test test/run-geom-test ./test/run-raster-test src/vector_tile.pb.cc test/catch.hpp
	./test/run-test
	./test/run-geom-test
	./test/run-raster-test

./test/run-raster-test: Makefile src/vector_tile.pb.cc test/raster_tile.cpp test/encoding_util.hpp src/*
	$(CXX) -o ./test/run-raster-test test/raster_tile.cpp src/vector_tile.pb.cc -DMAPNIK_PLUGINDIR=\"$(MAPNIK_PLUGINDIR)\" -I./src $(CXXFLAGS) $(MAPNIK_CXXFLAGS) $(PROTOBUF_CXXFLAGS) $(COMMON_FLAGS) $(MAPNIK_LDFLAGS) $(PROTOBUF_LDFLAGS) $(LDFLAGS) -Wno-unused-private-field
	./test/run-raster-test

./test/run-geom-test: Makefile src/vector_tile.pb.cc test/geometry_encoding.cpp test/encoding_util.hpp src/vector_tile_geometry_encoder.hpp src/*
	@$(CXX) -o ./test/run-geom-test test/geometry_encoding.cpp src/vector_tile.pb.cc -I./src $(CXXFLAGS) $(MAPNIK_CXXFLAGS) $(PROTOBUF_CXXFLAGS) $(COMMON_FLAGS) $(MAPNIK_LDFLAGS) $(PROTOBUF_LDFLAGS) $(LDFLAGS) -Wno-unused-private-field

geom: test/run-geom-test src/vector_tile.pb.cc
	./test/run-geom-test

clean:
	@rm -f ./src/vector_tile.pb.cc
	@rm -f ./src/vector_tile.pb.h
	@rm -f ./test/test-cfg.h
	@rm -f ./test/run-test
	@rm -f ./test/run-geom-test
	@rm -f ./test/run-raster-test

.PHONY: test
