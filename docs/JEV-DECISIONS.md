# Jev investigation decisions

Jev is an advisory tool for focused research questions. The original executable,
deterministic fixtures and platform tests remain the acceptance authority. No
Jev response becomes game logic without executable verification. Credentials
are read from the user-supplied external credential directory and are never
included in requests, source, evidence or game packages.

The client is `tools/jev_decide.py`. Exact non-secret input, responses, gateway
model/provider metadata, usage and elapsed time are retained in `evidence/jev/`.
Records are append-only; conclusions are recorded here separately. The gateway
model alias is `typesafe-ai/jev`; returned metadata is retained because this
alias is not an immutable model revision.

As of 2026-10-05: **64 API requests,62 successful evaluations,2 HTTP400 responses**.
The latest request (064) cost a gateway-reported **0.000036624 USD**.
No development-time saving has been measured.

## 2026-10-02: area status command investigation

- **Issue delegated:** Prioritize the next investigation at `S_00.eex:0x282c`
  and identify whether existing recovery fixtures cover the confusion animation.
- **Context supplied:** Observed opcode77 area scan, presentation-before-mutation
  call, MEFF48 selection, existing MEFF49/MCALL tests and the next script steps.
- **First request:** `001-status-investigation` failed with HTTP400 because the
  client used `options` instead of the documented `criteria`. This is an
  integration mistake, not a Jev judgment. The client now rejects that shape
  locally. No credential or response body was logged for the failed request.
- **Successful request:** `002-status-investigation` selected `status_probe`
  (probability1, confidence1) and rated the presentation coverage gap0.94.
  Elapsed0.622 seconds; gateway-reported cost0.000037002 USD. The gateway reports
  the Jev model served by `digitalocean`; routing metadata is preserved.
- **Decision taken:** Verify the area selector, mutation order and MEFF48 player
  before admitting this instruction. This confirms the proposed plan; it does
  not establish new executable semantics or a measured speed improvement.
- **Verification:** The original sequential handler now completes in `tools/status_event_probe.py`; `original_status_event.json` records 13 targets and 949 host-boundary requests. Its allowed hooks are explicit. `animation_tests` compares all 169 frames, mutation order and final units; `core_tests` checks the integrated state before every status mutation. This is bounded original-code evidence, not a Windows playthrough.

API contract: [Vercel evaluation API](https://vercel.com/docs/ai-gateway/modalities/evaluation).

First records: [failed request](evidence/jev/001-status-investigation.request.json),
[failed response](evidence/jev/001-status-investigation.response.json),
[successful request](evidence/jev/002-status-investigation.request.json),
[successful response](evidence/jev/002-status-investigation.response.json).

## 2026-10-02: confusion test coverage (003)

- **Issue delegated:** Prioritize independent validation of MEFF48's unusual
  zero-weight/zero-amount frames, and assess the proposed area/status coverage.
- **Record:** [request](evidence/jev/003-confusion-coverage.request.json),
  [response](evidence/jev/003-confusion-coverage.record.json).
- **Recommendation:** `zero_blend`, probability 0.93, confidence 0.89;
  proposed coverage sufficiency probability 0.75. These are model outputs,
  not measured correctness rates. Elapsed 0.600 seconds; reported cost
  0.000029022 USD. DigitalOcean returned 503; the gateway successfully routed
  the same Jev model to TypeSafe. The client did not retry or change models.
- **Decision:** Accepted the probe priority. The sufficiency score does not
  replace the acceptance tests.
- **Result:** `confusion_probe.py` captures 604 selector/status cases and
  327,680 original compositor palette pairs. Disassembly and the sequential
  raster trace reveal an additional distinction: weight=0/palette=255 bypasses
  the quantizing compositor and draws the original indices transparently.
  The engine now preserves that path for 39 of the 169 frames. Thus the
  investigation found something that the model's recommendation alone did
  not establish.
- **Limits:** Host raster boundaries use a deterministic indexed background;
  full original-window visual/timing equivalence remains unverified.

## 2026-10-02: next acceptance task (004)

- **Issue delegated:** Pick the next implementation dependency after validating
  confusion, the six AI instructions and 37 arrival-event handlers.
- **Record:** [request](evidence/jev/004-next-acceptance-task.request.json),
  [response](evidence/jev/004-next-acceptance-task.record.json).
- **Recommendation:** `objective_and_tutorial`, probability 1, confidence 1.
  Elapsed 0.703 seconds; reported cost 0.000030366 USD; TypeSafe provider.
- **Decision:** Accepted as the next investigation: objective banner at
  `S_00.eex:0x2f13`, target highlights and tutorial through the first real player
  interaction. Then implement those player commands and continue battle/outcomes.
- **Verification/status:** Scheduling advice only. The original dispatch table
  maps opcode26 to `0x413c0e`, which calls the dedicated banner `0x44bd27`.
  The old generic-dialogue placeholder is now an explicit diagnostic. Banner,
  tutorial and player controls are not claimed complete.

Three successful advisory calls have a combined gateway-reported cost of
0.000096390 USD; the initial HTTP400 has no returned usage/cost record. These
calls confirm or prioritize small research decisions. No development-speed
improvement has been measured. No executable, assets, credentials or personal
save data were submitted; exact non-secret contexts are preserved above.


## 2026-10-02: objective screen (005–007)

- **005 — objective assembly review:** [request](evidence/jev/005-objective-banner-analysis.request.json), [response](evidence/jev/005-objective-banner-analysis.record.json). Delegated sweep-axis interpretation and next-probe selection from the original disassembly. Jev chose **columns** (probability 0.67, confidence 0.50); **rejected** because all 48 original-code layouts sweep rows. Accepted its full-presentation probe priority. Cost 0.000354270 USD; elapsed 0.623 seconds.
- **006 — text alignment:** [request](evidence/jev/006-objective-text-alignment.request.json), [response](evidence/jev/006-objective-text-alignment.record.json). Delegated interpretation of the text-call argument and branch. Jev selected left alignment with vertical centering (0.98, confidence 0.97). Accepted after inspecting the original `0x41e9b4` branch; the native renderer now respects it. Cost 0.000025200 USD; elapsed 0.548 seconds.
- **007 — interrupt boundary review:** [request](evidence/jev/007-objective-pause-review.request.json), [response](evidence/jev/007-objective-pause-review.record.json). Delegated test priority for the last 20-tick pause. Accepted the first/last input and cleanup checks (1.0, confidence 1.0). Six original-input cases now verify early dismissal, no-input completion, save rejection, unchanged units/RNG and exactly one cleanup. Cost 0.000024024 USD; elapsed 0.645 seconds.

These calls informed investigation; original execution determined the result.
The incorrect sweep answer is retained, not rewritten as a successful decision.
Selected disassembly excerpts were submitted, never the executable binary.

## 2026-10-02: target highlights (008)

- **Issue delegated:** Review opcode92 and the existing selection helper for a concrete reuse defect; prioritize a lookup edge case.
- **Records:** [request](evidence/jev/008-target-highlight-review.request.json), [response](evidence/jev/008-target-highlight-review.record.json).
- **Recommendation:** `target_sentinel` (0.99, confidence 0.98) and `duplicate_lifecycle` (0.97, confidence 0.96). Accepted.
- **Implementation/evidence:** The old helper attempted to resolve slot255 as a unit. It now clears the target mask. `highlight_probe.py` executes 15 original schedules and 12 mask cases, including first active slot ordering and inactive duplicate IDs. Both scripted targets complete three flashes in the engine; native tests check panel lifetime, unchanged units/RNG and deterministic continuation after restoring the preceding dialogue.
- **Usage:** TypeSafe provider; 2,975 input tokens; elapsed 0.518 seconds; reported cost 0.000124950 USD.

## 2026-10-02: tutorial setup (009)

- **Issue delegated:** Interpret the tutorial setup flags and choose an original-code probe boundary that isolates external collection data.
- **Records:** [request](evidence/jev/009-tutorial-setup-review.request.json), [response](evidence/jev/009-tutorial-setup-review.record.json).
- **Recommendation:** Count nonzero completion bytes0–2 and require every collectible item's byte at item+3 (`count_and_all`, probability0.87/confidence0.83). Supply collection reads explicitly and stop at unexplored teaching/reward routines (`reader_boundary`, probability1/confidence0.99). Accepted.
- **Evidence:** `tutorial_probe.py` captures 68 setups and 32 entry/skip branches. Direct disassembly confirms `0x41a284` reads an indexed byte from `SVCMN.E5S`; the probe supplies bytes and never opens/writes that save. Entry acceptance stops at teaching routine `0x43353c`; decline can finish or lead to completion/treasure rewards. Teaching and reward effects are still unimplemented and are not silently omitted.
- **Usage:** TypeSafe provider; 2,966 input tokens; elapsed0.310 seconds; reported cost0.000124572 USD.

## 2026-10-02: tutorial primitives (010)

- **Issue delegated:** Interpret two original tutorial sprite poses and assess the cursor's map-plane side effect before porting.
- **Records:** [request](evidence/jev/010-tutorial-primitives.request.json), [response](evidence/jev/010-tutorial-primitives.record.json).
- **Recommendation:** Pose interpretation **abstained** (`unknown`, probability0.47/confidence0.30). Cursor cleanup identified as stateful (`plane_cleanup`, probability1/confidence1).
- **Decision/evidence:** Kept the abstention. 192 independent original-code cases establish pose1 frame4 and pose2 frames1/2/0/2, mirroring only direction1. Original cursor routines write3 then255. The engine preserves this mutation and guards unverified collisions. Model advice alone did not determine the frame sequence.
- **Usage:** TypeSafe; 3,914 input tokens; elapsed0.559 seconds; reported cost0.000164388 USD.

## 2026-10-02: tutorial save validation (011)

- **Issue delegated:** Review the proposed continuation validator for a missing invariant and choose a focused reproduction test.
- **Records:** [request](evidence/jev/011-tutorial-save-review.request.json), [response](evidence/jev/011-tutorial-save-review.record.json).
- **Recommendation:** Check extra queued dialogue pages (`queued_pages`, probability0.98/confidence0.97), using a real snapshot with recomputed checksum (0.94/confidence0.92).
- **Decision/evidence:** Accepted. The first native run failed specifically because the mutated snapshot was accepted. Validation now rejects unexpected pending pages and mismatched tutorial speaker/portrait data. All currently ported tutorial speeches independently fit one page. 94 deterministic save continuations and846 malformed-restore cases pass on Linux, macOS and Wasm. Rejected loads leave the live hash unchanged.
- **Usage:** DigitalOcean; 3,616 input tokens; elapsed0.684 seconds; reported cost0.000151872 USD.

## 2026-10-02: second tutorial range caller (012)

- **Issue delegated:** Interpret a compact assembly caller for Liu Bei's movement/attack-range demonstration and select useful original-code probe boundaries.
- **Records:** [request](evidence/jev/012-tutorial-range-caller.request.json), [response](evidence/jev/012-tutorial-range-caller.record.json).
- **Recommendation:** Movement search mode0 with movement-point budget, target255/255 and zone-of-control enabled (probability0.89/confidence0.85). Execute the original search and mask writer and capture complete grids/cleanup (0.98/confidence0.97).
- **Decision/evidence:** Accepted and independently verified by `tutorial_ally_probe.py`: the guide first walks from(9,0) to(10,0) using a separate mode2 search. Liu Bei then performs mode0 search with budget6, target255/255 and ZOC enabled. 39 reachable cells and4 attack-mask cells are visible; cleanup disables movement display and clears the mask while leaving the distance scratch. The probe captures118 events including16 speeches, unchanged RNG and final guide position. The second lesson is **not yet ported** into native gameplay.
- **Limits:** The previously hooked UI mask buffer is seeded clear at this boundary; original search, guide walking and mask writes run unhooked, while documented host raster/window/audio functions remain hooks. No full original-window comparison is claimed.
- **Usage:** TypeSafe; 2,372 input tokens; elapsed0.385 seconds; reported cost0.000099624 USD.

## 2026-10-02: second-lesson snapshot continuation (013)

- **Issue delegated:** Review saved cursor origin and range-cleanup invariants when the guide begins moving independently of the tutorial cursor.
- **Records:** [request](evidence/jev/013-tutorial-continuation-review.request.json), [response](evidence/jev/013-tutorial-continuation-review.record.json).
- **Recommendation:** Store the original cursor origin and validate guide movement separately (`origin_and_moves`, probability1/confidence0.99). Retain distance data when dismissing the overlay (`retain_distance`, probability0.99/confidence0.99).
- **Decision/evidence:** Accepted. Schema5 stores the origin before the guide walks. The original second lesson leaves guide(10,0), cursor(9,6), Liu Bei(9,5); range dismissal disables movement and clears the attack mask while retaining distance data. Eight integrated paths compare208 original interaction states and208 save continuations. Mutated origin, grids, branch state and other malformed fields are rejected. A Linux active-range snapshot restores unchanged on macOS and Wasm.
- **Usage:** TypeSafe;813 input tokens; elapsed0.719 seconds; reported cost0.000034146 USD.

## 2026-10-02: third-lesson movement cancellation (014)

- **Issue delegated:** Determine whether this original tutorial cancellation restores the whole pre-walk unit or only relocation state, and choose an independent probe.
- **Records:** [request](evidence/jev/014-tutorial-cancellation.request.json), [response](evidence/jev/014-tutorial-cancellation.record.json).
- **Recommendation:** The active lifecycle2 path changes coordinates/occupancy and redraws; do not restore previous facing/flags (`coordinates_only`, probability0.97/confidence0.95). Compare full unit/persistent bytes, occupancy and RNG (0.96/confidence0.94).
- **Decision/evidence:** Accepted after original execution. `tutorial_actions_probe.py` captures154 third-lesson events through the command-menu boundary. The demonstrated cancellation changes Cao Cao's y0→6 and the occupancy cells10/130; other live/persistent fields and RNG are unchanged. Twenty-four additional original-code cases vary direction, flags and the early-return argument, checking all live/persistent bytes, occupancy and RNG. The third lesson and actual player cancellation are **not yet implemented**; caller behavior beyond this tutorial remains to verify.
- **Usage:**2,144 input tokens; elapsed0.481 seconds; reported cost0.000090048 USD. The gateway received DigitalOcean503 and routed successfully to TypeSafe; this was one client API request.

## How Jev is being used

Prefer a bounded branch, calling convention, conflicting hypothesis or proposed
port for review. Include an insufficient-evidence choice. Follow useful answers
with an independent original-code probe; retain disagreements. Large assembly
dumps are less useful: call005 was wrong about sweep direction. Use compact
slices and explicit caller/callee arguments, then ask for a test that would
separate alternatives. No measured development-speed improvement is claimed.

**Cumulative: 50 API requests, 49 successful Jev evaluations, 1 HTTP400.**
The successful calls total **0.028167594 USD** in gateway-reported cost.
Gateway provider fallbacks are retained in records and are not additional client
requests. The model is used only during development, never during gameplay.

## 2026-10-02: blue overlay partial-register review (015)

- **Issue delegated:** Interpret `neg/sbb`, `and AL,0xe2`, and `add EAX,0x5a` in the original movement checker, and choose a discriminating raster probe.
- **Records:** [request](evidence/jev/015-blue-overlay-registers.request.json), [response](evidence/jev/015-blue-overlay-registers.record.json).
- **Recommendation:** Blue palette60, green90 (probability0.82/confidence0.76); independently vary both flags and layer conditions (0.88/confidence0.84).
- **Decision/evidence:** Accepted after executing1,536 original tile-painter cases. Blue takes precedence when both flags are set; terrain cursor draws after movement shading/range marks and before the selection cursor/unit. The native helper and renderer now preserve this ordering. The old renderer incorrectly shaded over terrain marks.
- **Usage:** TypeSafe;664 input tokens; elapsed0.615 seconds; reported cost0.000027888 USD. This call was made during the preceding investigation and is now reconciled into this log; its original request/response were retained at call time.

## 2026-10-02: tutorial command-menu enablement (016)

- **Issue delegated:** Review original menu reset, tutorial setters, initialization and callback sequence; distinguish demonstration input from executing a battle action.
- **Records:** [request](evidence/jev/016-tutorial-menu-enable.request.json), [response](evidence/jev/016-tutorial-menu-enable.record.json).
- **Recommendation:** Only 策略/待命/取消 enabled, and selection closes the demonstration without performing gameplay (probability0.74/confidence0.64). Execute callbacks at Win32 host boundaries (0.93/confidence0.89).
- **Decision/evidence:** Accepted after `tutorial_menu_probe.py` executes16 enablement configurations and six command IDs plus cancellation. Initialization always enables standby. `tutorial_menu_event_probe.py` executes all three enabled responses and both subsequent continuation choices; all six paths preserve game state at menu dismissal and continue through the original caller. Native input now uses the same enabled results and tutorial sequence.
- **Limits:** The probe supplies host window dimensions and bypasses the Win32 message pump explicitly. Original font metrics, button decorations, dragging and real-time input remain unverified; the SDL menu appearance is provisional.
- **Usage:** DigitalOcean;764 input tokens; elapsed0.536 seconds; reported cost0.000032088 USD.

## 2026-10-02: command-menu signed positioning (017)

- **Issue delegated:** Review pixel-versus-tile camera coordinates and signed halving in popup placement, and choose edge cases beyond normal font dimensions.
- **Records:** [request](evidence/jev/017-command-menu-position.request.json), [response](evidence/jev/017-command-menu-position.record.json).
- **Recommendation:** Signed pixel coordinates with truncation toward zero (probability0.66/confidence0.55); include odd/small/oversized windows (0.55/confidence0.33).
- **Decision/evidence:** Independently executed180 original layouts with supplied window sizes/client origins. The portable helper matches positions, including sentinel coordinates, both horizontal branches, negative intermediate positions and taller-than-viewport windows. These tests verify the placement algorithm, not the original font-dependent window size.
- **Usage:** TypeSafe;743 input tokens; elapsed0.476 seconds; reported cost0.000031206 USD.

## 2026-10-02: terrain panel semantics (018)

- **Issue delegated:** Interpret the terrain popup's unusual vertical placement and its grayscale/colored tactic-icon sequence.
- **Records:** [request](evidence/jev/018-terrain-panel-semantics.request.json), [response](evidence/jev/018-terrain-panel-semantics.record.json).
- **Recommendation:** Abstained on placement (`unknown`, probability0.61/confidence0.42). Recommended original palette grayscale followed by colored eligible icons (probability0.95/confidence0.93).
- **Decision/evidence:** Independent execution of138 original panel cases establishes the width-based threshold6 for vertical placement; the model did not establish it. All256 grayscale indices,30 terrain predicates and original indexed icon/tile pixels match the C++ panel tests. Imported colored/grayscale bitmap hashes also match.
- **Limits:** Clean, unoccupied tutorial terrain preview; not arbitrary occupied/effect-covered terrain. Original window composition/font pixels remain unverified.
- **Usage:** TypeSafe;837 input tokens; elapsed0.497 seconds; reported cost0.000035154 USD.

## 2026-10-02: terrain cursor reference boundary (019)

- **Issue delegated:** Review a reference-probe discrepancy after camera movement and choose whether to run the original camera routine or change the port to match the stub's stale cursor.
- **Records:** [request](evidence/jev/019-terrain-cursor-boundary.request.json), [response](evidence/jev/019-terrain-cursor-boundary.record.json).
- **Recommendation:** Execute the original scroll routine (`run_original_scroll`, probability0.99/confidence0.98); its unconditional cursor clear also applies when the destination is unchanged (probability0.97/confidence0.96).
- **Decision/evidence:** Accepted after original execution. `tutorial_cursor_probe.py` verifies12 unchanged/changed camera cases. The sequential terrain probe now runs454cef/45461c instead of the inherited scroll stub. It also reveals that speaker focus clears selection even with no camera displacement. Native state now separates the retained panel background from current cursor state. The complete fresh-profile tutorial has994 original interaction comparisons,994 deterministic save continuations and17,066 rejected malformed restores. All17 Linux suites pass.
- **Limits:** This call reviewed an already identified suspicious stub and supplied assembly; it did not independently discover the discrepancy. No measured time saving is claimed.
- **Usage:** TypeSafe;723 input tokens; elapsed0.585 seconds; reported cost0.000030366 USD.


## 2026-10-03: executable feature/function map (020–048)

The user requested a plain-text map of `Ekd5.exe` function/features by binary
offset, with substantial JEV assistance. The result is
[the UTF-8 binary map](../output/reverse-engineering/Ekd5-binary-feature-map.txt).
This documentation task does not advance the gameplay acceptance checklist.

- **Issues delegated:** Review219 draft entry labels/scopes in22 batches; reconsider13 exact contradictions/narrowings; review46 additional static labels for save/load, movies, resource loading, rewards, credits, startup and UI; audit7 address/boundary/reporting decisions.
- **Method:** Sent bounded original assembly, probe-source snippets, literals and proposed labels. JEV returned structured choices; it did not independently disassemble, execute, or navigate the EXE. Broad approvals were not treated as proof.
- **Independent corrections:** Swapped critical/double labels; class-family versus equipment initialization; formatter position arithmetic; tactic/standby menu setters; initialization versus cancel callbacks; sound-stop helper; tactic wrapper versus planner. `4259ac` and `44d8a0` are internal blocks, excluded from callable-function candidates.
- **Model limits observed:** The initial broad review approved several incorrect labels. Focused042 agreed with all13 supplied corrections, which is advisory agreement after independent identification.048 abstained on file-offset/BSS arithmetic; PE section mapping and source-byte assertions decide those facts. Review warnings on static/host-dependent labels were retained or narrowed, not promoted to tested gameplay.
- **Records/dispositions:** Every question and analyst disposition is preserved in [review-adjudication.json](evidence/exe-map/review-adjudication.json). Exact original requests and responses remain immutable under `docs/evidence/jev/`. No credentials or original user saves were sent.
- **Verification:** SHA-gated read-only disassembly; source/file/VA byte mapping; every visited instruction block; PE resource hashes; import slots; known internal regions; all final named entries have review records. Existing gameplay probes supplied evidence but were not all rerun for this documentation request. No timing benefit is measured.
- **Usage this task:** 29 API requests, all successful; 285 structured decisions; reported cost **0.026388600 USD**.

| Call | Delegated entry/address scope | Decisions | Reported USD | Record |
|---|---|---|---:|---|
| 020 | 10 entries (004014f8–00403b8f, non-contiguous) | supported_scoped:10 | 0.000826728 | [response](evidence/jev/020-exe-map-01.record.json) |
| 021 | 10 entries (00403d85–0040639a, non-contiguous) | supported_scoped:10 | 0.001176336 | [response](evidence/jev/021-exe-map-02.record.json) |
| 022 | 10 entries (004064f1–004082d6, non-contiguous) | supported_scoped:10 | 0.00099876 | [response](evidence/jev/022-exe-map-03.record.json) |
| 023 | 10 entries (004083ef–0040e499, non-contiguous) | supported_scoped:10 | 0.001092084 | [response](evidence/jev/023-exe-map-04.record.json) |
| 024 | 10 entries (0040eb54–00412230, non-contiguous) | too_broad:1, supported_scoped:9 | 0.000824712 | [response](evidence/jev/024-exe-map-05.record.json) |
| 025 | 10 entries (004122f1–004142b2, non-contiguous) | supported_scoped:9, too_broad:1 | 0.001069866 | [response](evidence/jev/025-exe-map-06.record.json) |
| 026 | 10 entries (004146d4–00416129, non-contiguous) | supported_scoped:10 | 0.00098553 | [response](evidence/jev/026-exe-map-07.record.json) |
| 027 | 10 entries (00416319–0041dc43, non-contiguous) | supported_scoped:10 | 0.000876456 | [response](evidence/jev/027-exe-map-08.record.json) |
| 028 | 10 entries (0041fb35–004224a8, non-contiguous) | supported_scoped:10 | 0.001029294 | [response](evidence/jev/028-exe-map-09.record.json) |
| 029 | 10 entries (00422f0d–0042fb10, non-contiguous) | supported_scoped:9, too_broad:1 | 0.000945882 | [response](evidence/jev/029-exe-map-10.record.json) |
| 030 | 10 entries (0042fd5a–00433bc8, non-contiguous) | supported_scoped:9, too_broad:1 | 0.000925722 | [response](evidence/jev/030-exe-map-11.record.json) |
| 031 | 10 entries (0043419f–004351da, non-contiguous) | supported_scoped:10 | 0.00087171 | [response](evidence/jev/031-exe-map-12.record.json) |
| 032 | 10 entries (00435829–0043a28a, non-contiguous) | supported_scoped:10 | 0.001265922 | [response](evidence/jev/032-exe-map-13.record.json) |
| 033 | 10 entries (0043a956–0043dc99, non-contiguous) | supported_scoped:9, too_broad:1 | 0.00129717 | [response](evidence/jev/033-exe-map-14.record.json) |
| 034 | 10 entries (0043e572–004404bd, non-contiguous) | supported_scoped:10 | 0.000915264 | [response](evidence/jev/034-exe-map-15.record.json) |
| 035 | 10 entries (00441488–0044c601, non-contiguous) | supported_scoped:10 | 0.00116151 | [response](evidence/jev/035-exe-map-16.record.json) |
| 036 | 10 entries (0044cc85–00450984, non-contiguous) | supported_scoped:9, too_broad:1 | 0.00119847 | [response](evidence/jev/036-exe-map-17.record.json) |
| 037 | 10 entries (0045137c–00453c17, non-contiguous) | supported_scoped:10 | 0.000869946 | [response](evidence/jev/037-exe-map-18.record.json) |
| 038 | 10 entries (00453e32–00455c3a, non-contiguous) | supported_scoped:10 | 0.00110943 | [response](evidence/jev/038-exe-map-19.record.json) |
| 039 | 10 entries (00455e44–0045a80f, non-contiguous) | too_broad:3, supported_scoped:7 | 0.001001154 | [response](evidence/jev/039-exe-map-20.record.json) |
| 040 | 10 entries (0045aa5c–0047467a, non-contiguous) | supported_scoped:9, too_broad:1 | 0.000719166 | [response](evidence/jev/040-exe-map-21.record.json) |
| 041 | 9 entries (00474a07–004817e0, non-contiguous) | supported_scoped:9 | 0.000611646 | [response](evidence/jev/041-exe-map-22.record.json) |
| 042 | 13 entries (00405ce6–0047fad0, non-contiguous) | corrected:13 | 0.000808248 | [response](evidence/jev/042-exe-map-corrections.record.json) |
| 043 | 10 entries (0040971d–004143c2, non-contiguous) | supported_scoped:10 | 0.000537306 | [response](evidence/jev/043-exe-map-43.record.json) |
| 044 | 10 entries (00417ed2–0041f072, non-contiguous) | supported_scoped:10 | 0.00083496 | [response](evidence/jev/044-exe-map-44.record.json) |
| 045 | 10 entries (0041f13b–0043dc21, non-contiguous) | supported_scoped:10 | 0.000909804 | [response](evidence/jev/045-exe-map-45.record.json) |
| 046 | 10 entries (00449cec–00475264, non-contiguous) | supported_scoped:9, too_broad:1 | 0.000904344 | [response](evidence/jev/046-exe-map-46.record.json) |
| 047 | 6 entries (0047631d–00483c40, non-contiguous) | supported_scoped:6 | 0.000555324 | [response](evidence/jev/047-exe-map-47.record.json) |
| 048 | 7 PE mapping/evidence-method questions | unknown:2, candidate_only:1, imports_only:1, advisory:1, allowlists:1, qualified:1 | 0.000065856 | [response](evidence/jev/048-exe-map-method.record.json) |


## 2026-10-03: game state dictionary (049–050)

- **Issue delegated049:** Review field interpretations with conflicting representations: opening side codes, completion count versus input mask, mixed signed/unsigned damage, allocated map-buffer addresses, pixel camera versus tile cursor, and preservation of unknown bytes/bits.
- **Records049:** [request](evidence/jev/049-state-dictionary-semantics.request.json), [response](evidence/jev/049-state-dictionary-semantics.record.json).
- **Recommendation049:** Keep side0/player,1/ally,2/enemy scoped to observed records; completion count; raw damage plus signed view; resolve map allocation/capacity; preserve distinct coordinate units and unknown data. All six choices supported these interpretations.
- **Independent evidence:** Original tutorial setup with completion bytes[1,0,1] produces count2 and guide slot25. Original movement cancellation changes only live y and two occupancy cells. Original strike plan matches the retained first fixture including RNG; original RNG wraparound matches. Tests check relocated/null/short buffers, unknown-byte/flag changes, and side labels from26 original live records. Corrections to draft side/stat labels were made during source inspection before sending the review.
- **Issue delegated050:** Choose regression checks and evidence/scope limits after replacing shared probe decoders with dictionary-backed adapters.
- **Records050:** [request](evidence/jev/050-state-dictionary-validation.request.json), [response](evidence/jev/050-state-dictionary-validation.record.json).
- **Recommendation050:** Regenerate focused initialization/strike fixtures; call captures observations rather than replay checkpoints; reject differing record selections; retain evidence limits. These four recommendations were accepted.
- **Decision/verification:** Full regenerated initialization and strike fixtures exactly match retained JSON:3,104 growth,5,300 equipment,7 averages,80 joins;1,297 strike plans,392 chance cases and1 original fault. The focused suite now has13 passing tests. Frozen pre-migration byte decoders remain in tests to avoid circular validation against the same dictionary offsets. See [dictionary](GAME-STATE-DICTIONARY.md) and [regression record](evidence/game-state/regression.json).
- **Limits:** Diagnostic tooling only; no new gameplay acceptance gate or resumable checkpoint. JEV reviewed supplied evidence, not the entire executable; no measured speedup claim.
- **Usage049:** HTTP200; elapsed0.728 seconds; reported cost0.000298116 USD.
- **Usage050:** HTTP200; elapsed0.34 seconds; reported cost0.000034692 USD.

## 2026-10-04: Cerberus guide exit and player-loop limits (051–052)

- **Issue delegated051:** Distinguish cropping from resampling, mode1 guide retirement from mode0 defeat, and the next verification boundary. Supplied four Cerberus/Ghidra decompilations, dictionary offsets and202 bounded original source-frame observations.
- **Records051:** [request](evidence/jev/051-cerberus-disappearance.request.json), [response](evidence/jev/051-cerberus-disappearance.record.json).
- **Recommendation051:** Abstained on all three questions (`unknown`; confidences0.39,0.36,0.59). No semantic conclusion is attributed to this response.
- **Independent disposition:** Executed original45137c/430a07 down to the lower raster boundary; expanded to208 cases and2678 frame comparisons, each with source and masked pixel hashes. These establish top-aligned crops and the required static terrain masks. The sequential original tutorial exit and portable tests establish cleanup/state/order for the captured path. Animated terrain13 and other removal modes remain outside the new implementation.
- **Usage051:** HTTP200;7791 input tokens; elapsed0.734 seconds; reported cost0.000327222 USD.
- **Issue delegated052:** Interpret Ghidra's truncated0x44e0d9 player gate at the ASave patch jump and choose the next investigation method.
- **Records052:** [request](evidence/jev/052-cerberus-player-loop-limit.request.json), [response](evidence/jev/052-cerberus-player-loop-limit.record.json).
- **Recommendation052:** Mark the view incomplete (`partial`, probability1/confidence1); use a separately annotated analysis view and original input probes (`annotate_and_probe`, probability0.96/confidence0.94).
- **Disposition:** Accepted the incompleteness classification and investigation direction. Preserved the raw dossier and marked the binary map/report accordingly. No Ghidra flow override has been performed yet. No player commands were inferred from the apparent `ASave(); return;` stub.
- **Corrections to the immutable052 packet:** It says811 gate cases; the actual retained fixture contains271. It also predicted player input as the immediate next guard; integrated execution instead stops earlier at turn-two poison presentation (`0x450984`,11-HP loss). These corrections are documented here without rewriting the original request/response. Neither changes the observed ASave decompiler limitation.
- **Usage052:** HTTP200;1380 input tokens; elapsed0.418 seconds; reported cost0.00005796 USD.
- **Limits:** JEV reviewed supplied evidence. It did not operate Ghidra, execute the original game or independently recover the input loop. See [Cerberus workflow](CERBERUS-RE.md) for executable evidence and the next unresolved boundary.

## 2026-10-04: poison maintenance and host-origin comparison (053–054)

- **Issue053:** Review the no-opponent255 path, display-before-HP-write order, and separation of recovery RNG from poison. Supplied Cerberus dossiers for44df61/450984 and the proposed original-code checks.
- **Records053:** [request](evidence/jev/053-poison-maintenance-order.request.json), [response](evidence/jev/053-poison-maintenance-order.record.json).
- **Advice053:** Display before HP mutation (confidence0.79); optional absent opponent (0.92); compare recovery separately from RNG-free poison (0.33). These are advisory interpretations, independently verified by original execution.
- **Evidence/disposition:** All272 NPC panel cases match the portable helper, including240 new no-opponent cases; the original32 cases remain identical. The complete sequential phase advances RNG340946258→601077795 during recovery, then displays44→33HP for ally210 and writes HP afterward. Poison and regeneration leave RNG601077795. The integrated test checks all six display pauses, pre-mutation HP, final unit/terrain/camera/RNG state and deterministic restoration.
- **Packet correction:** The immutable053 input says2613 existing arithmetic cases; the fixture actually contains1913. This descriptive count is corrected here; it was not used to implement a rule. Future packets should derive counts directly from fixtures.
- **Usage053:** HTTP200;4187 input tokens;0.687 seconds; reported0.000175854 USD.
- **Issue054:** Adjudicate a56-pixel Y-coordinate discrepancy between the sequential original panel and the native panel without concealing different host inputs.
- **Records054:** [request](evidence/jev/054-maintenance-host-origin.request.json), [response](evidence/jev/054-maintenance-host-origin.record.json).
- **Advice054:** Explicitly supply and record the same host viewport origin (confidence1); keep client origin distinct from world camera coordinates (0.99).
- **Evidence/disposition:** Original getters451a4f/451a60 read camera+0x3c/+0x40, already named client_x/client_y in the dictionary. The headless prefix never initializes a Windows client and inherited0,0. The sequential fixture now explicitly supplies0,56, matching isolated panel fixtures, records both origins, and preserves world camera. All native sequential panel operations then match. This establishes behavior under the supplied viewport, not original Windows layout equivalence. Dictionary1.0.1 records this distinction.
- **Usage054:** HTTP200;773 input tokens;0.355 seconds; reported0.000032466 USD.
- **Limits:** JEV neither executed the program nor established the correct original Windows client dimensions. No measured time saving; no player actions or complete battle claimed.

## 2026-10-04: original player input and movement controller (055–056)

- **Issue055:** Interpret the original continuation after the ASave transfer, retained hover selection, and the action-return/event-scan contract. Supplied the incomplete Cerberus44e0d9 dossier and unmodified continuation assembly.
- **Records055:** [request](evidence/jev/055-player-input-continuation.request.json), [response](evidence/jev/055-player-input-continuation.record.json).
- **Advice055:** Abstained on retained selection (confidence0.47); action result controls subsequent event/death checks (0.50); prioritize ordered input traces over treating the decompiler stub as complete (0.99).
- **Disposition055:** Investigation direction accepted. Actual outer-loop hover/action-return behavior remains unverified by execution; no native outer-loop implementation is justified by these suggestions. Cerberus43dde3 exposed the movement-controller dependency43d538, now being verified first.
- **Packet correction055:** Its dictionary context incorrectly calls camera+0x2c/+0x30 the world coordinates. The actual dictionary and original getters use camera+0x04/+0x08; client origin is+0x3c/+0x40. The incorrect offsets were not used by the new probe or C++ port. The immutable request is preserved.
- **Usage055:** HTTP200;8449 input tokens;0.647seconds; reported0.000354858USD.
- **Issue056:** Review invalid-destination retry, moved versus action-complete flags, and an explicit begin/pick state interface. Supplied the Cerberus43d538 decompile, original getter/branch assembly and six original movement cases.
- **Records056:** [request](evidence/jev/056-player-movement-state.request.json), [response](evidence/jev/056-player-movement-state.record.json).
- **Advice056:** Retry invalid destinations (0.64); movement sets flag4 and proceeds to the action menu separately from flag2 completion (0.73); preserve an explicit selection state and integrate after caller contracts are checked (0.86).
- **Independent evidence056:** Expanded original probe executes502 cases, including all400 map destinations for explicitly activated CaoCao. Original own-tile choice returns1 without movement; valid traversal sets flag4; invalid destinations emit original Chinese messages and permit another picker response. The portable begin/pick controller matches all502 cases on macOS, Linux and WebAssembly, including picker grids, final state and ordered requests.
- **Probe correction:** Ghidra's inferred local label local_8 is not the machine stack offset. Assembly places the controller mode atEBP-4; the first draft observer readEBP-8. The corrected502-case fixture records the real mode. Full registers passed for narrow range arguments also contained irrelevant high bits; requests now preserve their declared16/8-bit argument widths. These corrections are explicit in the056 packet.
- **Limits:** Tile-picker results and presentation boundaries are supplied/captured. No Windows input loop, full battle, browser UI or measured speedup claim.
- **Usage056:** HTTP200;5237 input tokens;0.59seconds; reported0.000219954USD.


## 2026-10-04: player menu, outer loop and combat boundaries (057–059)

- **Issue057:** Review command availability, cancellation orientation and wait completion from Cerberus43dde3/43e273 plus original movement evidence.
- **Records057:** [request](evidence/jev/057-player-action-handoff.request.json), [response](evidence/jev/057-player-action-handoff.record.json).
- **Advice057:** Conditional mirror restoration (confidence0.83); wait completes flags2/4 (0.59); class/level-based menu availability without MP/silence checks at this layer (0.80).
- **Verification057:** Accepted after1106 original-code cases matched C++ on macOS/Linux/Wasm, including full range grids and ordered targets. Cancellation restores the old tile/direction and clears moved4; east/west force mirror state, north/south preserve the current mirror. The17 stock bytes are tested independently. Neighbor borrowing covers ally/enemy, zero/nonzero HP, all four directions, with/without equipment effect51. This establishes availability, not tactic castability or item use.
- **Usage057:** HTTP200;3607 input tokens;0.55seconds; gateway cost0.000151494USD.
- **Issue058:** Interpret retained hover, cancellation's action-event gate, the post-completion candidate filter, and useful input-sequence tests. Supplied the unchanged continuation assembly and verified action results.
- **Records058:** [request](evidence/jev/058-player-loop-contract.request.json), [response](evidence/jev/058-player-loop-contract.record.json).
- **Advice058:** Conditional retained hover (0.48); abstained on cancellation/event gating (0.16) and remaining-unit filtering (0.64); prioritize sequence-edge probes (0.91).
- **Independent disposition058:** The new original loop executes actual43dde3 movement/menu/wait/cancel. Cancellation still invokes45b5fe, which itself invokes an event batch in these cases. A completed action additionally invokes death_scan then event_batch. The remaining-unit scan excludes completed2 and confused8, but does not exclude flag128; this differs from the initial player gate. These are execution findings, not conclusions supplied by JEV. Hover is retained outside only with a nonzero retention gate, tileY255 and main-window focus. Input/window/timer states are explicit supplied boundaries; no physical mouse or continuous turn-two claim.
- **Probe repairs058:** First run stopped at unlisted42c4cb. Inspection showed an eight-tick wait wrapper; its instructions now execute against the established timer boundary. A draft host command wroteCTRL+0 instead ofCTRL+1; getter44f9a0/setter40be20 established the correct byte before retained fixtures were generated. No original game bytes were patched.
- **Usage058:** HTTP200;7912 input tokens;0.545seconds; gateway cost0.000332304USD.
- **Issue059:** Identify bounded next experiments for player attack and tactic callbacks, using newly exported Cerberus43dada/43dc21. Existing NPC attack arithmetic does not establish party XP/HP result behavior.
- **Records059:** [request](evidence/jev/059-player-combat-boundaries.request.json), [response](evidence/jev/059-player-combat-boundaries.record.json).
- **Advice059:** A255 target skips the shown completion tail, but setup43f917 still needs checking (0.26); conditional second-attack branch needs callee/scratch/RNG verification (0.77); compose original callback and stop at unsupported party-result boundaries (0.87).
- **Disposition059:** Accepted as an investigation plan only. No new player attack/tactic execution or completion claim. Next probes must execute original setup, target selection boundary and combat continuation; unresolved0x472c90/0x43f917 callees retain unknown semantics until verified. Do not omit party results to force progress.
- **Usage059:** HTTP200;1800 input tokens;0.589seconds; gateway cost0.0000756USD.
- **Limits:** JEV did not operate Ghidra or execute the game. No measured development speedup. Frontend/player snapshot integration and the full acceptance checklist remain incomplete.

## 2026-10-05: screenshot layout and older prototype saves (060)

- **Delegated issue:** Review ruleset separation for restoring old snapshots after
  changing initial roster/terrain; assess how far screenshot-plus-save evidence
  supports compatibility claims. No artwork or unknown binary behavior delegated.
- **Records:** [request](evidence/jev/060-prototype-layout-save-compatibility.request.json),
  [response](evidence/jev/060-prototype-layout-save-compatibility.record.json).
- **Advice:** Separate old/new rulesets with transactional restore (confidence
  0.96); limit evidence to this captured layout, not complete gameplay (0.99).
- **Disposition:** Accepted. Schema1 restores the old baseline and schema2 uses
  the exact slot layout. Both validate in a temporary candidate before committing.
  The original old snapshot reproduces its exact historical next-attack hash;
  invalid cross-ruleset loads preserve state. The new full battle passes 365
  save/reload checkpoints; original source/save SHA checks remain unchanged.
- **Usage:** HTTP200, 660 input tokens, 0.862 seconds, reported $0.00002772.
  Total historical calls:60 (59 successful, one earlier HTTP400).
- **Limits:** Advisory review only; no JEV execution of the game or independent
  binary decoding. No measured time saving or full gameplay-equivalence claim.

## 2026-10-05: prototype attack playback and saves (061)

- **Delegated issue:** Review the presentation/save contract for reusing the
  recovered strike sequence without changing simplified damage, RNG or snapshots.
  Assess whether verified frame sequences justify timing/equivalence claims.
- **Records:** [request](evidence/jev/061-prototype-attack-playback.request.json),
  [response](evidence/jev/061-prototype-attack-playback.record.json).
- **Advice:** Atomic simulation with a guarded presentation phase (confidence
  0.95); reuse original sequence but label wall-clock pacing/numeric layout
  provisional (0.84).
- **Disposition:** Accepted after local checks. Packets are consumed once; stable
  saves exclude presentation state. Native playback tests cover pause, overlapping
  input, save rejection, valid/corrupt restoration, enemy scheduling and delayed
  defeat. The historical 365-checkpoint trace is unchanged on all three platforms.
- **Packet correction:** The immutable request says267 captured attack cases.
  The actual fixture contains **189 cases / 2,830 indexed frame hashes**, verified
  by the retained replay log. The incorrect descriptive count was not used in
  implementation. No full timing or combat equivalence follows from this review.
- **Usage:** HTTP200;640 input /87 output tokens;0.49seconds; reported$0.00002688.
  Total historical calls:61 (60 successful, one earlier HTTP400).
- **Limits:** No JEV execution of the game; no measured time saving. Original
  executable fixtures and local tests decide correctness.

## 2026-10-05: movement-to-attack queue and saves (062–063)

- **Delegated issue:** Review how original walking events should precede an
  already-committed enemy attack, retain stable snapshots and hold the final
  walking frame; assess evidence limits for the full-map adaptation.
- **Records:** [request062](evidence/jev/062-prototype-walking-queue.request.json),
  [response062](evidence/jev/062-prototype-walking-queue.record.json),
  [request063](evidence/jev/063-prototype-walking-queue.request.json),
  [response063](evidence/jev/063-prototype-walking-queue.record.json).
- **Request correction:** 062 returned HTTP400. Its final-frame question used
  boolean/string criteria. 063 reformulated that question as a choice using the
  previously successful schema and returned HTTP200. Both attempts are retained.
- **Advice:** Queue guarded presentation after atomic simulation (confidence0.98),
  hold the final frame (0.84), and keep equivalence claims bounded (0.95).
- **Disposition:** Accepted with independent checks: all1,749 original walking
  cases pass;499 current/legacy paths check coordinates, friendly overlays,
  cancellation, restoration and queued pre-impact HP. Slow native rendering
  presents all20 frames of the selected five-tile route. The365-checkpoint
  simulation trace is unchanged. Further platform evidence is linked from
  [the walking report](PROTOTYPE-MOVEMENT-ANIMATION.md).
- **Usage:** 062: HTTP400,0.243s, no returned usage/cost. 063: HTTP200,
  807 input /115 output tokens,0.520s, reported$0.000033894.
  Total historical calls:63 (61 successful, two HTTP400 responses).
- **Limits:** JEV reviewed the supplied design, did not execute or independently
  decode the original binary, and supplied no measured time-saving estimate.

## 2026-10-05: original prototype conversations (064)

- **Delegated issue:** Review original encounter trigger boundaries, pending-page
  persistence, single-use flags, older-save migration, and the limits of adding
  dialogue to the simplified victory rules.
- **Records:** [request](evidence/jev/064-prototype-unit-dialogue.request.json),
  [response](evidence/jev/064-prototype-unit-dialogue.record.json).
- **Advice:** Versioned pending-event/page snapshots (confidence0.96), triggers
  after committed actions with animation completed before speech (0.99), and
  bounded equivalence claims (0.98).
- **Disposition:** Accepted after independent checks. Exact imported script
  conditions/text, the Cerberus selector dossier, and159 replayed original-code
  cases establish adjacency/priority. The regenerated fixture exactly matches
  the retained fixture. Production-mode tests cover379 deterministic action/page
  checkpoints, both victory branches, retreat/defeat speech, blocking, migration,
  and corrupt-save rejection. Native rendering tests exercise keyboard/mouse,
  line widths, portraits, paused enemies and fresh-process exact-page recovery.
  [Dialogue report](PROTOTYPE-DIALOGUE.md) records platform evidence and limits.
- **Usage:** HTTP200;872 input /118 output tokens;7.419seconds;
  reported$0.000036624. Total historical calls:64 (62 successful, two HTTP400).
- **Limits:** JEV evaluated the supplied design; it did not execute or independently
  decode Ekd5.exe. Full event bodies, original victory timing and full campaign
  equivalence are not claimed. No development-time saving was measured.

## 2026-10-06: standalone additional battles (065)

- **Delegated issue:** Whether to expand the agreed standalone prototype while
  full campaign parsing remains unresolved, and how to preserve opening saves
  while introducing scenario identity and separate slots.
- **Records:** [request](evidence/jev/065-standalone-battles.request.json),
  [response](evidence/jev/065-standalone-battles.record.json).
- **Advice:** Bounded standalone scenes with explicit omissions; preserve opening
  schemas and add versioned separate saves (both confidence0.99).
- **Disposition:** Adopted with independent original-handler deployment checks,
  command-driven victories, negative restore tests and native renderer checks.
  Subsequent local playtests required the script-supported outside-gate objective
  for 虎牢关 and level20 friendly units for pursuit, with a strategy protecting
  Cao Cao. These later changes are local findings, not JEV recommendations.
- **Usage:** HTTP200;591 input /80 output tokens;0.572seconds;
  reported$0.000024822. Historical total65 calls:63 successful, twoHTTP400.
- **Limits:** Advisory design review only; no original game execution by JEV and
  no measured time savings. [Battle report](PROTOTYPE-ADDITIONAL-BATTLES.md).

## Calls066–070 — native runtime enemy choices, 2026-10-06

User-authorized runtime integration sends current battle state and a bounded
legal action set to JEV. This is gameplay decision delegation, not advice about
original executable semantics. Five HTTP200 evaluations were made during this
release (one per scenario plus an additional 虎牢关 protocol check). Every one
selected a0, the local AI suggestion; these samples do not establish stronger AI.
The extra protocol call occurred while a test used SDL's environment snapshot
instead of the process environment to set a missing key; that test was corrected
to use `setenv`, and subsequent failure/cancellation tests send no request.

| Call | Scenario | Actor | Latency | Provider-reported cost | Record |
|---|---:|---:|---:|---:|---|
| 66 | 2 | 119 | 1106ms | $0.000382998 | [request/response](evidence/balance/jev-log/decision-1791327378982503000.json) |
| 67 | 2 | 119 | 554ms | $0.000382998 | [request/response](evidence/balance/jev-log/decision-1791327441466175000.json) |
| 68 | 0 | 146 | 438ms | $0.000243306 | [request/response](evidence/balance/jev-log/decision-1791327624778830000.json) |
| 69 | 1 | 129 | 604ms | $0.00034629 | [request/response](evidence/balance/jev-log/decision-1791327626481492000.json) |
| 70 | 3 | 119 | 574ms | $0.00033978 | [request/response](evidence/balance/jev-log/decision-1791327681043316000.json) |

Total for this release: **5 live calls**, all successful, provider-reported
cost **$0.001695372**. Known cumulative total at packaging: **70 calls**
(68 successful, two historical HTTP400). Cache hits, local missing-key fallbacks
and cancellation records are logged separately and are not API calls.

The original-code NPC fixture is the independent authority for corrected levels
and equipment. The runtime decisions were validated against C++ legality, replayed
from portable snapshots across macOS/Linux/Wasm, and exercised through native
movement/attack presentation. No arbitrary model commands are executed. See
[implementation and limitations](PROTOTYPE-BALANCE-JEV.md). Future player-session
calls are tracked in the user's save-directory `jev/` logs, not auto-appended to
this source ledger. [Machine-readable release summary](evidence/balance/jev-log/summary.json).

## 2026-10-06 — prototype tactics (3 calls this change)

- **066: legal-action/save contract review.** Delegated the choice of JEV action
  boundary and most important integration check. The model chose typed legal
  choices and deterministic MP/status/pending-choice replay. Implemented and
  independently checked against original-code fixtures and native traces.
  [Request](evidence/jev/066-prototype-tactics.request.json),
  [response](evidence/jev/066-prototype-tactics.record.json).
- **Two live runtime decisions:** the C++ client submitted current scenario/unit
  state, MP, spirit, legal spell targets/costs/estimates and capability limits.
  JEV chose 毒烟 for 李肃 in 汜水关 and 灼热 for 李儒 in 虎牢关. Both responses were
  accepted, executed by C++, and reproduced identically after restoring the
  pending decision. These are isolated legal-contact fixtures; no claim that
  they reproduce the original AI's decisions or a complete battle route.
  [Results](evidence/prototype-tactics/jev-live/verification.json).
  Full append-only request/response records are under
  `evidence/prototype-tactics/jev-live/battle-1/` and `battle-2/`.

The development advisory series is now **066**. Runtime gameplay calls are logged
separately; do not report 66 as a grand total including historical gameplay calls.
No model output supplies damage formulas or mutates HP/MP directly. All calls in
this change succeeded; no retries were made. Credentials remain outside packages.


## 2026-10-06 — terrain healing and location pickups (2 calls this change)

- **067: migration and critical-test review.** Delegated how to add a previously
  absent inventory without resetting old balanced saves, and which interaction
  to test most carefully. JEV selected empty inventory/unclaimed locations with
  no load-time side effects, and move/cancel/save/commit/revisit verification.
  Both were implemented and independently checked against original conditions,
  deterministic engine tests and native input/storage tests. It did not supply
  original healing rates or item placements.
  [Request](evidence/jev/067-terrain-pickups.request.json),
  [response](evidence/jev/067-terrain-pickups.record.json).
- **One live runtime decision:** the C++ client submitted `caocao-terrain-v3`
  state, healing estimates, reserved healing destinations, eight-location
  catalog facts/current-scenario claims, inventory and capability limits. JEV
  chose legal action `a22`: move to (11,9), cast 毒烟 on Cao Cao. It did not choose
  a healing tile in this synthetic wounded-enemy fixture. C++ validated and
  executed the response; saved-choice replay matched result hash
  `16012cefc3917456134511d7870a86c4fd1015fc3efeba2cabc4913817440658`.
  [Request](evidence/terrain-benefits/jev-live/request.json),
  [result](evidence/terrain-benefits/jev-live/verification.json).
  The durable request/response record is under
  `evidence/terrain-benefits/jev-live/runtime/`.

Both calls succeeded, with no retries. The advisory series is now **067**;
that is not a grand total including runtime gameplay. The offline transport
regression used a nonexistent credential path and made no network requests.
Credentials remain external and are not copied to logs, assets or packages.
See [implementation, original-code evidence and limits](PROTOTYPE-TERRAIN.md).

## 2026-10-07 — consumables and inventory (2 calls this change)

- **068 advisory:** delegated the save-contract choice and highest-value regression. JEV selected explicit consumed counts (`remaining + consumed == claimed grants`) and cancel/use/save/restore/revisit coverage. Both were implemented; original EXE probes establish rules independently. [Request](evidence/jev/068-item-use-contract.request.json), [response/disposition](evidence/jev/068-item-use-contract.record.json).
- **One live runtime call:** `caocao-items-v4` sent a collected bean, wounded Cao Cao, current units, legal actions, player item effects/range/action cost, and no enemy access to player inventory. JEV chose `a0`, 毒烟 on Cao Cao from (13,11). C++ validated/executed it; restoring the saved choice produced the same result hash `998e1bfe16cefae4f68f6f10c85e8792e068b8b7ebe8ce305764866158a46667`. [Request](evidence/prototype-items/jev-live/request.json), [verification](evidence/prototype-items/jev-live/verification.json). Complete transport records are under `evidence/prototype-items/jev-live/runtime/`. This verifies the context and replay path, not optimal tactics or original AI equivalence.

Both network calls succeeded without retries. Earlier local integration attempts stopped on a pending dialogue before calling JEV; they made no network requests. The advisory series is **068**, not a grand total of gameplay calls. Credentials remain external and unbundled.

## 069 — Read-only detailed unit window (2026-10-07)

One advisory request, HTTP200. Delegated: choose learned-versus-currently-castable
strategy presentation and identify the highest-value modal regression. JEV chose
all learned entries with implementation/availability labels, and exact snapshot
checks across selection, tentative moves and enemy phase. Accepted as design
advice, independently checked with the68-record original loader fixture,130-unit
data test and native390-tab modal/input smoke. No gameplay fact was inferred
from the model's confidence. No measured time saving is claimed. Request/response:
`docs/evidence/jev/069-unit-details.{request,record}.json`. Runtime JEV unchanged;
inspection defers result application until the read-only modal closes.

## 2026-10-08 — REA gameplay repairs (3 HTTP attempts)

- **070 advisory, HTTP400:** attempted save-contract decision with an invalid `context` key; required `state` was missing. The failure record is preserved: [request](evidence/jev/070-revised-rules-save-contract.request.json), [record](evidence/jev/070-revised-rules-save-contract.record.json). No useful model decision was accepted.
- **071 corrected advisory, HTTP200:** delegated historical-save migration, highest-risk continuation regression, and growth validation. JEV selected preserving old rules, counter/double/level-up plus save continuity, and bounded growth checks. These were implemented and independently tested against edition data and original-code fixtures. [Request](evidence/jev/071-revised-rules-save-contract.request.json), [record](evidence/jev/071-revised-rules-save-contract.record.json). Model confidence is not executable evidence.
- **One runtime HTTP200:** revised-v5 context sent the 15 supported spells, grades/status, legal actions, script progress, counter/critical capabilities and visible units. Actor132 chose 气合; C++ accepted it. Restoring the pending choice reproduced its result. The follow-up was a verified durable-cache hit, not another network request. [Verification](evidence/revised-gameplay/jev/verification-live.json); request/response records are in `evidence/revised-gameplay/jev/live-1791504909592849000/`. HTTP elapsed748ms, cache1ms.

This change therefore made **three HTTP attempts: one failed advisory, one successful advisory, one successful runtime request**. There were no runtime retries; earlier offline fixture failures made no requests. The advisory identifier071 is not a grand total including gameplay calls. Credentials remain external, redacted and unbundled.

## 072 — Equipment inventory and save contract (2026-10-08)

**One advisory HTTP200 request; no runtime calls for this change.** Delegated ownership/save representation and highest-risk regression. JEV selected conservation of acquired gear across bag and player-equipped records, plus swap/restore/campaign validation. Implemented schema10 item triples with conservation, one-time in-memory schema9 migration, preserved gear XP, and campaign bag carryover. The user subsequently specified **only between battles**; that explicit constraint supersedes the request packet's proposed player-turn permission.

The implementation independently uses the recovered equipment class/slot checks and original dialog/data evidence. Automated tests cover40,560 compatibility combinations, corruption rejection, post-battle UI, cross-platform snapshots and deterministic resumed actions. Advice is not binary evidence and no measured hours saved are claimed. [Request](evidence/jev/072-equipment-storage-contract.request.json), [response and disposition](evidence/jev/072-equipment-storage-contract.record.json). Credentials remain external and unbundled.

## 2026-10-09: Cloudflare gameplay integration

The web game delegates enemy action choice to `typesafe-ai/jev` through an authenticated Worker. The key stays in Cloudflare secrets; only state and legal candidate actions go to the model. The live smoke test recorded 5 attempted gateway requests: 3 valid HTTP200 choices and 2 early failures before an HTTP response. Switching fetch redirect mode from error to manual fixed the observed request failure. The slowest successful observed decision was 850 ms. Cached/local results are distinguished from provider calls in D1 `jev_decisions`; raw errors and credentials are never logged. These are runtime gameplay calls, separate from the historical investigation count above. See [audit and verification](WEB-GAME-DEPLOYMENT.md) and [live audit](evidence/web-game/live-jev-audit.json).
