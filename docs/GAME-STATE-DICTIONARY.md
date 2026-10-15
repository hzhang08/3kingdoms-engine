# Original game state dictionary

Dictionary version 1.0.2; SHA-256 `68f64c286ac0bbaf2d6151aba897161a46ba63ce50a4bd8b8a5b8c01a4a9701c`.

Executable SHA-256: `199b1b67cf966ead46c49b430b9aab84bdc7d4f0e8dfa8e0ebeec5da280b01e0`.

Opening tutorial exit, movement/cancellation and physical attacks; incomplete field semantics remain explicit. Observation captures are not resumable game saves.

All addresses below are original-edition x86 virtual addresses, not EXE file offsets. All numbers are little-endian.
Fields marked observed have bounded probe evidence; this does not establish every caller. Partial fields retain uncertainty.
Unknown bytes and flag bits are preserved. Signed views of bits32 do not change the raw unsigned word.
Record extents are the inspected windows, not claims about complete C++ object sizes.

## Usage

The editable source is [game_state_dictionary.json](../tools/game_state_dictionary.json). The shared Python API and CLI are in [game_state.py](../tools/game_state.py).

Run from the caocao-engine project directory:

```sh
.venv/bin/python tools/game_state.py validate
.venv/bin/python tools/game_state.py lookup 0x004b2c5d
.venv/bin/python tools/game_state.py --dictionary docs/evidence/game-state/dictionary-1.0.0.json decode docs/evidence/game-state/tutorial.json
.venv/bin/python tools/game_state.py --dictionary docs/evidence/game-state/dictionary-1.0.0.json diff docs/evidence/game-state/cancel-before.json docs/evidence/game-state/cancel-after.json
.venv/bin/python tools/game_state.py --dictionary docs/evidence/game-state/dictionary-1.0.0.json lookup 0x00818c80 --capture docs/evidence/game-state/cancel-before.json
```

The historical examples use the archived1.0.0 dictionary matching their recorded identity; current captures use the default dictionary. The final lookup uses the recorded buffer binding. Without a capture, that harness address is deliberately unclassified.

Within an existing initialized ProbeVM, capture only the records needed for the investigation:

```python
from game_state import default_dictionary, format_changes
selection = {"live_units": [0], "occupancy": [0], "rng": [0]}
before = v.capture_state(selection, labels={"live_units[0]": "曹操"})
# Run the probe's already-configured, bounded original action here.
after = v.capture_state(selection)
print(format_changes(default_dictionary().diff(before, after)))
```

Capture only reads memory. Observation files include executable/dictionary identities, exact bytes, buffer bindings and a memory checksum. They omit CPU context and cannot resume execution.
Comparisons reject mismatched identities, corrupt bytes and different record selections. Display labels are optional annotations, not decoded identity evidence.
All stored array capacity is preserved, including bytes after a target sentinel and cells outside the active map. Field notes describe when those bytes are inactive.

## Validation and regeneration

```sh
.venv/bin/python -m unittest discover -s tests -p test_game_state.py -v
.venv/bin/python tools/state_dictionary_probe.py
.venv/bin/python tools/game_state.py describe --output docs/GAME-STATE-DICTIONARY.md
```

The original-code probe requires the locally supplied game and imported assets. It checks tutorial setup, cancellation, strike planning and RNG, writing diagnostics only to docs/evidence/game-state.
See [verification](evidence/game-state/verification.json), [regression comparisons](evidence/game-state/regression.json), [tests](evidence/game-state/tests.log), and [JEV decisions](JEV-DECISIONS.md).
Current scope does not include a complete inventory layout, all script storage, all controller fields, native C++ object memory layout, or a save/checkpoint implementation.

## live_units

Live battle units. Original36-byte live record decoded by movement and combat probes; labels apply to observed opening behavior.

VA `0x004b2c50`; size `36`; stride `36`; count `115`.

| Offset | Field | Type and count | Signedness | Evidence level | Meaning and evidence |
|---|---|---|---|---|---|
| `0x0` | id | u32 × 1 | unsigned | observed | See cited probe and structure scope. Units: index/flags. Evidence: live, live_baseline. |
| `0x4` | slot | u8 × 1 | unsigned | observed | See cited probe and structure scope. Units: index/flags. Evidence: live, live_baseline. |
| `0x5` | side | u8 × 1 | unsigned | partial | Side byte is distinct from slot-based allegiance checks; unknown codes are preserved. 0=player; 1=ally; 2=enemy Units: index/flags. Evidence: live, deployment, live_baseline. |
| `0x6` | x | u8 × 1 | unsigned | observed | See cited probe and structure scope. Units: tiles. Evidence: live, live_baseline. |
| `0x7` | y | u8 × 1 | unsigned | observed | See cited probe and structure scope. Units: tiles. Evidence: live, live_baseline. |
| `0x8` | target_slot | u8 × 1 | unsigned | observed | See cited probe and structure scope. 255=no target / unspecified in tested callers Units: index/flags. Evidence: live, live_baseline. |
| `0x9` | target_x | u8 × 1 | unsigned | observed | See cited probe and structure scope. 255=no target / unspecified in tested callers Units: tiles. Evidence: live, live_baseline. |
| `0xa` | target_y | u8 × 1 | unsigned | observed | See cited probe and structure scope. 255=no target / unspecified in tested callers Units: tiles. Evidence: live, live_baseline. |
| `0xb` | priority | u8 × 1 | unsigned | partial | Live+11 is assigned by AI priority routines; total ordering semantics are outside this field definition. Units: index/flags. Evidence: priority. |
| `0xc` | lifecycle | u8 × 1 | unsigned | observed | See cited probe and structure scope. 0=unused; 1=hidden/not deployed; 2=visible on battlefield; 3=retired in observed removal paths Units: index/flags. Evidence: live, live_baseline. |
| `0xd` | flags | u8 × 1 | unsigned | partial | Unknown bits retained. Bit32 participates in strike reaction but its general meaning is not assigned. 0x2=acted; 0x4=moved; 0x8=sprite mirror; 0x80=scripted movement/control override Units: index/flags. Evidence: flags. |
| `0xe` | ai | u8 × 1 | unsigned | observed | See cited probe and structure scope. Units: index/flags. Evidence: live, live_baseline. |
| `0xf` | direction | u8 × 1 | unsigned | observed | See cited probe and structure scope. 0=north; 1=east; 2=south; 3=west Units: index/flags. Evidence: move. |
| `0x10` | hp | u32 × 1 | unsigned | observed | Current battle HP; distinct from persistent maximum HP. Original poison450984 displays from pre-mutation HP;44df61 writes HP only after the panel returns. Fresh turn-two ally210 changes44 to33. Units: points. Evidence: live, live_baseline, phase_maintenance. |
| `0x14` | mp | u32 × 1 | unsigned | observed | See cited probe and structure scope. Units: points. Evidence: live, live_baseline. |
| `0x18` | grades | u8 × 6 | unsigned | observed | Neutral grade3; raw0..6 values retained. Stat order follows original getter callers. Units: index/flags. Evidence: live, stat_axes, live_baseline. Elements: attack, defense, spirit, agility, morale, movement. |
| `0x1e` | status | u8 × 1 | unsigned | partial | Only poison label is assigned; raw masks2/4/8 are tested by recovery but retained as unknown bits here. 0x10=poison Units: index/flags. Evidence: status, poison. |

Unknown byte intervals (end exclusive): `[0x1f,0x24)`.

## persistent_units

Persistent character records. Original72-byte persistent record; growth/join/equipment fixtures. Persistent HP/MP are not current battle HP/MP.

VA `0x004a1b68`; size `72`; stride `72`; count `512`.

| Offset | Field | Type and count | Signedness | Evidence level | Meaning and evidence |
|---|---|---|---|---|---|
| `0x2` | story_sprite | u8 × 1 | unsigned | observed | See cited probe and structure scope. Units: raw integer. Evidence: portrait. |
| `0x4` | face_base | u32 × 1 | unsigned | observed | See cited probe and structure scope. Units: raw integer. Evidence: portrait. |
| `0x15` | stats | u8 × 5 | unsigned | observed | See cited probe and structure scope. Units: raw integer. Evidence: persistent, stat_axes, persistent_baseline. Elements: attack, defense, spirit, agility, morale. |
| `0x1c` | hp | u32 × 1 | unsigned | observed | Persistent stat used to compute maximum HP; distinct from live current HP and equipment-adjusted maximum. Units: base/level-grown HP. Evidence: persistent, persistent_baseline. |
| `0x20` | mp | u8 × 1 | unsigned | observed | Persistent stat used to compute maximum MP; distinct from live current MP. Units: base/level-grown MP. Evidence: persistent, persistent_baseline. |
| `0x21` | base_stats | u8 × 5 | unsigned | observed | Growth attributes, not current combat statistics. Last byte is also read as base luck by recovery. Units: raw integer. Evidence: persistent, persistent_baseline. Elements: attack_growth_attribute, defense_growth_attribute, spirit_growth_attribute, agility_growth_attribute, luck. |
| `0x26` | speech_group | u8 × 1 | unsigned | partial | Critical-speech lookup group; see include/caocao/strike.hpp. Units: raw integer. Evidence: speech. |
| `0x27` | record_counters | u8 × 3 | unsigned | partial | Raw record counters; individual meanings intentionally unassigned. Units: raw integer. Evidence: persistent, persistent_baseline. |
| `0x2a` | owner | u8 × 1 | unsigned | observed | See cited probe and structure scope. Units: raw integer. Evidence: persistent, persistent_baseline. |
| `0x2b` | class | u8 × 1 | unsigned | observed | See cited probe and structure scope. Units: raw integer. Evidence: persistent, persistent_baseline. |
| `0x2c` | level | u8 × 1 | unsigned | observed | See cited probe and structure scope. Units: raw integer. Evidence: persistent, persistent_baseline. |
| `0x2d` | xp | u8 × 1 | unsigned | observed | See cited probe and structure scope. Units: raw integer. Evidence: persistent, persistent_baseline. |
| `0x2e` | equipment | u8 × 9 | unsigned | observed | Three triples(ID,level,XP). ID255 denotes empty;255 in other components is preserved without assuming the same meaning. Units: raw integer. Evidence: persistent, persistent_baseline. Elements: weapon.id, weapon.level, weapon.xp, armor.id, armor.level, armor.xp, accessory.id, accessory.level, accessory.xp. |

Unknown byte intervals (end exclusive): `[0x0,0x2)`, `[0x3,0x4)`, `[0x8,0x15)`, `[0x1a,0x1c)`, `[0x37,0x48)`.

## deployment

Scenario deployment descriptors. Original opening script deployment descriptors; no arbitrary later-battle selector semantics asserted.

VA `0x00494fd4`; size `64`; stride `64`; count `115`.

| Offset | Field | Type and count | Signedness | Evidence level | Meaning and evidence |
|---|---|---|---|---|---|
| `0x0` | id | u32 × 1 | unsigned | observed | See cited probe and structure scope. 65535=unused descriptor Units: raw integer. Evidence: deployment. |
| `0x4` | x | u8 × 1 | unsigned | observed | See cited probe and structure scope. Units: raw integer. Evidence: deployment. |
| `0x5` | y | u8 × 1 | unsigned | observed | See cited probe and structure scope. Units: raw integer. Evidence: deployment. |
| `0x8` | flags | u32 × 1 | unsigned | observed | See cited probe and structure scope. Units: raw integer. Evidence: deployment. |
| `0xc` | ai | u8 × 1 | unsigned | observed | See cited probe and structure scope. Units: raw integer. Evidence: deployment. |
| `0xe` | target | u16 × 1 | unsigned | observed | See cited probe and structure scope. Units: raw integer. Evidence: deployment. |
| `0x10` | promotion_mode | u8 × 1 | unsigned | observed | See cited probe and structure scope. Units: raw integer. Evidence: deployment. |
| `0x11` | level | u8 × 1 | unsigned | observed | See cited probe and structure scope. Units: raw integer. Evidence: deployment. |
| `0x20` | weapon_level | u8 × 1 | unsigned | observed | See cited probe and structure scope. Units: raw integer. Evidence: deployment. |
| `0x21` | armor_level | u8 × 1 | unsigned | observed | See cited probe and structure scope. Units: raw integer. Evidence: deployment. |
| `0x14` | equipment_overrides | u32 × 3 | unsigned | observed | Encoded script overrides, not necessarily raw item IDs; do not collapse sentinel/encoding values. Units: raw integer. Evidence: deployment. |

Unknown byte intervals (end exclusive): `[0x6,0x8)`, `[0xd,0xe)`, `[0x12,0x14)`, `[0x22,0x40)`.

## rng

Original random-number state. Original LCG state/output fixtures; modulo2^32 state, output(state>>16)&32767.

VA `0x0048ecd4`; size `4`; stride `4`; count `1`.

| Offset | Field | Type and count | Signedness | Evidence level | Meaning and evidence |
|---|---|---|---|---|---|
| `0x0` | state | u32 × 1 | unsigned | observed | state=(state*1103515245+12345) modulo2^32; output=(state>>16)&32767. Turn-two status recovery advances340946258 to601077795 before poison; the captured poison display/HP mutation does not advance RNG. Units: LCG bits. Evidence: rng, phase_maintenance. |

Unknown byte intervals (end exclusive): none within this inspected window.

## attack

Physical strike controller. Original strike plan fixture and controller inputs; damage stays a32-bit pattern because callers mix signed checks and unsigned arithmetic.

VA `0x004927f0`; size `1556`; stride `1556`; count `1`.

| Offset | Field | Type and count | Signedness | Evidence level | Meaning and evidence |
|---|---|---|---|---|---|
| `0x0` | actor_slot | u8 × 1 | unsigned | observed | See cited probe and structure scope. Units: raw integer. Evidence: strike. |
| `0x1` | primary_target_slot | u8 × 1 | unsigned | observed | See cited probe and structure scope. Units: raw integer. Evidence: strike. |
| `0x4` | actor_id | u32 × 1 | unsigned | observed | See cited probe and structure scope. Units: raw integer. Evidence: strike. |
| `0x8` | actor_record | ptr32 × 1 | unsigned address | observed | See cited probe and structure scope. Units: raw integer. Evidence: strike. |
| `0xc` | actor_live | ptr32 × 1 | unsigned address | observed | See cited probe and structure scope. Units: raw integer. Evidence: strike. |
| `0x10` | targets | u8 × 116 | unsigned | observed | Storage capacity116 includes sentinel; bytes after first255 remain raw scratch and are not active targets. 255=ordered target-list terminator Units: raw integer. Evidence: strike. |
| `0x84` | hp_damage | bits32 × 116 | context-dependent | observed | Preserve unsigned raw word and signed interpretation; original callers use unsigned subtraction plus signed positive-damage checks. Entries align with ordered target positions, not live slot IDs. Units: raw integer. Evidence: strike_apply. |
| `0x254` | mp_damage | bits32 × 116 | context-dependent | observed | Preserve unsigned raw word and signed interpretation; original callers use unsigned subtraction plus signed positive-damage checks. Entries align with ordered target positions, not live slot IDs. Units: raw integer. Evidence: strike_apply. |
| `0x424` | area_mode | u8 × 1 | unsigned | observed | See cited probe and structure scope. Units: raw integer. Evidence: strike_apply. |
| `0x428` | xp | u32 × 1 | unsigned | observed | See cited probe and structure scope. Units: raw integer. Evidence: strike. |
| `0x42c` | weapon_xp | u32 × 1 | unsigned | observed | See cited probe and structure scope. Units: raw integer. Evidence: strike. |
| `0x430` | armor_xp | u32 × 116 | unsigned | observed | See cited probe and structure scope. Units: raw integer. Evidence: strike. |
| `0x600` | actor_is_player | u32 × 1 | unsigned | observed | See cited probe and structure scope. 0=false; 1=true Units: boolean storage. Evidence: strike. |
| `0x604` | critical | u32 × 1 | unsigned | observed | See cited probe and structure scope. 0=false; 1=true Units: boolean storage. Evidence: strike. |
| `0x608` | round | u8 × 1 | unsigned | observed | Strike-round byte; verified ordinary0 and follow-up1; not the scenario turn count. Units: raw integer. Evidence: strike. |

Unknown byte intervals (end exclusive): `[0x2,0x4)`, `[0x425,0x428)`, `[0x609,0x614)`.

## counter

Physical counterattack state. Original strike plan fixture and controller inputs; damage stays a32-bit pattern because callers mix signed checks and unsigned arithmetic.

VA `0x00492e04`; size `4`; stride `4`; count `1`.

| Offset | Field | Type and count | Signedness | Evidence level | Meaning and evidence |
|---|---|---|---|---|---|
| `0x0` | depth | u32 × 1 | unsigned | observed | See cited probe and structure scope. Units: raw integer. Evidence: strike. |

Unknown byte intervals (end exclusive): none within this inspected window.

## tutorial

Opening tutorial state. Original tutorial setup with explicit collection bytes; no legacy save writes.

VA `0x004b2c04`; size `22`; stride `22`; count `1`.

| Offset | Field | Type and count | Signedness | Evidence level | Meaning and evidence |
|---|---|---|---|---|---|
| `0x0` | unit_pointer_c | ptr32 × 1 | unsigned address | partial | Original setup captures three live-unit pointers. Only guide role is assigned; other pointer roles remain descriptive. Units: raw integer. Evidence: tutorial. |
| `0x8` | unit_pointer_b | ptr32 × 1 | unsigned address | partial | Original setup captures three live-unit pointers. Only guide role is assigned; other pointer roles remain descriptive. Units: raw integer. Evidence: tutorial. |
| `0x10` | guide_pointer | ptr32 × 1 | unsigned address | observed | Original setup captures three live-unit pointers. Only guide role is assigned; other pointer roles remain descriptive. Units: raw integer. Evidence: tutorial. |
| `0x4` | all_treasures | u32 × 1 | unsigned | observed | See cited probe and structure scope. 0=false; 1=true Units: raw integer. Evidence: tutorial. |
| `0xc` | completed_routes | u8 × 1 | unsigned | observed | Count of nonzero completion bytes among three collection entries; not the original three-bit input mask. Units: raw integer. Evidence: tutorial. |
| `0x14` | cursor | u8 × 2 | unsigned | observed | See cited probe and structure scope. 255=unspecified Units: world tiles. Evidence: tutorial. Elements: x, y. |

Unknown byte intervals (end exclusive): `[0xd,0x10)`.

## camera

Camera and overlay state prefix. Known prefix only; object extends beyond0x70. Unknown bytes retained. Pixel origins and camera-relative tile cursors must not be mixed.

VA `0x004b4250`; size `112`; stride `112`; count `1`.

| Offset | Field | Type and count | Signedness | Evidence level | Meaning and evidence |
|---|---|---|---|---|---|
| `0x4` | pixel_x | bits32 × 1 | context-dependent | observed | Preserve raw32-bit word and signed view; native placement uses signed pixel arithmetic. Origins are not tile indices. Units: pixels. Evidence: menu. |
| `0x8` | pixel_y | bits32 × 1 | context-dependent | observed | Preserve raw32-bit word and signed view; native placement uses signed pixel arithmetic. Origins are not tile indices. Units: pixels. Evidence: menu. |
| `0x14` | viewport_width | bits32 × 1 | context-dependent | observed | Preserve raw32-bit word and signed view; native placement uses signed pixel arithmetic. Origins are not tile indices. Units: pixels. Evidence: menu. |
| `0x18` | viewport_height | bits32 × 1 | context-dependent | observed | Preserve raw32-bit word and signed view; native placement uses signed pixel arithmetic. Origins are not tile indices. Units: pixels. Evidence: menu. |
| `0x3c` | client_x | bits32 × 1 | context-dependent | observed | Client drawing origin returned by451a4f/451a60; distinct from world camera pixel_x/pixel_y. Original panel placement adds this origin to tile*48. Headless probes explicitly supply0,56; this is not proof of Windows client initialization. Units: pixels. Evidence: menu, phase_maintenance. |
| `0x40` | client_y | bits32 × 1 | context-dependent | observed | Client drawing origin returned by451a4f/451a60; distinct from world camera pixel_x/pixel_y. Original panel placement adds this origin to tile*48. Headless probes explicitly supply0,56; this is not proof of Windows client initialization. Units: pixels. Evidence: menu, phase_maintenance. |
| `0x44` | covered_rect | u8 × 4 | unsigned | observed | See cited probe and structure scope. Units: camera-relative tiles. Evidence: overlay. Elements: x, y, width, height. |
| `0x50` | covered | u32 × 1 | unsigned | observed | See cited probe and structure scope. Units: raw integer. Evidence: overlay. |
| `0x54` | selection_cursor | u8 × 2 | unsigned | observed | World position adds camera pixel origin divided by48. Both255 means no selection; scrolling clears these bytes even without displacement. 255=no cursor Units: camera-relative tiles. Evidence: camera. Elements: x, y. |
| `0x61` | cursor_style | u8 × 1 | unsigned | partial | See cited probe and structure scope. Units: raw integer. Evidence: overlay. |
| `0x64` | blue_movement | u32 × 1 | unsigned | observed | See cited probe and structure scope. Units: raw integer. Evidence: overlay. |
| `0x68` | green_movement | u32 × 1 | unsigned | observed | See cited probe and structure scope. Units: raw integer. Evidence: overlay. |

Unknown byte intervals (end exclusive): `[0x0,0x4)`, `[0xc,0x14)`, `[0x1c,0x3c)`, `[0x48,0x50)`, `[0x56,0x61)`, `[0x62,0x64)`, `[0x6c,0x70)`.

## command_menu

Command menu object prefix. Original menu callbacks and enablement with supplied host window sizes; UI layout pixels remain provisional. Player command availability, wait and cancellation also execute in the selected-unit parent controller.

VA `0x004b5e98`; size `68`; stride `68`; count `1`.

| Offset | Field | Type and count | Signedness | Evidence level | Meaning and evidence |
|---|---|---|---|---|---|
| `0x4` | window_handle | ptr32 × 1 | unsigned address | partial | Opaque HWND value, not a pointer to a decoded memory structure. Units: raw integer. Evidence: menu. |
| `0x28` | result | u8 × 1 | unsigned | observed | See cited probe and structure scope. 1=attack; 2=tactic; 3=item; 4=standby; 255=cancel/no selection Units: raw integer. Evidence: menu. |
| `0x2c` | anchor_x | bits32 × 1 | context-dependent | observed | See cited probe and structure scope. Units: pixels. Evidence: menu. |
| `0x30` | anchor_y | bits32 × 1 | context-dependent | observed | See cited probe and structure scope. Units: pixels. Evidence: menu. |
| `0x34` | attack_enabled | u32 × 1 | unsigned | observed | Original43e273 attack-range result has a target; original distance/threat/mask grids and ordered target list are compared. 0=false; 1=true Units: boolean storage. Evidence: menu, player_action. |
| `0x38` | tactic_enabled | u32 × 1 | unsigned | observed | Original43dde3 enables for any class/level-unlocked tactic, including cardinal neighbors when equipment effect51 is present. This menu layer does not filter MP, silence, neighbor allegiance or neighbor HP. Castability is separate. 0=false; 1=true Units: boolean storage. Evidence: menu, player_action. |
| `0x3c` | item_enabled | u32 × 1 | unsigned | observed | Original40d9ee returns whether any of the17 consumable stock bytes is nonzero; no target/usable-item check here. 0=false; 1=true Units: boolean storage. Evidence: menu, player_action. |
| `0x40` | standby_enabled | u32 × 1 | unsigned | observed | Original menu initializer enables standby. Selecting4 sets live flags2 and4; parent returns1. Tutorial demonstration remains a separate non-gameplay path. 0=false; 1=true Units: boolean storage. Evidence: menu, player_action. |

Unknown byte intervals (end exclusive): `[0x0,0x4)`, `[0x8,0x28)`, `[0x29,0x2c)`.

## battle

Battle controller prefix. Original bounded battle controller init; do not infer fields outside listed offsets.

VA `0x004b3d08`; size `25`; stride `25`; count `1`.

| Offset | Field | Type and count | Signedness | Evidence level | Meaning and evidence |
|---|---|---|---|---|---|
| `0x1` | outcome | u8 × 1 | unsigned | observed | Raw value; enum semantics not extrapolated beyond original probes. In the verified player loop, raw5 returns immediately and raw6 requests end-turn confirmation then resets to0; these are UI commands, not victory/defeat values. Units: raw integer. Evidence: script, player_input. |
| `0x2` | turn_minus_one | u8 × 1 | unsigned | observed | Stored turn count is zero-based; displayed turn adds1. Units: raw integer. Evidence: script. |
| `0x3` | turn_limit | u8 × 1 | unsigned | observed | Raw value; enum semantics not extrapolated beyond original probes. Units: raw integer. Evidence: controller. |
| `0x4` | environment_selector | u8 × 1 | unsigned | observed | Raw value; enum semantics not extrapolated beyond original probes. Units: raw integer. Evidence: controller. |
| `0x5` | animation_tick | u8 × 1 | unsigned | observed | Raw value; enum semantics not extrapolated beyond original probes. Units: raw integer. Evidence: controller. |
| `0x10` | phase | u8 × 1 | unsigned | observed | Raw value; enum semantics not extrapolated beyond original probes. Units: raw integer. Evidence: script. |
| `0x11` | region | u8 × 1 | unsigned | observed | Raw value; enum semantics not extrapolated beyond original probes. Units: raw integer. Evidence: controller. |

Unknown byte intervals (end exclusive): `[0x0,0x1)`, `[0x6,0x10)`, `[0x12,0x19)`.

## input_gate

Battle input gate. Original phase transition input gate; stops before first AI action.

VA `0x004b4228`; size `4`; stride `4`; count `1`.

| Offset | Field | Type and count | Signedness | Evidence level | Meaning and evidence |
|---|---|---|---|---|---|
| `0x0` | active | u32 × 1 | unsigned | observed | See cited probe and structure scope. Units: raw integer. Evidence: phase. |

Unknown byte intervals (end exclusive): none within this inspected window.

## script_reader

EEX reader prefix. Original condition/event reader state and1024 variable slots; backing pointers are runtime addresses.

VA `0x00492f90`; size `56`; stride `56`; count `1`.

| Offset | Field | Type and count | Signedness | Evidence level | Meaning and evidence |
|---|---|---|---|---|---|
| `0x0` | vtable | ptr32 × 1 | unsigned address | observed | See cited probe and structure scope. Units: raw integer. Evidence: script. |
| `0x4` | mode | u32 × 1 | unsigned | partial | See cited probe and structure scope. Units: raw integer. Evidence: script. |
| `0x8` | argument_byte_count | u32 × 1 | unsigned | observed | See cited probe and structure scope. Units: raw integer. Evidence: reader. |
| `0xc` | cursor | ptr32 × 1 | unsigned address | observed | See cited probe and structure scope. Units: raw integer. Evidence: reader. |
| `0x1c` | storage_blocks | ptr32 × 1 | unsigned address | observed | See cited probe and structure scope. Units: raw integer. Evidence: script. |
| `0x20` | script_base | ptr32 × 1 | unsigned address | observed | See cited probe and structure scope. Units: raw integer. Evidence: script. |
| `0x24` | event_index | u32 × 1 | unsigned | partial | See cited probe and structure scope. Units: raw integer. Evidence: script. |
| `0x28` | section | u32 × 1 | unsigned | partial | See cited probe and structure scope. Units: raw integer. Evidence: script. |

Unknown byte intervals (end exclusive): `[0x10,0x1c)`, `[0x2c,0x38)`.

## script_variables

EEX numeric variable slots. Original condition/event reader state and1024 variable slots; backing pointers are runtime addresses.

VA `0x00492fc8`; size `4`; stride `4`; count `1024`.

| Offset | Field | Type and count | Signedness | Evidence level | Meaning and evidence |
|---|---|---|---|---|---|
| `0x0` | value | bits32 × 1 | context-dependent | observed | Raw32-bit script slot; tested opening event flags often use0/1, but do not force boolean semantics or signedness for all slots. Units: raw integer. Evidence: script. |

Unknown byte intervals (end exclusive): none within this inspected window.

## priority_target

Script priority-target field. Original strike plan fixture and controller inputs; damage stays a32-bit pattern because callers mix signed checks and unsigned arithmetic.

VA `0x00496c96`; size `2`; stride `2`; count `1`.

| Offset | Field | Type and count | Signedness | Evidence level | Meaning and evidence |
|---|---|---|---|---|---|
| `0x0` | unit_id | u16 × 1 | unsigned | partial | See cited probe and structure scope. Units: raw integer. Evidence: strike. |

Unknown byte intervals (end exclusive): none within this inspected window.

## map_dimensions

Map dimensions. Original map-buffer descriptor is fixed; allocated buffer address in probes is a harness choice, not a game VA.

VA `0x004b422c`; size `2`; stride `2`; count `1`.

| Offset | Field | Type and count | Signedness | Evidence level | Meaning and evidence |
|---|---|---|---|---|---|
| `0x0` | width | u8 × 1 | unsigned | observed | See cited probe and structure scope. Units: tiles. Evidence: map. |
| `0x1` | height | u8 × 1 | unsigned | observed | See cited probe and structure scope. Units: tiles. Evidence: map. |

Unknown byte intervals (end exclusive): none within this inspected window.

## map_buffer

Map buffer descriptor prefix. Original map-buffer descriptor is fixed; allocated buffer address in probes is a harness choice, not a game VA.

VA `0x004aeb38`; size `20`; stride `20`; count `1`.

| Offset | Field | Type and count | Signedness | Evidence level | Meaning and evidence |
|---|---|---|---|---|---|
| `0x4` | capacity | u32 × 1 | unsigned | observed | See cited probe and structure scope. Units: bytes. Evidence: map. |
| `0xc` | data | ptr32 × 1 | unsigned address | observed | See cited probe and structure scope. Units: raw integer. Evidence: map. |
| `0x10` | owned_flag | u8 × 1 | unsigned | partial | Probe supplies1; complete allocator/ownership semantics unverified. Units: raw integer. Evidence: map. |

Unknown byte intervals (end exclusive): `[0x0,0x4)`, `[0x8,0xc)`, `[0x11,0x14)`.

## terrain

Terrain buffer. Resolved via actual map-buffer pointer, never hardcoded harness address. Plane capacity is1600 bytes; only width*height cells are active.

`u32(0x004aeb44) + 0x640`, bounded by capacity at `0x004aeb3c`; size `1600`; stride `1600`; count `1`.

| Offset | Field | Type and count | Signedness | Evidence level | Meaning and evidence |
|---|---|---|---|---|---|
| `0x0` | bytes | u8 × 1600 | unsigned | partial | Preserve all bytes, including unused/stale capacity; current map dimensions define active cell range. Units: cell byte. Evidence: map. |

Unknown byte intervals (end exclusive): none within this inspected window.

## occupancy

Occupancy buffer. Resolved via actual map-buffer pointer, never hardcoded harness address. Plane capacity is1600 bytes; only width*height cells are active.

`u32(0x004aeb44) + 0xc80`, bounded by capacity at `0x004aeb3c`; size `1600`; stride `1600`; count `1`.

| Offset | Field | Type and count | Signedness | Evidence level | Meaning and evidence |
|---|---|---|---|---|---|
| `0x0` | bytes | u8 × 1600 | unsigned | partial | Preserve all bytes, including unused/stale capacity; current map dimensions define active cell range. 255=empty Units: cell byte. Evidence: map. |

Unknown byte intervals (end exclusive): none within this inspected window.

## distance

Distance buffer. Resolved via actual map-buffer pointer, never hardcoded harness address. Plane capacity is1600 bytes; only width*height cells are active.

`u32(0x004aeb44) + 0x12c0`, bounded by capacity at `0x004aeb3c`; size `1600`; stride `1600`; count `1`.

| Offset | Field | Type and count | Signedness | Evidence level | Meaning and evidence |
|---|---|---|---|---|---|
| `0x0` | bytes | u8 × 1600 | unsigned | partial | Preserve all bytes, including unused/stale capacity; current map dimensions define active cell range. Units: cell byte. Evidence: map. |

Unknown byte intervals (end exclusive): none within this inspected window.

## route

Route buffer. Original route scratch includes stale bytes; preserve all256 bytes rather than stopping at sentinel.

`u32(0x004aeb44) + 0x1900`, bounded by capacity at `0x004aeb3c`; size `256`; stride `256`; count `1`.

| Offset | Field | Type and count | Signedness | Evidence level | Meaning and evidence |
|---|---|---|---|---|---|
| `0x0` | bytes | u8 × 256 | unsigned | partial | Original route scratch includes stale bytes; preserve all256 bytes rather than stopping at sentinel. Units: route scratch byte. Evidence: route. |

Unknown byte intervals (end exclusive): none within this inspected window.

## terrain_effects

Terrain Effects buffer. Resolved via actual map-buffer pointer, never hardcoded harness address. Plane capacity is1600 bytes; only width*height cells are active.

`u32(0x004aeb44) + 0x1f40`, bounded by capacity at `0x004aeb3c`; size `1600`; stride `1600`; count `1`.

| Offset | Field | Type and count | Signedness | Evidence level | Meaning and evidence |
|---|---|---|---|---|---|
| `0x0` | bytes | u8 × 1600 | unsigned | partial | Preserve all bytes, including unused/stale capacity; current map dimensions define active cell range. 255=empty Units: cell byte. Evidence: map. |

Unknown byte intervals (end exclusive): none within this inspected window.

## consumables

Consumable stock bytes. 17 byte stock entries used by40d9ee menu availability. Item-ID mapping is not asserted by these availability fixtures.

VA `0x004b09db`; size `17`; stride `17`; count `1`.

| Offset | Field | Type and count | Signedness | Evidence level | Meaning and evidence |
|---|---|---|---|---|---|
| `0x0` | counts | u8 × 17 | unsigned | observed | Each independently supplied entry with value1 or255 enables item menu; all-zero stock disables it. No use/decrement semantics established. Units: stock count. Evidence: player_action. |

Unknown byte intervals (end exclusive): none within this inspected window.

## player_panel_retention

Player loop panel retention gate. Known read/reset behavior only; not all writers or complete UI semantics.

VA `0x004b4208`; size `4`; stride `4`; count `1`.

| Offset | Field | Type and count | Signedness | Evidence level | Meaning and evidence |
|---|---|---|---|---|---|
| `0x0` | gate | u32 × 1 | unsigned | observed | 44e355 retains previous hovered slot on an empty/outside tile when this field is nonzero, tileY is255, and the main window has focus. Clearing a covered panel on the empty-tile path also zeros the field. Inputs are supplied host state. Units: nonzero gate. Evidence: player_input. |

Unknown byte intervals (end exclusive): none within this inspected window.

## Evidence index

- **live**: [tools/movement_probe.py](../tools/movement_probe.py); anchor `def live_json(v,slot):`. Original36-byte live record decoded by movement and combat probes; labels apply to observed opening behavior.
- **persistent**: [tools/initialization_probe.py](../tools/initialization_probe.py); anchor `def state(vm,uid):`. Original72-byte persistent record; growth/join/equipment fixtures. Persistent HP/MP are not current battle HP/MP.
- **portrait**: [tools/unit_probe.py](../tools/unit_probe.py); anchor `'face_base':struct.unpack_from`. Original record loader/portrait probe over512 characters at campaign stage zero.
- **deployment**: [tools/deployment_probe.py](../tools/deployment_probe.py); anchor `def slots(v):`. Original opening script deployment descriptors; no arbitrary later-battle selector semantics asserted.
- **flags**: [docs/FORMATS.md](../docs/FORMATS.md); anchor `clears action/movement flags`. Original movement/attack/banner probes establish action2, moved4, mirror8 and scripted override128 in scoped paths; other bits remain unknown.
- **move**: [tools/tutorial_actions_probe.py](../tools/tutorial_actions_probe.py); anchor `actual==bytes(expected)`. Original cancellation preserves all live bytes except coordinates; compares occupancy, persistent records and RNG.
- **status**: [tools/recovery_probe.py](../tools/recovery_probe.py); anchor `for status in (0,1,2,4,8,16,30,255)`. Original abnormal-status recovery cases; dictionary keeps masks without inventing names for unverified bits.
- **poison**: [src/recovery.cpp](../src/recovery.cpp); anchor `if(!(b.status&16))return 0;`. Port of the original poison-maintenance fixture identifies poison bit16.
- **strike**: [tools/strike_probe.py](../tools/strike_probe.py); anchor `CONTROL=0x4927f0`. Original strike plan fixture and controller inputs; damage stays a32-bit pattern because callers mix signed checks and unsigned arithmetic.
- **strike_apply**: [tools/strike_application_probe.py](../tools/strike_application_probe.py); anchor `unsigned setters`. Original HP/MP application, wrap/cap behavior and automatic-item boundary; no automatic-item completion implied.
- **rng**: [tools/executable_probe.py](../tools/executable_probe.py); anchor `'multiply':1103515245`. Original LCG state/output fixtures; modulo2^32 state, output(state>>16)&32767.
- **tutorial**: [tools/tutorial_probe.py](../tools/tutorial_probe.py); anchor `def setup():`. Original tutorial setup with explicit collection bytes; no legacy save writes.
- **camera**: [tools/tutorial_cursor_probe.py](../tools/tutorial_cursor_probe.py); anchor `after==[255,255]`. Original cursor clearing and camera-scroll lifetime; lower host raster/placement boundaries captured.
- **overlay**: [tools/overlay_probe.py](../tools/overlay_probe.py); anchor `camera+0x68`. Original painter probes: blue/green flags, covered-region and selection-cursor ordering.
- **menu**: [tools/tutorial_menu_probe.py](../tools/tutorial_menu_probe.py); anchor `DIALOG=0x4b5e98`. Original menu callbacks and enablement with supplied host window sizes; UI layout pixels remain provisional.
- **controller**: [tools/controller_initialization_probe.py](../tools/controller_initialization_probe.py); anchor `after[1:6]`. Original bounded battle controller init; do not infer fields outside listed offsets.
- **phase**: [tools/phase_transition_probe.py](../tools/phase_transition_probe.py); anchor `input_active=v.u32(CTRL+0x520)`. Original phase transition input gate; stops before first AI action.
- **script**: [tools/battle_events_probe.py](../tools/battle_events_probe.py); anchor `READER=0x492f90`. Original condition/event reader state and1024 variable slots; backing pointers are runtime addresses.
- **reader**: [tools/deployment_probe.py](../tools/deployment_probe.py); anchor `expected_count=2`. Typed argument count excludes string payload bytes; pointer and counter are distinct.
- **map**: [tools/battle_initialization_probe.py](../tools/battle_initialization_probe.py); anchor `v.put32(0x4aeb38+4,0x2580)`. Original map-buffer descriptor is fixed; allocated buffer address in probes is a harness choice, not a game VA.
- **route**: [tools/route_probe.py](../tools/route_probe.py); anchor `does not append a terminator`. Original route scratch includes stale bytes; preserve all256 bytes rather than stopping at sentinel.
- **priority**: [tools/ai_priority_probe.py](../tools/ai_priority_probe.py); anchor `Only priority bytes may change.`. Original AI-priority execution checks that only live byte11 changes.
- **stat_axes**: [src/combat_rules.cpp](../src/combat_rules.cpp); anchor `in.defense=battle_stat(b,bu,1,data);`. Battle-stat index mapping in the differential-tested port: attack0, defense1, spirit2, agility3 and morale4.
- **speech**: [include/caocao/strike.hpp](../include/caocao/strike.hpp); anchor `persistent offset 0x26`. Original critical-speech group field used by critical_speech_probe.py.
- **live_baseline**: [tests/test_game_state.py](../tests/test_game_state.py); anchor `def old_live(row):`. Frozen byte decoder from movement_probe.py before dictionary migration; independently checks all decoded values without reading dictionary offsets.
- **persistent_baseline**: [tests/test_game_state.py](../tests/test_game_state.py); anchor `def old_unit(row, uid):`. Frozen byte decoder from initialization_probe.py before dictionary migration; checks512 raw records and original fixture regeneration.
- **phase_maintenance**: [tools/phase_maintenance_probe.py](../tools/phase_maintenance_probe.py); anchor `run('poison'`. Sequential fresh-profile turn-two recovery, NPC poison numeric display before HP write, then regeneration. Host client origin explicitly supplied; no Windows window creation or player input.
- **player_action**: [tools/player_action_probe.py](../tools/player_action_probe.py); anchor `0x4b0770+0x26b`. 1106 original movement/menu/wait/cancel cases with explicit tile/modal inputs. Tests availability and persistent/RNG invariance, not attack/tactic/item execution.
- **player_input**: [tools/player_input_probe.py](../tools/player_input_probe.py); anchor `0x44e3a9`. Original loop with explicit host inputs. Retained hover, wait/cancel dispatch, event/death order and remaining-unit scan; window/raster boundaries supplied.
