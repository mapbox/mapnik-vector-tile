PROTOBUF_CXXFLAGS=$(shell pkg-config protobuf --cflags)
PROTOBUF_LDFLAGS=$(shell pkg-config protobuf --libs-only-L) -lprotobuf-lite
CXXFLAGS := $(CXXFLAGS) # inherit from env
LDFLAGS := $(LDFLAGS) # inherit from env

all: tileinfo

tileinfo: tileinfo.cpp ../../src/vector_tile.pb.cc
	$(CXX) tileinfo.cpp ../../src/vector_tile.pb.cc -o tileinfo -lprotobuf-lite

test:
	./tileinfo ../data/14_8716_8015.vector.pbf

clean:
	@rm -f ./tileinfo

.PHONY: test
