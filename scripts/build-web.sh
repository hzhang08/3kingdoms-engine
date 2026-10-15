#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
# Use the Python environment validated by bootstrap instead of Xcode's Python
# 3.9, which cannot run the pinned Emscripten compiler.
export PATH="$PWD/.venv/bin:/opt/homebrew/bin:$PATH"
if [ ! -x .venv/bin/python ]; then
  echo 'Run scripts/bootstrap-mac.sh before building WebAssembly.' >&2
  exit 1
fi
source .cache/emsdk/emsdk_env.sh >/dev/null 2>&1
emcmake cmake -S . -B build-web -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-web -j 8
cp assets/generated/fonts/CaoCaoSong.ttf build-web/CaoCaoSong.ttf
