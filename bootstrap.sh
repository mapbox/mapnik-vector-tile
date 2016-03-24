#!/usr/bin/env bash

MASON_VERSION="694d08c"

function setup_mason() {
    if [[ ! -d ./.mason ]]; then
        git clone https://github.com/mapbox/mason.git ./.mason
        (cd ./.mason && git checkout ${MASON_VERSION})
    else
        echo "Updating to latest mason"
        (cd ./.mason && git fetch && git checkout ${MASON_VERSION})
    fi
    export MASON_DIR=$(pwd)/.mason
    export PATH=$(pwd)/.mason:$PATH
    export CXX=${CXX:-clang++}
    export CC=${CC:-clang}
}

function install() {
    MASON_PLATFORM_ID=$(mason env MASON_PLATFORM_ID)
    if [[ ! -d ./mason_packages/${MASON_PLATFORM_ID}/${1}/ ]]; then
        mason install $1 $2
        mason link $1 $2
    fi
}

ICU_VERSION="55.1"

function install_mason_deps() {
    install ccache 3.2.4
    install mapnik latest
    install protobuf 2.6.1
    install freetype 2.6
    install harfbuzz 0.9.40
    install jpeg_turbo 1.4.0
    install libxml2 2.9.2
    install libpng 1.6.17
    install webp 0.4.2
    install icu ${ICU_VERSION}
    install proj 4.8.0
    install libtiff 4.0.4beta
    install boost 1.59.0
    install boost_liball 1.59.0
    install pixman 0.32.6
    install cairo 1.14.2
}

function setup_runtime_settings() {
    local MASON_LINKED_ABS=$(pwd)/mason_packages/.link
    export PROJ_LIB=${MASON_LINKED_ABS}/share/proj
    export ICU_DATA=${MASON_LINKED_ABS}/share/icu/${ICU_VERSION}
    export GDAL_DATA=${MASON_LINKED_ABS}/share/gdal
    if [[ $(uname -s) == 'Darwin' ]]; then
        export DYLD_LIBRARY_PATH=$(pwd)/mason_packages/.link/lib:${DYLD_LIBRARY_PATH}
        # OS X > 10.11 blocks DYLD_LIBRARY_PATH so we pass along using a
        # differently named variable
        export MVT_LIBRARY_PATH=${DYLD_LIBRARY_PATH}
    else
        export LD_LIBRARY_PATH=$(pwd)/mason_packages/.link/lib:${LD_LIBRARY_PATH}
    fi
    export PATH=$(pwd)/mason_packages/.link/bin:${PATH}
}

function main() {
    setup_mason
    install_mason_deps
    setup_runtime_settings
    echo "Ready, now run:"
    echo ""
    echo "    make test"
}

main
