set -e -u
set -o pipefail

if [[ ${COVERAGE:-false} == true ]]; then
    # test only debug version
    export LDFLAGS="--coverage"
    export CXXFLAGS="--coverage"
    make debug -j${JOBS:-1} V=1
    make test-debug
else
    # test both release and debug
    make release -j${JOBS:-1} V=1
    make test
    make debug -j${JOBS:-1} V=1
    make test-debug
fi

set +e +u
