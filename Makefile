MAPNIK_PLUGINDIR = $(shell mason_packages/.link/bin/mapnik-config --input-plugins)
BUILDTYPE ?= Release

GYP_REVISION=3464008

all: libvtile

mason_packages/.link/bin/mapnik-config:
	./install_mason.sh

./deps/gyp:
	git clone https://chromium.googlesource.com/external/gyp.git ./deps/gyp && cd ./deps/gyp && git checkout $(GYP_REVISION)

build/Makefile: mason_packages/.link/bin/mapnik-config ./deps/gyp gyp/build.gyp test/*
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
	rm -rf ./mason_packages

.PHONY: test


