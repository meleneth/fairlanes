#!/usr/bin/env bash
set -euo pipefail

docker run --rm \
  -v "$PWD":/src \
  -w /src \
  emscripten/emsdk:latest \
  bash -lc '
    set -euo pipefail

    rm -rf build-wasm

    emcmake cmake -S . -B build-wasm \
      -DCMAKE_BUILD_TYPE=Release \
      -DFAIRLANES_BUILD_TESTS=OFF \
      -DFAIRLANES_ENABLE_TRACY=OFF

    cmake --build build-wasm --target fairlanes -j
  '
