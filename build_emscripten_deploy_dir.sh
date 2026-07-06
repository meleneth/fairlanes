#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${BUILD_DIR:-build-wasm}"
DIST_DIR="${DIST_DIR:-dist/fairlanes}"

# rm -rf "$BUILD_DIR"
rm -rf "$DIST_DIR"

mkdir -p "$DIST_DIR"

docker run --rm \
  --user "$(id -u):$(id -g)" \
  -e HOME=/tmp \
  -v "$PWD":/src \
  -w /src \
  emscripten/emsdk:latest \
  bash -lc '
    set -euo pipefail
    emcmake cmake -S . -B build-wasm \
      -DCMAKE_BUILD_TYPE=Release \
      -DFL_BUILD_TESTS=OFF \
      -DFAIRLANES_ENABLE_TRACY=OFF
    cmake --build build-wasm --target fairlanes -j
  '

find "$BUILD_DIR" -type f | grep -E 'fairlanes\.(html|js|wasm|worker\.js|data|symbols|wasm\.map|js\.map)$' || true

cp webserve/Dockerfile "$DIST_DIR/"
cp webserve/nginx.conf "$DIST_DIR/"
cp webserve/index.html "$DIST_DIR/"
cp webserve/app.js "$DIST_DIR/"
cp webserve/xterm.js "$DIST_DIR/" 2>/dev/null || true
cp webserve/xterm.css "$DIST_DIR/" 2>/dev/null || true
cp webserve/xterm-addon-fit.js "$DIST_DIR/" 2>/dev/null || true

cp "$BUILD_DIR/fairlanes.js" "$DIST_DIR/"
cp "$BUILD_DIR/fairlanes.wasm" "$DIST_DIR/"
cp "$BUILD_DIR/"*.worker.js "$DIST_DIR/" 2>/dev/null || true
cp "$BUILD_DIR/"*.data "$DIST_DIR/" 2>/dev/null || true
cp "$BUILD_DIR/"*.map "$DIST_DIR/" 2>/dev/null || true

sha256sum "$DIST_DIR"/* >"$DIST_DIR/SHA256SUMS"

echo "Built deploy bundle:"
cat "$DIST_DIR/SHA256SUMS"
