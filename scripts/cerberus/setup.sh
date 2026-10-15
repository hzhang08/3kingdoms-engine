#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
export GHIDRA_NOTES_ENABLE_SHARED=0 GHIDRA_NOTES_AUTO_SYNC=0 GHIDRA_NOTES_REPO='' CERBERUS_MCP_ALLOW_UNSAFE_RUN=0 CERBERUS_MCP_BRIDGE_WRITE_OK=0 CERBERUS_MCP_RUNTIME_NAMES='' CERBERUS_MCP_RUNTIME_PIDS=''
commit=3116c94de17386d18367ed14e55dd2c7d1c7fe14
cache="$PWD/.cache/cerberus-build"
repo="$PWD/.cache/cerberus-re-skill-review"
mkdir -p "$cache" output/reverse-engineering/cerberus
if [[ ! -d "$repo/.git" ]]; then git clone --no-checkout https://github.com/OwenPawl/cerberus-re-skill.git "$repo"; fi
git -C "$repo" cat-file -e "$commit^{commit}"
# git archive exports the pinned object without changing any reviewed checkout.
mkdir -p "$cache/cerberus"
git -C "$repo" archive "$commit" | tar -x -C "$cache/cerberus"
archive=ghidra_12.0.4_PUBLIC_20260303.zip
if [[ ! -f "$cache/$archive" ]]; then
 curl -fL --retry 2 -o "$cache/$archive" "https://github.com/NationalSecurityAgency/ghidra/releases/download/Ghidra_12.0.4_build/$archive"
fi
printf '%s  %s\n' c3b458661d69e26e203d739c0c82d143cc8a4a29d9e571f099c2cf4bda62a120 "$cache/$archive" | shasum -a 256 -c -
cp scripts/cerberus/Dockerfile scripts/cerberus/requirements.lock.txt "$cache/"
docker build --platform linux/amd64 -t caocao-cerberus:12.0.4 "$cache"
