#!/usr/bin/env bash
set -euo pipefail

docker run --rm \
  -v "$PWD":/src \
  -w /src \
  emscripten/emsdk:latest \
  bash -lc '
    set -euo pipefail

    rm -rf build-wasm-debug

    emcmake cmake -S . -B build-wasm-debug \
      -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DFL_BUILD_TESTS=OFF \
      -DFAIRLANES_ENABLE_TRACY=OFF \
      -DCMAKE_CXX_FLAGS_RELWITHDEBINFO="-O1 -gsource-map -g3" \
      -DCMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO="-gsource-map -g3 -sASSERTIONS=2 -sSAFE_HEAP=1 -sSTACK_OVERFLOW_CHECK=2"

    cmake --build build-wasm-debug --target fairlanes -j

    find build-wasm-debug -type f | grep -E "fairlanes|worker|wasm|js|html|map" || true
  '
