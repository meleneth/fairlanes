#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${BUILD_DIR:-build-wasm}"
DIST_DIR="${DIST_DIR:-dist/fairlanes}"
CONTENT_GENERATED_DIR="${CONTENT_GENERATED_DIR:-}"
HOST_GENERATED_CONTENT_DIR="${HOST_GENERATED_CONTENT_DIR:-$BUILD_DIR/host_generated/fairlanes_content}"

if [[ -z "$CONTENT_GENERATED_DIR" ]]; then
  for candidate in \
    build-linux-debug/generated/fairlanes_content \
    build/generated/fairlanes_content \
    build-reconcile-debug/generated/fairlanes_content; do
    if [[ -d "$candidate/fl/generated" ]]; then
      CONTENT_GENERATED_DIR="$candidate"
      break
    fi
  done
fi

if [[ -n "$CONTENT_GENERATED_DIR" && ! -d "$CONTENT_GENERATED_DIR/fl/generated" ]]; then
  echo "CONTENT_GENERATED_DIR must point at a generated Fairlanes content tree." >&2
  echo "Got: $CONTENT_GENERATED_DIR" >&2
  exit 1
fi

if [[ -z "$CONTENT_GENERATED_DIR" ]]; then
  if ! command -v ruby >/dev/null 2>&1; then
    echo "No generated Fairlanes content tree found and ruby is not available on the host PATH." >&2
    echo "Set CONTENT_GENERATED_DIR to an existing generated content tree or install ruby." >&2
    exit 1
  fi

  CONTENT_GENERATED_DIR="$HOST_GENERATED_CONTENT_DIR"
  echo "Generating Fairlanes content with host ruby: $(command -v ruby)"
  ruby scripts/fairlanes_content_codegen.rb --out-dir "$CONTENT_GENERATED_DIR"
fi

# rm -rf "$BUILD_DIR"
rm -rf "$DIST_DIR"

mkdir -p "$DIST_DIR"

content_cmake_arg=()
if [[ -n "$CONTENT_GENERATED_DIR" ]]; then
  echo "Using generated Fairlanes content from: $CONTENT_GENERATED_DIR"
  content_cmake_arg=("-DFAIRLANES_CONTENT_PREGENERATED_DIR=/src/$CONTENT_GENERATED_DIR")
fi

docker run --rm \
  --user "$(id -u):$(id -g)" \
  -e HOME=/tmp \
  -e FAIRLANES_CONTENT_PREGENERATED_DIR="${content_cmake_arg[*]}" \
  -v "$PWD":/src \
  -w /src \
  emscripten/emsdk:latest \
  bash -lc '
    set -euo pipefail
    emcmake cmake -S . -B build-wasm \
      -DCMAKE_BUILD_TYPE=Release \
      -DFL_BUILD_TESTS=OFF \
      -DFAIRLANES_ENABLE_TRACY=OFF \
      ${FAIRLANES_CONTENT_PREGENERATED_DIR}
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
