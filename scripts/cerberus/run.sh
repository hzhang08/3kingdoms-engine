#!/usr/bin/env bash
# Static-only Cerberus: sole target is this edition of Ekd5.exe, mounted read-only.
set -euo pipefail
cd "$(dirname "$0")/../.."
case "${1:-} ${2:-}" in
  'doctor '|'import analyze'|'export function-dossier') ;;
  *) echo 'Use doctor, import analyze, or export function-dossier.' >&2; exit 2 ;;
esac
source_exe=${CAOCAO_EXE:-/Users/henryzhang/Downloads/sgzccz/Ekd5.exe}
expected=199b1b67cf966ead46c49b430b9aab84bdc7d4f0e8dfa8e0ebeec5da280b01e0
actual=$(shasum -a 256 "$source_exe")
[[ "${actual%% *}" = "$expected" ]] || { echo 'Unsupported executable hash' >&2; exit 1; }
mkdir -p output/reverse-engineering/cerberus
exec docker run --rm --platform linux/amd64 --network none --hostname caocao-re --add-host caocao-re:127.0.0.1 --cap-drop ALL --security-opt no-new-privileges --pids-limit 256 --memory 6g \
  -e GHIDRA_NOTES_ENABLE_SHARED=0 -e GHIDRA_NOTES_AUTO_SYNC=0 -e GHIDRA_NOTES_REPO= \
  -e CERBERUS_MCP_ALLOW_UNSAFE_RUN=0 -e CERBERUS_MCP_BRIDGE_WRITE_OK=0 -e CERBERUS_MCP_RUNTIME_NAMES= -e CERBERUS_MCP_RUNTIME_PIDS= \
  --mount "type=bind,source=$source_exe,target=/input/Ekd5.exe,readonly" \
  --mount "type=bind,source=$PWD/output/reverse-engineering/cerberus,target=/workspace" \
  caocao-cerberus:12.0.4 "$@"
