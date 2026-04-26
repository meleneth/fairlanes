#!/usr/bin/bash
set -euo pipefail

# from your project root
docker run --rm -it -v "$PWD":/src -w /src emscripten/emsdk:latest bash -lc 'emcmake cmake -S . -B build-wasm -G Ninja && cmake --build build-wasm -j'
