#!/usr/bin/env bash

function setup_mason() {
    if [[ ! -d ./.mason ]]; then
        git clone --depth 1 https://github.com/mapbox/mason.git ./.mason
    else
        echo "Updating to latest mason"
        (cd ./.mason && git pull)
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

function install_mason_deps() {
    install mapnik 3.0.0-rc3
    install protobuf 2.6.1
    install freetype 2.5.5
    install harfbuzz 0.9.40
    install jpeg_turbo 1.4.0
    install libxml2 2.9.2
    install libpng 1.6.16
    install webp 0.4.2
    install icu 54.1
    install proj 4.8.0
    install libtiff 4.0.4beta
    install boost 1.57.0
    install boost_libsystem 1.57.0
    install boost_libthread 1.57.0
    install boost_libfilesystem 1.57.0
    install boost_libprogram_options 1.57.0
    install boost_libregex 1.57.0
    install boost_libpython 1.57.0
    install pixman 0.32.6
    install cairo 1.12.18
}

function setup_runtime_settings() {
    local MASON_LINKED_ABS=$(pwd)/mason_packages/.link
    export PROJ_LIB=${MASON_LINKED_ABS}/share/proj
    export ICU_DATA=${MASON_LINKED_ABS}/share/icu/54.1
    export GDAL_DATA=${MASON_LINKED_ABS}/share/gdal
    if [[ $(uname -s) == 'Darwin' ]]; then
        export DYLD_LIBRARY_PATH=$(pwd)/mason_packages/.link/lib:${DYLD_LIBRARY_PATH}
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
