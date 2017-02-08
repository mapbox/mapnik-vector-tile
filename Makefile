MAPNIK_PLUGINDIR := $(shell mapnik-config --input-plugins)
BUILDTYPE ?= Release

GEOMETRY_REVISION=v0.9.0
WAGYU_REVISION=0.3.0
PROTOZERO_REVISION=v1.5.1
GYP_REVISION=3464008

all: libvtile

./deps/gyp:
	git clone https://chromium.googlesource.com/external/gyp.git ./deps/gyp && cd ./deps/gyp && git checkout $(GYP_REVISION)

./deps/protozero:
	git clone https://github.com/mapbox/protozero.git ./deps/protozero && cd ./deps/protozero && git checkout $(PROTOZERO_REVISION)

./deps/geometry:
	git clone https://github.com/mapbox/geometry.hpp.git ./deps/geometry && cd ./deps/geometry && git checkout $(GEOMETRY_REVISION)

./deps/wagyu:
	git clone https://github.com/mapbox/wagyu.git ./deps/wagyu && cd ./deps/wagyu && git checkout $(WAGYU_REVISION)

build/Makefile: ./deps/gyp ./deps/protozero ./deps/wagyu ./deps/geometry gyp/build.gyp test/*
	deps/gyp/gyp gyp/build.gyp --depth=. -DMAPNIK_PLUGINDIR=\"$(MAPNIK_PLUGINDIR)\" -Goutput_dir=. --generator-output=./build -f make

libvtile: build/Makefile Makefile
	@$(MAKE) -C build/ BUILDTYPE=$(BUILDTYPE) V=$(V)

test/geometry-test-data:
	git submodule update --init

test: libvtile test/geometry-test-data
	DYLD_LIBRARY_PATH=$(MVT_LIBRARY_PATH) ./build/$(BUILDTYPE)/tests

testpack:
	rm -f ./*tgz
	npm pack
	tar -ztvf *tgz
	rm -f ./*tgz

clean:
	rm -rf ./build
	rm -rf ./deps/protozero
	rm -rf ./deps/wagyu

.PHONY: test


