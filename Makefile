GYP_REVISION=024e167

SSE_MATH ?= true

default: release

mason_packages/.link/bin:
	SKIP_MAPNIK_INSTALL=YES ./install_mason.sh

mason_packages/.link/bin/mapnik-config:
	./install_mason.sh

pre_build_check:
	@echo "Looking for mapnik-config on your PATH..."
	mapnik-config -v

./deps/gyp:
	git clone https://github.com/refack/GYP.git ./deps/gyp && cd ./deps/gyp && git checkout $(GYP_REVISION)

build/Makefile: pre_build_check ./deps/gyp gyp/build.gyp test/*
	python deps/gyp/gyp_main.py gyp/build.gyp -Denable_sse=$(SSE_MATH) --depth=. -DMAPNIK_PLUGINDIR=\"$(shell mapnik-config --input-plugins)\" -Goutput_dir=. --generator-output=./build -f make
	$(MAKE) -C build/ V=$(V)

release: mason_packages/.link/bin/mapnik-config Makefile
	CXXFLAGS="-D_GLIBCXX_USE_CXX11_ABI=0 $(CXXFLAGS)" $(MAKE) release_base

debug: mason_packages/.link/bin/mapnik-config Makefile
	CXXFLAGS="-D_GLIBCXX_USE_CXX11_ABI=0 $(CXXFLAGS)" $(MAKE) debug_base

# note: we set PATH to the mason bins to pick up protoc
# and CXXFLAGS/LDFLAGS to find protobuf headers/libs
# This will only find mason installed mapnik-config if run via the `release` or `debug` targets
release_base: mason_packages/.link/bin Makefile
	CXXFLAGS="-isystem `pwd`/mason_packages/.link/include $(CXXFLAGS)" \
	 LDFLAGS="-L`pwd`/mason_packages/.link/lib $(LDFLAGS)" \
	 PATH="`pwd`/mason_packages/.link/bin/:${PATH}" \
	 BUILDTYPE=Release $(MAKE) build/Makefile

debug_base: mason_packages/.link/bin Makefile
	CXXFLAGS="-isystem `pwd`/mason_packages/.link/include $(CXXFLAGS)" \
	 LDFLAGS="-L`pwd`/mason_packages/.link/lib $(LDFLAGS)" \
	 PATH="`pwd`/mason_packages/.link/bin/:${PATH}" \
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


