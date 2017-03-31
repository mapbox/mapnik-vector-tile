#!/usr/bin/env bash

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
    setup_runtime_settings
    echo "Ready, now run:"
    echo ""
    echo "    make test"
}

main
