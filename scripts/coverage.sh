#!/usr/bin/env bash

set -e -u
set -o pipefail

if [[ ${COVERAGE:-false} != false ]]; then
    curl -S -f https://codecov.io/bash -o codecov
    chmod +x codecov
    ./codecov -x "llvm-cov gcov" -Z
fi