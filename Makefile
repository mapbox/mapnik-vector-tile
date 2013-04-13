all: mapnik-vector-tile

src/vector_tile.pb.cc:
	protoc -Iproto/ --cpp_out=./src proto/vector_tile.proto

run-test: test/vector_tile.cpp test/test_utils.hpp src/*
	$(CXX) -o run-test test/vector_tile.cpp src/vector_tile.pb.cc -I./src `mapnik-config --cflags --libs --ldflags ` -licuuc -lboost_system -lprotobuf -Wno-unused-private-field

test: run-test src/vector_tile.pb.cc
	./run-test

mapnik-vector-tile: src/vector_tile.pb.cc

clean:
	@rm -f src/vector_tile.pb.cc
	@rm -f src/vector_tile.pb.h
	@rm -f ./run-test

.PHONY: test
