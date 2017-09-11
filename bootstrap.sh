#!/usr/bin/env bash

# if built against mason package fix dynamic data locations
function setup_runtime_settings() {
    if [[ -f $(pwd)/mason_packages/.link/bin/mapnik-config ]]; then
        # TODO: use --proj-lib, --gdal-data, etc after https://github.com/mapnik/mapnik/pull/3759 is fixed
        #export PROJ_LIB=$(mapnik-config --proj-lib)
        #export GDAL_DATA=$(mapnik-config --gdal-data)
        #export ICU_DATA=$(mapnik-config --icu-data)
        local MASON_LINKED_ABS=$(pwd)/mason_packages/.link
        export PROJ_LIB=${MASON_LINKED_ABS}/share/proj
        ICU_VERSION=$(ls ${MASON_LINKED_ABS}/share/icu/)
        export ICU_DATA=${MASON_LINKED_ABS}/share/icu/${ICU_VERSION}
        export GDAL_DATA=${MASON_LINKED_ABS}/share/gdal
        if [[ $(uname -s) == 'Darwin' ]]; then
            export DYLD_LIBRARY_PATH=$(pwd)/mason_packages/.link/lib:${DYLD_LIBRARY_PATH:-}
            # OS X > 10.11 blocks DYLD_LIBRARY_PATH so we pass along using a
            # differently named variable
            export MVT_LIBRARY_PATH=${DYLD_LIBRARY_PATH}
        else
            export LD_LIBRARY_PATH=$(pwd)/mason_packages/.link/lib:${LD_LIBRARY_PATH:-}
        fi
        export PATH=$(pwd)/mason_packages/.link/bin:${PATH}
    fi
}

function main() {
    setup_runtime_settings
    echo "Ready, now run:"
    echo ""
    echo "    make test"
}

main
