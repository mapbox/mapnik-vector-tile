MAPNIK_PLUGINDIR := $(shell mapnik-config --input-plugins)
BUILDTYPE ?= Release

all: libvtile

./deps/gyp:
	git clone https://chromium.googlesource.com/external/gyp.git ./deps/gyp && cd ./deps/gyp && git checkout 3464008

./deps/clipper:
	git clone https://github.com/mapnik/clipper.git -b r493-mapnik ./deps/clipper && cd ./deps/clipper && git checkout a136c54 && ./cpp/fix_members.sh

build/Makefile: ./deps/gyp ./deps/clipper gyp/build.gyp test/*cpp
	deps/gyp/gyp gyp/build.gyp --depth=. -DMAPNIK_PLUGINDIR=\"$(MAPNIK_PLUGINDIR)\" -Goutput_dir=. --generator-output=./build -f make

libvtile: build/Makefile Makefile
	@$(MAKE) -C build/ BUILDTYPE=$(BUILDTYPE) V=$(V)

test: libvtile
	./build/$(BUILDTYPE)/tests

clean:
	rm -rf ./build

.PHONY: test


