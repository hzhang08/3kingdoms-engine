#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p docs/evidence/balance
case "${1:-mac}" in
 mac)
  cmake --build build-mac --target prototype_balance_tests prototype_tests caocao_prototype_app jev_enemy_tests -j8
  build-mac/prototype_balance_tests assets/prototype docs/evidence/balance/original-scenario-initialization.json docs/evidence/balance/mac
  build-mac/prototype_tests assets/prototype
  build-mac/jev_enemy_tests assets/prototype "$(mktemp -d /tmp/caocao-jev-tests.XXXXXX)"
  CAOCAO_JEV_MODE=off CAOCAO_PROTOTYPE_SAVE_DIR="$(mktemp -d /tmp/caocao-balance-ui.XXXXXX)" SDL_AUDIODRIVER=dummy build-mac/caocao_prototype_app assets/prototype --balance-smoke docs/evidence/balance/render-mac
  ;;
 linux)
  docker build --platform linux/amd64 -f scripts/Dockerfile -t caocao-engine-build:0.2 .
  docker run --rm --platform linux/amd64 -e CAOCAO_JEV_MODE=off -e CAOCAO_PROTOTYPE_SAVE_DIR=/tmp/balance-save -v "$PWD:/work" caocao-engine-build:0.2 sh -ec '
   cmake -S . -B build-linux -G Ninja -DCMAKE_BUILD_TYPE=Release
   cmake --build build-linux --target prototype_balance_tests caocao_prototype_app jev_enemy_tests -j8
   build-linux/prototype_balance_tests assets/prototype docs/evidence/balance/original-scenario-initialization.json docs/evidence/balance/linux
   build-linux/jev_enemy_tests assets/prototype /tmp/jev-tests
   SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software timeout 60 build-linux/caocao_prototype_app assets/prototype --balance-smoke docs/evidence/balance/render-linux'
  ;;
 web)
  export PATH="$PWD/.venv/bin:/opt/homebrew/bin:$PATH"
  source .cache/emsdk/emsdk_env.sh >/dev/null 2>&1
  emcmake cmake -S . -B build-web -G Ninja -DCMAKE_BUILD_TYPE=Release
  cmake --build build-web --target prototype_balance_tests caocao_prototype_app -j8
  node build-web/prototype_balance_tests.js assets/prototype docs/evidence/balance/original-scenario-initialization.json docs/evidence/balance/wasm
  ;;
 parity)
  cmp docs/evidence/balance/mac.trace docs/evidence/balance/linux.trace
  cmp docs/evidence/balance/mac.trace docs/evidence/balance/wasm.trace
  ;;
 *) echo 'Use mac, linux, web, or parity' >&2; exit 2;;
esac
