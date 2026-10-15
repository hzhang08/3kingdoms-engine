#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
command -v cmake >/dev/null && command -v ninja >/dev/null || HOMEBREW_NO_AUTO_UPDATE=1 HOMEBREW_NO_INSTALL_CLEANUP=1 brew install cmake ninja
PYTHON_BIN="${PYTHON_BIN:-/opt/homebrew/bin/python3}"
"$PYTHON_BIN" -c 'import sys; assert sys.version_info >= (3, 10), "Python 3.10+ required"'
"$PYTHON_BIN" -m venv .venv
.venv/bin/pip install -r requirements.txt
mkdir -p .cache
if [ ! -d .cache/emsdk/.git ]; then git clone https://github.com/emscripten-core/emsdk.git .cache/emsdk; fi
git -C .cache/emsdk checkout e566f7bdcc7735f44037911c24b87a58a3c93145
"$PYTHON_BIN" .cache/emsdk/emsdk.py install 6.0.10
"$PYTHON_BIN" .cache/emsdk/emsdk.py activate 6.0.10
