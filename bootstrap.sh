#!/usr/bin/env bash

function setup_mason() {
    if [[ ! -d ./.mason ]]; then
        git clone --depth 1 https://github.com/mapbox/mason.git ./.mason
    else
        echo "Updating to latest mason"
        (cd ./.mason && git pull)
    fi
    export MASON_DIR=$(pwd)/.mason
    if [[ $(uname -s) == 'Linux' ]]; then source ./.mason/scripts/setup_cpp11_toolchain.sh; fi
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
    install mapnik dev
    install protobuf 2.6.1
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
