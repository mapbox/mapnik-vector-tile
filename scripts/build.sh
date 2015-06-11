set -e -u
set -o pipefail

if [[ ${COVERAGE:-false} == true ]]; then
    export LDFLAGS="--coverage"
    export CXXFLAGS="--coverage"
fi

echo $PROJ_LIB
ls $PROJ_LIB/
make -j${JOBS} test BUILDTYPE=${BUILDTYPE:-Release} V=1

set +e +u