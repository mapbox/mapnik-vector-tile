PROTOBUF_CXXFLAGS=$(shell pkg-config protobuf --cflags)
PROTOBUF_LDFLAGS=$(shell pkg-config protobuf --libs-only-L) -lprotobuf-lite
CXXFLAGS := $(CXXFLAGS) # inherit from env
LDFLAGS := $(LDFLAGS) # inherit from env

all: tileinfo

tileinfo: tileinfo.cpp ../../src/vector_tile.pb.cc
	$(CXX) $(PROTOBUF_CXXFLAGS) $(PROTOBUF_LDFLAGS) tileinfo.cpp ../../src/vector_tile.pb.cc -o tileinfo -lprotobuf-lite -lz

test:
	./tileinfo ../data/14_8716_8015.vector.pbf
	./tileinfo ../data/14_2620_6331.vector.pbf.z

install:
	cp ./tileinfo /usr/local/bin/tileinfo
	chmod +x /usr/local/bin/tileinfo

clean:
	@rm -f ./tileinfo

.PHONY: test
