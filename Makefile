MAPNIK_PLUGINDIR := $(shell mapnik-config --input-plugins)
BUILDTYPE ?= Release

CLIPPER_REVISION=b82601d67830a84179888360e3dea645c1125a26
PROTOZERO_REVISION=v1.3.0
GYP_REVISION=3464008

all: libvtile

./dependencies/gyp:
	git clone https://chromium.googlesource.com/external/gyp.git ./dependencies/gyp && cd ./dependencies/gyp && git checkout $(GYP_REVISION)

./dependencies/protozero:
	git clone https://github.com/mapbox/protozero.git ./dependencies/protozero && cd ./dependencies/protozero && git checkout $(PROTOZERO_REVISION)

./dependencies/clipper:
	git clone https://github.com/mapnik/clipper.git -b r496-mapnik ./dependencies/clipper && cd ./dependencies/clipper && git checkout $(CLIPPER_REVISION) && ./cpp/fix_members.sh

build/Makefile: ./dependencies/gyp ./dependencies/clipper ./dependencies/protozero gyp/build.gyp test/*
	dependencies/gyp/gyp gyp/build.gyp --depth=. -DMAPNIK_PLUGINDIR=\"$(MAPNIK_PLUGINDIR)\" -Goutput_dir=. --generator-output=./build -f make

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
	rm -rf ./dependencies/protozero
	rm -rf ./dependencies/clipper

.PHONY: test


