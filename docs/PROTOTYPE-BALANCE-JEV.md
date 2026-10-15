# Battle balance and native JEV decisions — prototype 0.4.0

2026-10-06. Applies to new simplified prototype battles. Original files, the
Electron/Wine project, campaign-engine investigations and existing saves remain
unchanged. Restart the native executable to load this release; press **B** to
select a new battle or **R** to restart. Loading an older save deliberately keeps
its historical stats and rules, rather than changing a battle in progress.

## Corrected initialization in every battle

The old prototype assigned every enemy level 1, friendly NPCs the same level as
the player, and omitted both NPC default equipment and script opcode 72. This
made the later battles artificially easy. The new implementation reads the
verified initialization prefix directly from the checksummed S_00..S_03 script
assets and uses the existing original-rule C++ helpers:

1. Take the trimmed mean of selected party levels (`deployment_level`).
2. Apply opcode 69's scenario adjustment. All four scripts encode 52, meaning −2.
3. Apply each opcode 70/71 row's own level adjustment and promotion mode.
4. Generate class/level-appropriate default weapon and armor, then apply opcode
   72's equipment IDs, removal/auto-level rules and class restrictions.

The standalone party levels remain explicit prototype inputs (3, 10, 10, 20),
not recovered campaign progression. The party now also receives appropriate
ordinary equipment; Cao Cao retains R_00's 倚天剑. NPC growth/equipment is checked
against original x86 execution, not inferred from JEV advice.

| Battle | Party preset | Ordinary NPC level | Notable enemy level / scripted equipment |
|---|---:|---:|---|
| 颍川 | 3 | 1 | 张宝/张梁 5; 张宝's opcode 72 accessory restored |
| 汜水关 | 10 | 8 | 华雄 14; all four equipment assignments restored, including 孙坚/祖茂/袁绍 |
| 虎牢关 | 10 | 8 | 吕布 14 with 方天画戟 and 赤兔马; other named enemies 12 |
| 董卓追击 | 20 | 18 | 吕布 24 with the same scripted treasures; 李儒/徐荣/李傕 22 |

For example, 虎牢关吕布 changes from level 1, HP110, attack50, defense50,
no equipment, to level14, HP188, attack147, defense147. Cao Cao is level10,
HP147, attack102, defense106 after his equipment correction. These are displayed
stats, not terrain-adjusted damage promises. See `evidence/balance/mac.json` for
every unit, and `original-scenario-initialization.json` for the executable oracle.

Two other shared errors are corrected: 防御 now displays physical defense
(index1), not spirit (index2); new games use the recovered hit/damage result
without the artificial 12–45 clamp, so misses do zero damage and strong attacks
can exceed45. Historical saves retain the old clamp to preserve continuation.

The original gear records and their supported statistical/range/movement effects
are restored. This does not add all treasure special attacks: 方天画戟's additional
attack behavior, counterattacks, critical/double strikes, tactics, items and XP
remain outside this simplified mode. 赤兔马's zone-of-control bypass uses the
existing recovered movement helper. Original ambush timing, duels, guard/event AI
changes, departing pursuit units and campaign progression are still incomplete.

## JEV at runtime

Native macOS/Linux uses `typesafe-ai/jev` at the fixed Vercel HTTPS evaluate
endpoint. With a valid credential loaded at startup it is on by default. **J** toggles
JEV/local AI; **I** shows the current setting. The enemy status line shows
thinking, the accepted choice, cache reuse, or a local fallback. Browser/WASM
continues to use local AI: no key or HTTP client is compiled into it.

For each actionable enemy unit, the shared engine generates up to48 legal
move/physical-attack options, including its deterministic local suggestion,
wait, targets and alternative positions. The submitted state includes scenario,
objective, turn, terrain, units' positions/HP/stats/equipment and legal choices.
It omits credentials, local paths, save files and hidden RNG state. JEV returns
one choice ID. C++ revalidates the choice and the exact request state before
committing; the model cannot edit HP, teleport, set RNG or execute commands.

The native worker is asynchronous. At most one request is active. Curl has a
15-second deadline, with a17-second worker deadline, bounded output, verified
TLS and no redirects. Missing credentials, HTTP/network failure, invalid response
or timeout uses option a0 (local AI). Restart/load/toggle/quit cancels pending
work; stale responses cannot apply to another battle. New requests start only outside menus/dialogue/animation boundaries.
An in-flight response may finish while a menu is open; its action waits. Cancelled requests do not become game
commands. Saving is blocked during an in-flight decision; load/restart remain available.

Credentials stay external. The default path is
`$HOME/Documents/credentials/vercel-cert-jev.txt`; override it with
`CAOCAO_JEV_CREDENTIAL_FILE`. Since 0.9.3 the native client reads and validates
the key once during startup, keeps it in memory for that session, and never
reopens the file for requests, battle changes, save/load or J toggles. A missing,
denied or invalid key leaves local AI enabled; configure it and restart to retry.
Rotating/deleting the file does not change an already running session.
`CAOCAO_JEV_MODE=off` skips the startup read entirely; enabling JEV then requires
a restart without that setting. macOS may still ask for Documents access during
startup. The credential buffer is cleared after the last worker stops at shutdown.
Startup diagnostics record only ready/unavailable/invalid/disabled, never the key.
The key
is sent only in the authorization header; it is not in argv, environment,
request logs, assets, snapshots or release packages. Native HTTP uses
`/usr/bin/curl` without a shell or curlrc. macOS provides it; on Linux install
`curl` and `ca-certificates` for JEV. Local gameplay does not require them.

Every evaluation attempt is logged in the native save directory under `jev/`,
with request, response or failure, selected option, latency and provider usage.
A pending record is written before the HTTP request. `jev/cache/` stores
checksummed decisions keyed to the precise battle state. Fallback choices are
cached too, preventing connectivity changes from rerolling that recorded action.
These local gameplay logs contain no API key.

## Startup credential verification (0.9.3)

`jev_credential_tests` (also CTest `jev_credential_startup`) passes on macOS
and Linux. It loads a synthetic key, removes/replaces its file, then submits
uncached oversized requests: they reach request validation with the cached key
and stop before HTTP. Missing/invalid/oversized/disabled startup credentials,
restart after key rotation, and absence of the synthetic key from logs/cache
are also checked. Existing asynchronous fallback/cache/cancellation tests pass.
A headless native frontend smoke confirms one `ready` startup event, unchanged
battle/save/reward checks and no key in diagnostics or snapshots. WebAssembly
still compiles without this native credential client. Evidence is under
`docs/evidence/jev-startup-0.9.3/`. No live JEV HTTP calls were made for this change;
no macOS permission was reset to artificially retrigger its consent dialog.

## Saves and determinism

New snapshots use schema5, engine`prototype-0.5`, ruleset`script-balance-v1`,
original/expansion manifest identities and a payload checksum. Schemas1–4 still
restore with their original balance, damage and hash behavior. Old saves use
local AI; restart/new-battle switches to the corrected rules and permits JEV.
No save migration overwrites a slot until the user saves.

A resolved-but-unexecuted JEV choice is part of the portable snapshot. It replays
without another API call and consumes exactly the same combat RNG. Subsequent
state-identical visits reuse the native decision cache. **Future, not-yet-made
model decisions are external input:** exporting only a snapshot to another
machine does not copy its future cache, and cannot promise identical future JEV
answers. The deterministic cross-platform guarantee applies to a recorded choice
sequence; unrecorded online model calls are not claimed deterministic.

## Evidence and limits

- `tools/prototype_balance_probe.py`: original handlers68/69/70/71/72/75 and
  battle construction0x44cc85, fixed declared party inputs, isolated CPU emulator.
  Archive reads and rendering/collection I/O boundaries are explicit in the
  fixture. Original SHA256:
  `199b1b67cf966ead46c49b430b9aab84bdc7d4f0e8dfa8e0ebeec5da280b01e0`.
- Cerberus/Ghidra dossier0x44274c supplies construction context; executable
  probes determine the records. All108 spawned NPC records match the oracle
  (21/28/32/27). The full fixture also includes unspawned opening NPCs.
- macOS/Linux/Wasm share trace SHA256
  `2e0eb5fa1e685b4df79cf58f7b6cfa5f1f11360daf6c91cc412a80458ce4321b`:
  971 recorded choice continuations, old-save restoration, restart into new
  rules, altered-stat rejection and passive defeat.
- Historical opening traces remain379 checkpoints
  `25de5680625b5f0d35a0526298adb15bf3adc0cbffdcbe8ceec8a4e4ca2295e4`
  and365 checkpoints
  `6916cde52483c54ce12fb7e2a8d9cd3d3e4d714e93f93fcf68882ddb419fdcfe`.
  Historical scenario victories/save tests also pass.
- Native SDL renders all four corrected battlefields; the native JEV scheduler
  is exercised on macOS with live model responses and cached responses, queued
  move/attack presentation, and portable pending-choice replay. Separate tests
  reject malformed choices and verify missing-key fallback/caching/cancellation.
- A greedy local autoplayer wins颍川(turn9) and汜水关(turn11), loses虎牢关(turn4)
  and董卓追击(turn9). Recorded attacks include misses and damage up to88.
  This is a regression observation, not a human difficulty or victory guarantee.
  New continuous human/JEV victory playthroughs of all four battles are unverified.
- JEV is an alternate prototype opponent, not a reconstruction of original AI.
  In the sampled live decisions it selected a0, the local suggestion. No claim
  is made that these few samples establish improved strategic strength.
- No new Chrome/Safari UI acceptance run: this change prioritizes native
  gameplay; Wasm compilation/state parity is verified. Full original fidelity
  and the earlier full acceptance checklist remain incomplete.

## Reproduce

Use the existing pinned SDL3/CMake/Emscripten setup in `PROTOTYPE.md`.

```sh
.venv/bin/python tools/prototype_balance_probe.py
python3 scripts/stage-prototype-assets.py
scripts/check-prototype-balance.sh mac
scripts/check-prototype-balance.sh linux
scripts/check-prototype-balance.sh web
scripts/check-prototype-balance.sh parity
python3 scripts/package-prototype-mac.py
python3 scripts/package-prototype.py
```

The Linux script builds image`caocao-engine-build:0.2` with explicit
`--platform linux/amd64`. Live JEV smoke tests are opt-in, send battle state and
use API credit; invoke `caocao_prototype_app assets/prototype --jev-smoke OUTPUT
SCENARIO` with an isolated `CAOCAO_PROTOTYPE_SAVE_DIR`. Scenarios0–3 are supported.
`--balance-smoke OUTPUT` renders all four without model requests. Source and
new test commands are indexed in `TOOLS-CATALOG.md`.

API references: [JEV typed evaluation](https://github.com/vercel/eve/blob/main/docs/guides/evaluate.md),
[SDL3 process API](https://wiki.libsdl.org/SDL3/CategoryProcess).
