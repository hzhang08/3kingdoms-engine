#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p docs/evidence/scenarios
if [ "${CAOCAO_SKIP_STAGE:-0}" != 1 ]; then python3 scripts/stage-prototype-assets.py; fi
case "${1:-all}" in
 mac)
  cmake --build build-mac --target prototype_tests prototype_scenarios_tests caocao_prototype_app -j8
  build-mac/prototype_scenarios_tests assets/prototype docs/evidence/scenarios/mac
  build-mac/prototype_tests assets/prototype
  export CAOCAO_PROTOTYPE_SAVE_DIR="$(mktemp -d /tmp/caocao-battles-mac.XXXXXX)"
  SDL_AUDIODRIVER=dummy build-mac/caocao_prototype_app assets/prototype --scenarios-smoke docs/evidence/scenarios/render-mac
  SDL_AUDIODRIVER=dummy build-mac/caocao_prototype_app assets/prototype --scenarios-resume-smoke docs/evidence/scenarios/render-mac
  SDL_AUDIODRIVER=dummy build-mac/caocao_prototype_app assets/prototype --scenarios-combat-smoke docs/evidence/scenarios/render-mac
  ;;
 linux)
  docker info >/dev/null
  docker run --rm --platform linux/amd64 -e SDL_AUDIODRIVER=dummy -e CAOCAO_PROTOTYPE_SAVE_DIR=/tmp/caocao-battles-linux -v "$PWD:/work" caocao-engine-build:0.1 sh -ec '
   cmake -S . -B build-linux -G Ninja -DCMAKE_BUILD_TYPE=Release
   cmake --build build-linux --target prototype_tests prototype_scenarios_tests caocao_prototype_app -j8
   build-linux/prototype_scenarios_tests assets/prototype docs/evidence/scenarios/linux
   build-linux/prototype_tests assets/prototype
   xvfb-run -a build-linux/caocao_prototype_app assets/prototype --scenarios-smoke docs/evidence/scenarios/render-linux
   xvfb-run -a build-linux/caocao_prototype_app assets/prototype --scenarios-resume-smoke docs/evidence/scenarios/render-linux
   xvfb-run -a build-linux/caocao_prototype_app assets/prototype --scenarios-combat-smoke docs/evidence/scenarios/render-linux
  '
  ;;
 web)
  export PATH="$PWD/.venv/bin:/opt/homebrew/bin:$PATH"
  source .cache/emsdk/emsdk_env.sh >/dev/null 2>&1
  emcmake cmake -S . -B build-web -G Ninja -DCMAKE_BUILD_TYPE=Release
  cmake --build build-web --target prototype_tests prototype_scenarios_tests caocao_prototype_app -j8
  node build-web/prototype_scenarios_tests.js assets/prototype docs/evidence/scenarios/wasm
  node build-web/prototype_tests.js assets/prototype
  ;;
 parity)
  export PATH="$PWD/.venv/bin:/opt/homebrew/bin:$PATH"
  source .cache/emsdk/emsdk_env.sh >/dev/null 2>&1
  for battle in 1 2 3; do
   cmp "docs/evidence/scenarios/mac-$battle.trace" "docs/evidence/scenarios/linux-$battle.trace"
   cmp "docs/evidence/scenarios/linux-$battle.trace" "docs/evidence/scenarios/wasm-$battle.trace"
   node build-web/prototype_scenarios_tests.js assets/prototype --resume "docs/evidence/scenarios/linux-$battle.json"
   docker run --rm --platform linux/amd64 -v "$PWD:/work" caocao-engine-build:0.1 build-linux/prototype_scenarios_tests assets/prototype --resume "docs/evidence/scenarios/wasm-$battle.json"
   build-mac/prototype_scenarios_tests assets/prototype --resume "docs/evidence/scenarios/linux-$battle.json"
  done
  ;;
 all)
  "$0" mac
  "$0" linux
  "$0" web
  "$0" parity
  ;;
 *) echo 'Usage: scripts/check-prototype-scenarios.sh [all|mac|linux|web|parity]' >&2; exit 2 ;;
esac
