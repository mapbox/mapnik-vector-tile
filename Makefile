GYP_REVISION=3464008

default: release

mason_packages/.link/bin:
	SKIP_MAPNIK_INSTALL=YES ./install_mason.sh

mason_packages/.link/bin/mapnik-config:
	./install_mason.sh

pre_build_check:
	@echo "Looking for mapnik-config on your PATH..."
	mapnik-config -v

./deps/gyp:
	git clone https://chromium.googlesource.com/external/gyp.git ./deps/gyp && cd ./deps/gyp && git checkout $(GYP_REVISION)

build/Makefile: ./deps/gyp gyp/build.gyp test/*
	deps/gyp/gyp gyp/build.gyp --depth=. -DMAPNIK_PLUGINDIR=\"$(shell mapnik-config --input-plugins)\" -Goutput_dir=. --generator-output=./build -f make
	$(MAKE) -C build/ V=$(V)

release: mason_packages/.link/bin/mapnik-config Makefile
	CXXFLAGS="-D_GLIBCXX_USE_CXX11_ABI=0" PATH="`pwd`/mason_packages/.link/bin/:${PATH}" $(MAKE) release_base

debug: mason_packages/.link/bin/mapnik-config Makefile
	CXXFLAGS="-D_GLIBCXX_USE_CXX11_ABI=0" PATH="`pwd`/mason_packages/.link/bin/:${PATH}" $(MAKE) debug_base

release_base: pre_build_check mason_packages/.link/bin Makefile
	BUILDTYPE=Release $(MAKE) build/Makefile

debug_base: pre_build_check mason_packages/.link/bin Makefile
	BUILDTYPE=Debug $(MAKE) build/Makefile

test/geometry-test-data/README.md:
	git submodule update --init

test: test/geometry-test-data/README.md
	BUILDTYPE=Release ./test/run.sh

test-debug: test/geometry-test-data/README.md
	BUILDTYPE=Debug ./test/run.sh

testpack:
	rm -f ./*tgz
	npm pack
	tar -ztvf *tgz
	rm -f ./*tgz

clean:
	rm -rf ./build

distclean: clean
	rm -rf ./mason
	rm -rf ./deps/gyp
	rm -rf ./mason_packages

.PHONY: test build/Makefile


