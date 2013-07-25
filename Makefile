PROTOBUF_CXXFLAGS=$(shell pkg-config protobuf --cflags)
PROTOBUF_LDFLAGS=$(shell pkg-config protobuf --libs-only-L) -lprotobuf-lite
MAPNIK_CXXFLAGS=$(shell mapnik-config --cflags) -Wsign-compare
MAPNIK_LDFLAGS=$(shell mapnik-config --libs --ldflags --dep-libs)
CXXFLAGS := $(CXXFLAGS) # inherit from env
LDFLAGS := $(LDFLAGS) # inherit from env

all: mapnik-vector-tile

mapnik-vector-tile: src/vector_tile.pb.cc

src/vector_tile.pb.cc: proto/vector_tile.proto
	protoc -Iproto/ --cpp_out=./src proto/vector_tile.proto

python/vector_tile_pb2.py: proto/vector_tile.proto
	protoc -Iproto/ --python_out=python proto/vector_tile.proto

python: python/vector_tile_pb2.py

test/run-test: src/vector_tile.pb.cc test/vector_tile.cpp test/test_utils.hpp src/*
	$(CXX) -o ./test/run-test test/vector_tile.cpp src/vector_tile.pb.cc -I./src $(CXXFLAGS) $(MAPNIK_CXXFLAGS) $(PROTOBUF_CXXFLAGS) $(MAPNIK_LDFLAGS) $(PROTOBUF_LDFLAGS) $(LDFLAGS) -Wno-unused-private-field

test: test/run-test src/vector_tile.pb.cc
	./test/run-test

python-test: python/vector_tile_pb2.py
	python ./test/python/test.py

clean:
	@rm -f ./src/vector_tile.pb.cc
	@rm -f ./src/vector_tile.pb.h
	@rm -f ./test/run-test
	@rm -f ./python/vector_tile_pb2.py
	@rm -f ./python/*pyc

.PHONY: test
