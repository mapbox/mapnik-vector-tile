MAPNIK_PLUGINDIR := $(shell mapnik-config --input-plugins)
BUILDTYPE ?= Release

CLIPPER_REVISION=e0973e0802
PBF_REVISION=1df6453
GYP_REVISION=3464008

all: libvtile

./deps/gyp:
	git clone https://chromium.googlesource.com/external/gyp.git ./deps/gyp && cd ./deps/gyp && git checkout $(GYP_REVISION)

./deps/pbf:
	git clone https://github.com/mapbox/pbf.hpp.git ./deps/pbf && cd ./deps/pbf && git checkout $(PBF_REVISION)

./deps/clipper:
	git clone https://github.com/mapnik/clipper.git -b r493-mapnik ./deps/clipper && cd ./deps/clipper && git checkout $(CLIPPER_REVISION) && ./cpp/fix_members.sh

build/Makefile: ./deps/gyp ./deps/clipper ./deps/pbf gyp/build.gyp test/*cpp
	deps/gyp/gyp gyp/build.gyp --depth=. -DMAPNIK_PLUGINDIR=\"$(MAPNIK_PLUGINDIR)\" -Goutput_dir=. --generator-output=./build -f make

libvtile: build/Makefile Makefile
	@$(MAKE) -C build/ BUILDTYPE=$(BUILDTYPE) V=$(V)

test: libvtile
	./build/$(BUILDTYPE)/tests

testpack:
	rm -f ./*tgz
	npm pack
	tar -ztvf *tgz
	rm -f ./*tgz

clean:
	rm -rf ./build

.PHONY: test


