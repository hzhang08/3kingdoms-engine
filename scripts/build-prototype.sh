#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p docs/evidence
case "${1:-all}" in
  linux)
    python3 scripts/stage-prototype-assets.py
    docker info >/dev/null
    docker image inspect caocao-engine-build:0.1 >/dev/null 2>&1 || docker build --platform linux/amd64 -t caocao-engine-build:0.1 -f scripts/Dockerfile .
    docker run --rm --platform linux/amd64 -e SDL_AUDIODRIVER=dummy -e CAOCAO_PROTOTYPE_SAVE_DIR=/tmp/prototype-smoke -v "$PWD:/work" caocao-engine-build:0.1 sh -ec '
      cmake -S . -B build-linux -G Ninja -DCMAKE_BUILD_TYPE=Release
      cmake --build build-linux --target prototype_tests caocao_prototype_app -j 8
      build-linux/prototype_tests assets/prototype docs/evidence/prototype-linux.snapshot.json
      build-linux/prototype_tests assets/prototype --resume docs/evidence/prototype-linux.snapshot.json
      xvfb-run -a build-linux/caocao_prototype_app assets/prototype --smoke docs/evidence/prototype-linux
      xvfb-run -a build-linux/caocao_prototype_app assets/prototype --resume-smoke
      xvfb-run -a build-linux/caocao_prototype_app assets/prototype --attack-smoke /tmp/prototype-attack-frames
      xvfb-run -a build-linux/caocao_prototype_app assets/prototype --movement-smoke /tmp/prototype-walking-frames
      xvfb-run -a build-linux/caocao_prototype_app assets/prototype --enemy-animation-slow-smoke /tmp/prototype-slow-enemy-frames
      xvfb-run -a build-linux/caocao_prototype_app assets/prototype --mixed-enemy-animation-smoke /tmp/prototype-mixed-enemy-frames
      xvfb-run -a build-linux/caocao_prototype_app assets/prototype --dialogue-smoke docs/evidence/prototype-dialogue-linux
      xvfb-run -a build-linux/caocao_prototype_app assets/prototype --resume-smoke
    '
    ;;
  web)
    export PATH="$PWD/.venv/bin:/opt/homebrew/bin:$PATH"
    source .cache/emsdk/emsdk_env.sh >/dev/null 2>&1
    emcmake cmake -S . -B build-web -G Ninja -DCMAKE_BUILD_TYPE=Release
    cmake --build build-web --target prototype_tests caocao_prototype_app -j 8
    node build-web/prototype_tests.js assets/prototype docs/evidence/prototype-wasm.snapshot.json
    node build-web/prototype_tests.js assets/prototype --resume docs/evidence/prototype-wasm.snapshot.json
    ;;
  all)
    "$0" linux
    "$0" web
    export PATH="$PWD/.venv/bin:/opt/homebrew/bin:$PATH"
    source .cache/emsdk/emsdk_env.sh >/dev/null 2>&1
    node build-web/prototype_tests.js assets/prototype --resume docs/evidence/prototype-linux.snapshot.json
    docker run --rm --platform linux/amd64 -v "$PWD:/work" caocao-engine-build:0.1 build-linux/prototype_tests assets/prototype --resume docs/evidence/prototype-wasm.snapshot.json
    node build-web/prototype_tests.js assets/prototype --resume docs/evidence/prototype-linux.snapshot.json.dialogue.json
    docker run --rm --platform linux/amd64 -v "$PWD:/work" caocao-engine-build:0.1 build-linux/prototype_tests assets/prototype --resume docs/evidence/prototype-wasm.snapshot.json.dialogue.json
    python3 scripts/package-prototype.py
    ;;
  *) echo 'Usage: scripts/build-prototype.sh [all|linux|web]' >&2; exit 2 ;;
esac
