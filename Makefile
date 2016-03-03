MAPNIK_PLUGINDIR := $(shell mapnik-config --input-plugins)
BUILDTYPE ?= Release

CLIPPER_REVISION=b82601d67830a84179888360e3dea645c1125a26
PROTOZERO_REVISION=v1.3.0
GYP_REVISION=3464008

all: libvtile

./deps/gyp:
	git clone https://chromium.googlesource.com/external/gyp.git ./deps/gyp && cd ./deps/gyp && git checkout $(GYP_REVISION)

./deps/protozero:
	git clone https://github.com/mapbox/protozero.git ./deps/protozero && cd ./deps/protozero && git checkout $(PROTOZERO_REVISION)

./deps/clipper:
	git clone https://github.com/mapnik/clipper.git -b r496-mapnik ./deps/clipper && cd ./deps/clipper && git checkout $(CLIPPER_REVISION) && ./cpp/fix_members.sh

build/Makefile: ./deps/gyp ./deps/clipper ./deps/protozero gyp/build.gyp test/*
	deps/gyp/gyp gyp/build.gyp --depth=. -DMAPNIK_PLUGINDIR=\"$(MAPNIK_PLUGINDIR)\" -Goutput_dir=. --generator-output=./build -f make

libvtile: build/Makefile Makefile
	@$(MAKE) -C build/ BUILDTYPE=$(BUILDTYPE) V=$(V)

test/geometry-test-data:
	git submodule update --init

test: libvtile test/geometry-test-data
	./build/$(BUILDTYPE)/tests

testpack:
	rm -f ./*tgz
	npm pack
	tar -ztvf *tgz
	rm -f ./*tgz

clean:
	rm -rf ./build
	rm -rf ./deps/protozero
	rm -rf ./deps/clipper

.PHONY: test


