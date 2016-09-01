set -e -u
set -o pipefail

if [[ ${COVERAGE:-false} == true ]]; then
    export LDFLAGS="--coverage ${LDFLAGS}"
    export CXXFLAGS="--coverage ${CXXFLAGS}"
fi

make -j${JOBS:-1} test BUILDTYPE=${BUILDTYPE:-Release} V=1

set +e +u
