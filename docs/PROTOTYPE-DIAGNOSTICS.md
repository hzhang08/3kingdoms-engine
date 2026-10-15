# Native diagnostics — 0.9.2

New native sessions automatically write local diagnostics beneath:

`~/.local/share/caocao-prototype/diagnostics/sessions-v2/run-*/`

`XDG_DATA_HOME` or `CAOCAO_PROTOTYPE_SAVE_DIR` relocates the save directory; `CAOCAO_PROTOTYPE_LOG_DIR` selects an explicit diagnostic root. Historical `diagnostics/session-*.jsonl` files are preserved.

## What is recorded

- `session.json` preserves version, asset manifests, renderer/audio/AI settings, start time, process ID and observed exit status, even after event rotation.
- `events-0.jsonl` through `events-3.jsonl` record ordered events with timestamps, scenario, turn, phase and execution stage. Sort by `sequence` when combining rotated files.
- Actions have before/after state hashes; NPC steps include RNG and script progress. Story choices, reward notices, animation boundaries, save/load results, backup failures, AI decision identifiers, ten-second heartbeats and slow frames are included.
- Startup and caught runtime exceptions have `fatal_error` entries. A best-effort `failure-snapshot.json` captures committed engine state after a runtime exception; it is explicitly not a stable playback checkpoint.

The new diagnostics do not read credentials, dump environment variables, or include HTTP headers/bodies. JEV results in the game log use a field whitelist. Its existing separate request/cache logs remain under the save directory's `jev/` folder. This update adds no telemetry or network requests.

## Recovery snapshots

`recovery-latest.json` and `recovery-previous.json` contain ordinary prototype snapshots. They update only at safe boundaries, with no active animation, file dialog or pending JEV request. Identical states are not rewritten. Files are replaced atomically, retaining the previous checkpoint. Manual slots remain untouched.

To recover, launch the game, press **F6**, choose **导入备份**, and select a recovery file from the relevant run directory. Import validates the snapshot; choose a manual slot and save only after checking the restored game. The app does not automatically restore or overwrite a manual save.

These are best-effort local backups, not a guarantee against disk or power failure. A forced kill or native memory fault may leave no final error record; the last flushed events and completed recovery files remain useful. Failures before the log directory can open still cannot be recorded there.

## Bounds and isolation

Each run has four rotating 2 MiB event files. Recovery/failure snapshots are limited to 512 KiB each. At startup the logger keeps the eight newest inactive runs, plus currently active sessions and the new run. Only directories with its own metadata format are eligible for pruning. Older-format logs and unrelated directories are left alone.

Concurrent game instances have distinct run directories. Logging/storage failures are caught; a failed recovery write is retried on a later action rather than every rendered frame. Diagnostics do not alter game rules, RNG, save schema or the existing JEV decision contract. Schema 9 remains current.

## Verification

Evidence is under [evidence/diagnostics-0.9.2](evidence/diagnostics-0.9.2/):

- Native unit checks exercise rotation, final-error retention, concurrent sessions, cleanup boundaries, corrupt-checkpoint rejection, snapshot restoration and deterministic continuation.
- Mac/Linux native smoke checks exercise real reward rendering, combat, healing, saves/backups, command errors, allied actions and recovery guards. Snapshot files remain unchanged during animation and match the engine afterward.
- Deliberate missing-assets and invalid-option failures verify startup/runtime error capture. A deliberately unusable log directory still allows the gameplay smoke check to complete.
- WebAssembly compiles; the file logger is native-only. No browser UI retest or new gameplay-equivalence claim is implied.

Reproduce the focused native checks from the project root:

```sh
cmake --build build-mac --target caocao_prototype_app diagnostics_tests -j 6
build-mac/diagnostics_tests assets/prototype /tmp/caocao-diagnostic-unit-check
CAOCAO_JEV_MODE=off CAOCAO_PROTOTYPE_SAVE_DIR=/tmp/caocao-diagnostic-saves CAOCAO_PROTOTYPE_LOG_DIR=/tmp/caocao-diagnostic-logs build-mac/caocao_prototype_app assets/prototype --revised-smoke /tmp/caocao-diagnostic-ui-check
```
