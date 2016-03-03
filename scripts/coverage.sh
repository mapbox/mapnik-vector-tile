#!/usr/bin/env bash

set -e -u
set -o pipefail

COVERAGE=${COVERAGE:-false}

if [[ ${COVERAGE} == true ]]; then
    PYTHONUSERBASE=$(pwd)/mason_packages/.link pip install --user cpp-coveralls
    if [[ $(uname -s) == 'Linux' ]]; then
        export PYTHONPATH=$(pwd)/mason_packages/.link/lib/python2.7/site-packages
    else
        export PYTHONPATH=$(pwd)/mason_packages/.link/lib/python/site-packages
    fi
    ./mason_packages/.link/bin/cpp-coveralls \
        --build-root build \
        --gcov-options '\-lp' \
        --exclude deps/clipper/cpp/clipper.hpp \
        --exclude deps/clipper/cpp/clipper.cpp \
        --exclude examples \
        --exclude deps \
        --exclude test \
        --exclude bench \
        --exclude build/Debug/obj/gen/ \
        --exclude mason_packages \
        --exclude scripts > /dev/null
fi

set +e +u