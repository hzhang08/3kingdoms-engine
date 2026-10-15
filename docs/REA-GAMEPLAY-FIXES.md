> **0.9.1 crash fix, 2026-10-08:** A real Mac session exited at 17:51:05 while displaying the 太平清领道 reward. The renderer looked for `terrain-benefits/images/76.bmp`, but the original equipment icon is bundled at `unit-details/images/Item/076.bmp`. The exception reached the outer handler and returned exit status 1; no matching macOS crash report was found. The reward renderer now uses the equipment atlas for treasures and retains the existing consumable artwork. Fatal errors and a best-effort committed-state snapshot are written to the local session log without overwriting manual slots. The failure was reproduced before the fix; actual scripted 太平清领道/布手套 notices, five additional item-icon fixtures, saves/backups, counters and healing pass on Mac and Linux. Wasm compilation passes; browser UI was not rerun for this patch. Evidence: [reproduction error](evidence/reward-crash-2026-10-08/repro-output.txt), [Mac verification](evidence/reward-crash-2026-10-08/mac/verification.json), [Linux verification](evidence/reward-crash-2026-10-08/linux/verification.json), [error-log verification](evidence/reward-crash-2026-10-08/logging-verification.json). Gameplay rules and schema 9 are unchanged. The 0.9.0 investigation and binary identities below are preserved as historical evidence.

# Native gameplay repairs — prototype 0.9.0 baseline

2026-10-08. This release implements the first substantial repair pass from [REA-GAMEPLAY-GAP-ANALYSIS.md](REA-GAMEPLAY-GAP-ANALYSIS.md). It is a playable four-battle replacement engine, not a claim of identical original gameplay or completion of the original acceptance checklist.

## What changed

| Audit gap | Shipped change | Remaining boundary |
|---|---|---|
| Single physical strike | Original controller ordering, criticals, doubles, counters, applicable equipment/status effects and XP now drive playable attacks. Every strike has native playback. | Adapted notices replace some original windows; physical area and automatic-bean controllers outside current rosters remain explicit unsupported paths. |
| No experience | Character and weapon/armor XP, level growth, caps, maximum-stat refresh and deterministic restoration. | Promotion and equipment-changing interfaces remain absent. |
| Five tactics only | Fifteen tactics: 灼热、火阵、浊流、水阵、旋风、落石、地阵、诱惑、钝兵、虚脱、毒烟、定身、小补给、气合、霸气. Healing/support targets, status effects, area targets, MP and XP are connected. | Not all 68 tactics; some higher-level learned tactics still show 暂未开放. Area effects use original frames sequentially per target, not original whole-scene choreography. |
| Allied/enemy behavior | Separate allied phase; side 1 is no longer player-controlled. Original action-priority ordering, deployment AI settings, stationary/conditional-engagement modes and scripted activation are used. | Candidate scoring and target-following remain adaptations. Local/JEV choices are not original AI equivalence. |
| Missing battle events | Source-offset event interpreter restores selected encounters, hidden reinforcements, ambushes, duel dialogue/terminal battlefield effects, activation, removals and event flags. | Original duel choreography, full opening tutorial, some location-specific opcode84 behavior and intervening Rxx stories remain incomplete. |
| Incorrect victory paths | Opening victory follows Zhang Bao/Zhang Liang events; pursuit includes retreat choice, changed objective and west-edge escape. Original rectangle/count/duel conditions have fresh executable probes. | Full original campaign/event equivalence remains unproven. |
| No rewards/carryover | Script treasure identifiers/counts are recorded once and visible in inventory. 下一战 carries party records, equipment/XP, remaining consumables and treasure history; NPC scaling follows carried levels. | Reward equipment levels, cash arithmetic, equipping rewards and original recruitment/story transitions remain unresolved. New members use explicit prototype initialization. |
| Simplified confusion | Original luck, healer-neighbor and equipment recovery; grade recovery, poison and terrain/equipment regeneration. Zhang Bao's recovery accessory no longer forces a wasted action. | Recovery presentation is adapted. |
| Missing 方天画戟 pursuit | A qualifying kill chooses one further target and runs one additional attack controller; no recursive pursuit chain. | Verified bounded gate/selector plus controller composition, not a complete original playthrough with every treasure. |

## Saves and controls

Normal new games use schema 9, engine `prototype-0.9`, ruleset `revised-battles-v1`. Old schemas retain their historical battle rules when loaded; restarting or choosing a new independent battle enables the new rules. Existing save files, original game files and the Electron project are preserved.

- **F6 / 存档位** selects one of ten local slots, reads the old per-battle save, or imports/exports a JSON backup.
- **F5/F9** save/load the selected slot. Backup import validates before replacing live state and never overwrites a slot automatically.
- **下一战** after completed victory dialogue/rewards carries progress. **B** intentionally starts an independent preset; it does not carry progress.
- **P / 查看道具** includes earned treasure counts. Treasure equipment changes are not yet available.
- Story choices are clickable or use number keys. Allies and enemies play through the same attack/movement presentation queue.

Native slots are `~/.local/share/caocao-prototype/manual-slot-1.json` through `manual-slot-10.json`. The legacy `slot-1.json` and `slot-battle-N.json` paths remain readable. Browser saves use IndexedDB under the page's origin; backups use the same schema across platforms. Filesystem writes are atomic replacements. Save validation checks growth, equipment identities, event cursors, reward ledgers, campaign history, RNG, pending AI decisions and manifests before committing.

## Evidence and tests

Fresh rule evidence uses the unchanged edition SHA-256 `199b1b67cf966ead46c49b430b9aab84bdc7d4f0e8dfa8e0ebeec5da280b01e0`.

- 73 original attack-controller cases: 121 strikes, 40 counters, 8 second strikes and 14 criticals; exact retained controller traces and RNG.
- 72 original pursuit gate/selection cases, including 12 cases using the real caller's preferred-target value.
- 1,240 original rectangle/count/duel-condition cases; source cursor, reinforcement, retreat, victory and one-time reward tests.
- 1,500 original tactic-helper cases, plus healing and multi-target command/save/playback tests.
- Three campaign-continuation fixtures, cap cases and six forged-history rejection cases. These use synthetic victories and do not establish three ordinary complete campaign wins.
- Four finite autonomous battle traces, restoring after every committed action. The deliberately simple test player may lose; victory/defeat paths are checked independently with scenario fixtures.
- Native rendering smoke checks counters, healing, ten slots, backup import/export and corrupt-file rejection. Visual snapshots are inspected separately from rule hashes.

Machine-readable reports, process logs, snapshots and screenshots are under [evidence/revised-gameplay](evidence/revised-gameplay/). `checks-{mac,linux,wasm}.json` records the exact results; `cross-platform.json` records comparisons. Original oracles are under `tests/fixtures/original_prototype_*` and `original_attack_controller.json`. A passing helper fixture is not a whole-game equivalence claim.

### Release verification

- All five revised-rule suites passed on macOS, containerized Linux x86-64 and WebAssembly/Node. Their output traces are byte-identical across the three builds. Four Linux snapshots restored identically in WebAssembly, and four WebAssembly snapshots restored identically on macOS.
- Seven historical prototype regression suites passed on macOS. Native rendering/persistence smoke tests passed on both macOS and Linux; the Linux run used Xvfb in the existing local container environment.
- The finite autoplay player won 颍川 (turn 5) and 汜水关 (turn 11), and lost 虎牢关 (turn 3) and 追击 (turn 7). These are test-player outcomes, not demonstrations of balanced difficulty or four ordinary player victories. Separate scenario fixtures exercise victory, defeat, retreat and turn-limit conditions.
- The signed portable Mac executable ran with adjacent assets from `/tmp/曹操 原生验证 0.9`, with `/tmp` as its working directory and JEV disabled. It passed counters, healing, ten slots, backup restoration and corrupt-backup rejection. This verifies the relocated executable; it is not a fresh Finder double-click test. Native backup smoke tests exercise the file-dialog completion callback, not a manual OS-picker session.
- Chrome accepted a Linux-generated backup through the actual file-input control. The in-app browser also imported it and passed local save/reload. Browser export was clicked, but the automation did not receive a download event; inspection of Chrome's internal Downloads page was blocked by browser policy. Browser export is therefore **unverified**, rather than passed or classified as a proven game bug. Safari and a full browser battle playthrough were not run in this repair pass.

See [verification index](evidence/revised-gameplay/index.json), [native rendering inspection](evidence/revised-gameplay/native-mac/inspection.png), and [Linux backup restored in Chrome](evidence/revised-gameplay/browser-linux-import.png). Result files named `events-*.json` contain line-oriented state-hash traces, not JSON objects; their runner metadata is valid JSON in `checks-*.json`.

## REA, Cerberus and JEV

REA MCP became available in this chat. Its session reports no configured native decompiler provider, so no REA decompilation is claimed. REA is used for the packaged-process evidence boundary; original semantics come from hash-gated bounded x86 execution, existing Cerberus artifacts, original scripts and recovered C++ helpers. New probe tools are `prototype_pursuit_probe.py`, `prototype_scenario_events_probe.py` and `prototype_extended_tactics_probe.py`.

The relocated package capture is retained as `packaged-process-rea.json`, Evidence ID `ev_ea9be763da616c7e0ff8744cbd61b2720466864b7110770533662f81a443c4f4`. Its executable SHA-256 is `c0d0849c9d4e0228acb4df70a9f7cdf3de936200f790f762bd33686a461de3ef`. REA reports sampled process trees, before/after filesystem observations and unrecorded inherited environment; it does not establish complete network isolation. The independently retained `packaged-rea/verification.json` preserves the numeric slot count (10); REA's normalized terminal text replaces that number with a port placeholder.

JEV advised the save migration/validation design and was exercised once with the revised gameplay context. The context advertises actual tactic/counter capabilities, current grades/status, scripted progress and legal options; hidden/dead troops are omitted from the unit list. Its response is validated and stored before execution, and restored decisions replay without another network call. JEV advice is not evidence of original mechanics. See [JEV-DECISIONS.md](JEV-DECISIONS.md) for every attempt.

## Reproduction

From the project root, use the existing pinned CMake/SDL3/Emscripten environment and `caocao-engine-build:0.1` Linux image. The local OrbStack daemon must be running for Docker.

```sh
.venv/bin/python tools/import_prototype_extended_tactics.py
.venv/bin/python tools/prototype_unit_assets.py
python3 scripts/stage-prototype-assets.py
cmake --build build-mac --target caocao_prototype_app prototype_combat_tests prototype_events_tests prototype_tactics_extended_tests prototype_campaign_tests prototype_revised_tests -j 6
python3 scripts/check-revised-gameplay.py build-mac mac
```

For Linux, run the same CMake targets and `scripts/check-revised-gameplay.py build-linux linux` inside the existing `/work` container mount. For WebAssembly, source `.cache/emsdk/emsdk_env.sh`, build the same targets in `build-web`, and run `python3 scripts/check-revised-gameplay.py build-web wasm`. Tests use Node for deterministic traces; browser rendering/storage checks are separate.

```sh
CAOCAO_JEV_MODE=off CAOCAO_PROTOTYPE_SAVE_DIR=/tmp/caocao-revised-ui build-mac/caocao_prototype_app assets/prototype --revised-smoke /tmp/caocao-revised-ui-evidence
python3 scripts/package-prototype-mac.py
python3 scripts/package-prototype.py
```

The live JEV test is opt-in: `build-mac/jev_revised_tests assets/prototype OUTPUT --live`. It makes one HTTP request, then verifies a local cache hit. Credentials remain external and are not bundled.

## Still not original-game equivalence

The screenshot-matched turn-2 opening, full HP initialization, persistent fire tiles, weather, full opening story/tutorial, later campaigns, reward cash/equipment levels, equipment changes/promotions, some learned tactics, exact original AI, duel/composite spell presentation and campaign music are still limits. Original `.e5s` import/export and account/cloud features remain outside this native prototype. No full-campaign or original acceptance completion is claimed.
