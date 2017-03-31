#!/usr/bin/env bash

set -e -u
set -o pipefail

if [[ ${COVERAGE:-false} != false ]]; then
    ./codecov -x "llvm-cov gcov" -Z
fi