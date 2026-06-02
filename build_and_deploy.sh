#!/usr/bin/env bash
set -euo pipefail

sudo rm -rf build-wasm

./build_emscripten_deploy_dir.sh

rsync -av --delete dist/ whirred.io:dist/

ssh whirred.io '
  set -euo pipefail
  cd dist/fairlanes
  docker build -t fairlanes-staging .
  cd /www/docker-stuff
  docker compose up -d
'
