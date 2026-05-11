#!/usr/bin/bash
set -euo pipefail

docker run --rm -it \
  -v "$PWD":/src \
  -w /src \
  emscripten/emsdk:latest \
  bash -lc 'emcmake cmake -S . -B build-wasm -DCMAKE_BUILD_TYPE=Release && cmake --build build-wasm -j'
