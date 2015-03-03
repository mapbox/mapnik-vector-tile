MAPNIK_PLUGINDIR := $(shell mapnik-config --input-plugins)
BUILDTYPE ?= Release

all: libvtile

./deps/gyp:
	git clone https://chromium.googlesource.com/external/gyp.git ./deps/gyp && cd ./deps/gyp && git checkout 3464008

build/Makefile: ./deps/gyp gyp/build.gyp
	deps/gyp/gyp gyp/build.gyp --depth=. -DMAPNIK_PLUGINDIR=\"$(MAPNIK_PLUGINDIR)\" -Goutput_dir=. --generator-output=./build -f make

libvtile: build/Makefile Makefile
	@$(MAKE) -C build/ BUILDTYPE=$(BUILDTYPE) V=$(V)

test: libvtile
	./build/Release/tests

clean:
	rm -rf ./build

.PHONY: test


