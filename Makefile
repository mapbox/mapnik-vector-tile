MAPNIK_PLUGINDIR := $(shell mapnik-config --input-plugins)
BUILDTYPE ?= Release

WAGYU_REVISION=0.3.0
PROTOZERO_REVISION=v1.4.2
GYP_REVISION=3464008

all: libvtile

./deps/gyp:
	git clone https://chromium.googlesource.com/external/gyp.git ./deps/gyp && cd ./deps/gyp && git checkout $(GYP_REVISION)

./deps/protozero:
	git clone https://github.com/mapbox/protozero.git ./deps/protozero && cd ./deps/protozero && git checkout $(PROTOZERO_REVISION)

./deps/wagyu:
	git clone https://github.com/mapbox/wagyu.git ./deps/wagyu && cd ./deps/wagyu && git checkout $(WAGYU_REVISION)

build/Makefile: ./deps/gyp ./deps/protozero ./deps/wagyu gyp/build.gyp test/*
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


