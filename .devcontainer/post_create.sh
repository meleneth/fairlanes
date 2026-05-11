#!/usr/bin/env bash
set -euo pipefail

run_as_root() {
  if command -v sudo >/dev/null 2>&1; then
    sudo "$@"
  else
    "$@"
  fi
}

echo "Installing build tools..."
run_as_root apt-get update
run_as_root apt-get install -y \
  build-essential \
  cmake \
  ninja-build \
  bubblewrap \
  gdb \
  clangd \
  git \
  pkg-config \
  gcovr


workspace="${containerWorkspaceFolder:-/workspaces/fairlanes}"

VSCODE_BIN_DIR="$(ls -d ~/.vscode-server/bin/* | head -n 1)"

echo "Installing VSIX via vscode-server..."
"$VSCODE_BIN_DIR/bin/code-server" \
  --install-extension "$workspace/.devcontainer/extensions/ftxui-color-swatch-0.0.1.vsix" \
  --force
