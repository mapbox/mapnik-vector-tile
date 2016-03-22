#!/usr/bin/env bash

set -e -u
set -o pipefail

COVERAGE=${COVERAGE:-false}

if [[ ${COVERAGE} == true ]]; then
    easy_install --user --upgrade pip
    export PATH="${HOME}/Library/Python/2.7/bin:${PATH}"

    PYTHONUSERBASE=$(pwd)/mason_packages/.link pip install --user cpp-coveralls
    if [[ $(uname -s) == 'Linux' ]]; then
        export PYTHONPATH=$(pwd)/mason_packages/.link/lib/python2.7/site-packages
    else
        export PYTHONPATH=$(pwd)/mason_packages/.link/lib/python/site-packages
    fi
    ./mason_packages/.link/bin/cpp-coveralls \
        --build-root build \
        --gcov-options '\-lp' \
        --exclude examples \
        --exclude deps \
        --exclude test \
        --exclude bench \
        --exclude build/Debug/obj/gen/ \
        --exclude mason_packages \
        --exclude scripts > /dev/null
fi

set +e +u