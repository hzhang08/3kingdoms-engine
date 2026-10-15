#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
docker info >/dev/null
docker build --platform linux/amd64 -f scripts/Dockerfile -t caocao-engine-build:0.1 .
docker run --rm --platform linux/amd64 -e XDG_RUNTIME_DIR=/tmp -e SDL_AUDIODRIVER=dummy -v "$PWD:/work" caocao-engine-build:0.1 sh -c 'cmake -S . -B build-linux -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build-linux -j 8 && ctest --test-dir build-linux --output-on-failure && xvfb-run -a build-linux/caocao assets/generated --render-smoke && xvfb-run -a build-linux/caocao assets/generated --render-battle-smoke'
