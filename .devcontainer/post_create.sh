#!/usr/bin/env bash
set -euo pipefail

workspace="${containerWorkspaceFolder:-/workspaces/fairlanes}"

VSCODE_BIN_DIR="$(ls -d ~/.vscode-server/bin/* | head -n 1)"

echo "Installing FTXUI color swatch VSIX via vscode-server..."
"$VSCODE_BIN_DIR/bin/code-server" \
  --install-extension "$workspace/.devcontainer/extensions/ftxui-color-swatch-0.0.1.vsix" \
  --force
