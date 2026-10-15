# Original battlefield conversations — prototype 0.2.5

Updated 2026-10-05 (America/Los_Angeles).

The simplified native prototype now presents original Chinese battlefield
dialogue with original portraits. Move Cao Cao next to Zhang Bao or Zhang Liang
(diagonals count), then attack, wait, or end the turn to commit the action.
The two units must still be alive. A tentative move does not consume an encounter;
cancelling checks the restored position. Each encounter plays once per battle.
Press Space, Return, or click the dialogue box to continue. F5/F9 can save/load
the exact page. Simulation pauses until the pending dialogue is finished.

Save progress in an already running older version, close it, then reopen
`dist/prototype-macos-executable/Start Game.command`. The title must show 0.2.5.
An already running executable does not acquire changes from replacing its file.

## Original evidence

These findings come from this edition's decoded, hash-checked `S_00.eex` and
bounded execution of its original x86 condition/selection routines. This update
does not claim a new live Wine UI observation of every conversation.

| Speech | Original event offset | Original text instruction offset(s) | Trigger / prototype behavior |
| --- | --- | --- | --- |
| Cao Cao / Zhang Bao | `0x30c9` | `0x3122` | Original opcode46 adjacency of IDs0/146; original flag40 |
| Cao Cao / Zhang Liang | `0x3235` | `0x328e` | Original opcode46 adjacency of IDs0/147; original flag41 |
| Zhang Bao defeated | `0x37fd` | `0x3828` | HP0, flag50 |
| Zhang Liang defeated | `0x389b` | `0x38c6` | HP0, flag51 |
| Liu Bei retreats | `0x3929` | `0x3954` | HP0, flag52 |
| Guan Yu retreats | `0x39d0` | `0x39fb` | HP0, flag53 |
| Zhang Fei retreats | `0x3a5a` | `0x3a85` | HP0, flag54 |
| Cao Cao / Liu Bei victory exchange and departure lines | `0x333d` | `0x33e5`, `0x35bd`, `0x3652`, `0x36f3` | All three companions survive |
| Alternate Cao Cao victory speech | `0x333d` | `0x3740` | At least one companion has retreated |
| Defeat narration | `0x3c26` | `0x3c3f` | Prototype defeat, including turn limit |

The first two encounters contain five and three pages respectively. Chinese text
and line breaks are extracted from the source instructions, not rewritten.
Speaker markers select the original portrait. Page entries retain both source
instruction and event offsets. The immutable catalog is built from the packaged
script; snapshots do not supply arbitrary dialogue text or asset paths.

Original opcode46 requires active, living units at Chebyshev distance1, including
diagonals. `0x4100a1` retains the last eligible event. Consequently Zhang Liang's
encounter precedes Zhang Bao's when both become eligible together. Named defeat
speeches take precedence over encounters. Existing player-loop evidence places
event batches after committed or cancelled actions. Rendering waits for the
current walking/attack packet to finish before showing the reserved speech.

- Original `Ekd5.exe` SHA-256:
  `199b1b67cf966ead46c49b430b9aab84bdc7d4f0e8dfa8e0ebeec5da280b01e0`.
- Original `S_00.eex` SHA-256:
  `9f073c2164c4b5f9fb809dd24a1b543baadbef7e64292cf2812c9fbc7dea4d29`.
- Imported `scripts/S_00.json` SHA-256:
  `27cd8d9fd4123b58c2dfd55935932c0daf21beab328ba2f158cad3726f83acac`.
- [Script listing](S_00-listing.txt),
  [Cerberus selector dossier](../output/reverse-engineering/cerberus/exports/caocao/Ekd5.exe/dossiers/0x4100a1/decompile.c),
  [original-code replay](evidence/prototype-dialogue-original-replay.log),
  [C++ condition comparisons](evidence/prototype-dialogue-original-conditions.log).

## Persistence and compatibility

New snapshots use schema3 / engine `prototype-0.3`. The separate `ruleset` retains
`prototype-0.1` or `prototype-0.2` simulation compatibility. `dialogue` stores
triggered event IDs, pending event IDs, and the exact current page. An event's
flag is reserved when queued, so loading any of its pages neither repeats nor
skips the exchange. ID101 is a prototype variant identifier for the alternate
victory speech, not a newly discovered original variable.

Validation checks the checksum, known/unique IDs, queue order, pending trigger
conditions, outcome branch and page bounds before replacing live state.
Normal hashes now include dialogue state; gameplay-only hashes remain available
to compare old simulation/RNG behavior. Presentation packets remain transient.

Schema1/2 saves load without changing gameplay fields or RNG. Completed deaths
and outcomes are marked already handled; future proximity encounters remain
eligible because older files contain no conversation history. Reading an old
slot does not rewrite it. Saving afterward produces schema3, which older app
versions cannot load. Original `.e5s` files and Electron saves remain untouched.

## Verification

- Production conversation tests cover adjacency/diagonal priority, tentative
  movement/cancellation, attack/wait/end-turn boundaries, all five named retreat
  speeches, both victory branches, defeat, turn limit, blocking input and AI,
  single-use flags, exact-page restore, older-save next-strike/RNG preservation,
  and transactional rejection of corrupt event/page/branch states.
- Complete automated production-mode runs reach victory on turn8 and passive
  defeat on turn13. The379-checkpoint dialogue-inclusive trace is
  `25de5680625b5f0d35a0526298adb15bf3adc0cbffdcbe8ceec8a4e4ca2295e4`.
  The test suite consumes53 dialogue pages across focused and full-battle cases.
- Historical animation fixtures explicitly disable conversations to retain their
  schema1/2 comparison baseline. Their365-checkpoint trace remains
  `6916cde52483c54ce12fb7e2a8d9cd3d3e4d714e93f93fcf68882ddb419fdcfe`.
  Those fixtures alone are not evidence for the production dialogue feature.
- Native SDL tests render26 selected pages across encounters, named retirements,
  both victory branches and defeat. They verify line widths, glyph coverage,
  mouse/Space/Return continuation, F5/F9 restoration, enemy pause and attack-before-
  speech ordering. Fresh-process loading restores the same page and full hash.
  [Mac results](evidence/prototype-dialogue-mac/result.json),
  [Cao Cao encounter screenshot](evidence/prototype-dialogue-mac/zhang-liang-start.png),
  [restored Zhang Liang screenshot](evidence/prototype-dialogue-mac/zhang-liang-restored.png).
- macOS arm64, Linux x86-64 and WebAssembly produce the identical379-checkpoint
  trace above. Linux/OpenGL also passes all26 rendered pages and fresh-process
  restoration. A Linux mid-dialogue snapshot continued in Wasm, and a Wasm
  snapshot continued in Linux, both produce
  `19432468a447362a09ad45b3c0c518b2bd42a18b88512cdd61f83bc1a8e5d5c4`.
  [Platform build/replay log](evidence/prototype-0.2.5-platforms.log),
  [Linux rendering results](evidence/prototype-dialogue-linux/result.json).
- The exact packaged `Start Game.command` passes the same26-page native test
  and a separate-process reload when invoked from `/tmp` with spaces/Chinese
  in the test save/log paths. It loads adjacent packaged assets and uses Metal.
  [Packaged rendering](evidence/prototype-dialogue-packaged/result.json),
  [fresh-process reload](evidence/prototype-0.2.5-packaged-resume.log).
  The running older user process was left intact. Mac app and executable
  signatures verify after atomic executable replacement.

Reproduce native checks with an isolated save directory:

```sh
cmake --build build-mac --target caocao_prototype_app prototype_tests battle_events_tests -j8
build-mac/prototype_tests assets/prototype docs/evidence/prototype-mac.snapshot.json
build-mac/battle_events_tests assets/generated
CAOCAO_PROTOTYPE_SAVE_DIR=/tmp/caocao-dialogue-test \
  build-mac/caocao_prototype_app assets/prototype --dialogue-smoke /tmp/caocao-dialogue-frames
CAOCAO_PROTOTYPE_SAVE_DIR=/tmp/caocao-dialogue-test \
  build-mac/caocao_prototype_app assets/prototype --resume-smoke
scripts/build-prototype.sh all
python3 scripts/package-prototype-mac.py
```

## Scope limits

This is original dialogue integrated into the simplified battle. The prototype
still wins by defeating all enemies. The original event bodies can remove
remaining enemies after the leaders die; those removals, rewards and campaign
transitions are not added here. The victory speech therefore uses prototype
victory timing. Full original event-body interpretation is not claimed.

The prototype begins at the captured turn2 battlefield. Earlier introductions,
fire setup and tutorial conversations are not replayed. There is no arbitrary
talk command between any two units: the two proximity exchanges above are the
ones identified in the original opening-battle script. Dialogue is text with
portraits, not added voice acting. No full-campaign or exact original-window-layout
equivalence follows from these tests. Browser UI was not rerun for this update;
the same SDL renderer and simulation are built for WebAssembly.

JEV call064 reviewed persistence, action boundaries and evidence limits;
[the decision log](JEV-DECISIONS.md) records its exact role and disposition.
