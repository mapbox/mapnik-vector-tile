#!/bin/bash

set -eu
set -o pipefail

function install() {
    ./mason/mason install $1 $2
    ./mason/mason link $1 $2
}

ICU_VERSION="57.1"
MASON_VERSION="v0.14.2"

if [ ! -f ./mason/mason.sh ]; then
    mkdir -p ./mason
    curl -sSfL https://github.com/mapbox/mason/archive/${MASON_VERSION}.tar.gz | tar --gunzip --extract --strip-components=1 --exclude="*md" --exclude="test*" --directory=./mason
fi

# core deps
install protozero 1.5.2
install geometry 0.9.2
install wagyu 0.4.3
install protobuf 3.3.0

# mapnik
if [[ ${SKIP_MAPNIK_INSTALL:-} != 'YES' ]] && [[ ! -f ./mason_packages/.link/bin/mapnik-config ]]; then

    # mapnik deps
    install jpeg_turbo 1.5.1
    install libpng 1.6.28
    install libtiff 4.0.7
    install icu ${ICU_VERSION}
    install proj 4.9.3
    install pixman 0.34.0
    install cairo 1.14.8
    install webp 0.6.0
    install libgdal 2.1.3
    install boost 1.63.0
    install boost_libsystem 1.63.0
    install boost_libfilesystem 1.63.0
    install boost_libregex_icu57 1.63.0
    install freetype 2.7.1
    install harfbuzz 1.4.2-ft

    # mapnik
    install mapnik 3.0.14

fi
