set -e -u
set -o pipefail

if [[ ${COVERAGE:-false} == true ]]; then
    export LDFLAGS="--coverage"
    export CXXFLAGS="--coverage"
fi

make -j${JOBS} test BUILDTYPE=${BUILDTYPE:-Release} V=1

set +e +u