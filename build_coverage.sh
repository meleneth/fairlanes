#!/usr/bin/bash
set -euo pipefail

mkdir -p build-coverage/coverage/html
pushd build-coverage
cmake .. -DENABLE_COVERAGE=ON
popd

cmake --build build-coverage --target coverage

## clean old counters (optional but often wise)
#lcov --directory build-coverage --zerocounters
#
## baseline: records all instrumented files as 0% (uses .gcno)
#lcov --capture --initial --directory build-coverage --output-file baseline.info
#
## run tests to create .gcda
#ctest --test-dir build-coverage --output-on-failure
#
## real capture
#lcov --capture --directory build-coverage --output-file run.info
#
# merge + filter + html
#lcov --add-tracefile baseline.info --add-tracefile run.info --output-file total.info
#lcov --remove run.info '/usr/*' '*/_deps/*' '*/tests/*' --output-file total.cleaned.info
#genhtml total.cleaned.info --output-directory build-coverage/coverage/html
