#!/usr/bin/env bash
set -euo pipefail

run_as_root() {
  if command -v sudo >/dev/null 2>&1; then
    sudo "$@"
  else
    "$@"
  fi
}


echo "Installing Fairlanes VS Code extension..."

workspace="${containerWorkspaceFolder:-/workspaces/fairlanes}"

code --install-extension "$workspace/.devcontainer/extensions/ftxui-color-swatch-0.0.1.vsix" --force
echo "Installing build tools..."
run_as_root apt-get update
run_as_root apt-get install -y \
  build-essential \
  cmake \
  ninja-build \
  gdb \
  clangd \
  git \
  pkg-config \
  gcovr
