#!/bin/bash

set -eu
set -o pipefail

function install() {
    ./mason/mason install $1 $2
    ./mason/mason link $1 $2
}

ICU_VERSION="58.1"
MASON_VERSION="485514d8"

if [ ! -f ./mason/mason.sh ]; then
    mkdir -p ./mason
    curl -sSfL https://github.com/mapbox/mason/archive/${MASON_VERSION}.tar.gz | tar --gunzip --extract --strip-components=1 --exclude="*md" --exclude="test*" --directory=./mason
fi

# core deps
install protozero 1.6.4
install geometry 1.0.0
install wagyu 0.4.3
install protobuf 3.5.1

# mapnik
if [[ ${SKIP_MAPNIK_INSTALL:-} != 'YES' ]] && [[ ! -f ./mason_packages/.link/bin/mapnik-config ]]; then

    # mapnik deps
    install jpeg_turbo 1.5.2
    install libpng 1.6.32
    install libtiff 4.0.8
    install icu ${ICU_VERSION}-brkitr
    install proj 7.2.1 libproj
    install pixman 0.34.0
    install cairo 1.14.8
    install webp 0.6.0
    install libgdal 2.2.3
    install boost 1.75.0
    install boost_libsystem 1.75.0
    install boost_libfilesystem 1.75.0
    install boost_libregex_icu58 1.75.0
    install freetype 2.7.1
    install harfbuzz 1.4.4-ft

    # mapnik
    install mapnik e553f55dc

fi
