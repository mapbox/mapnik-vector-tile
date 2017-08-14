#!/bin/bash

set -eu
set -o pipefail

source ./bootstrap.sh

BUILDTYPE=${BUILDTYPE:-Release}

./build/${BUILDTYPE}/tests