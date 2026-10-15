# Observed formats in the supplied edition

Executable SHA-256: `199b1b67cf966ead46c49b430b9aab84bdc7d4f0e8dfa8e0ebeec5da280b01e0`.

## LS12

`Ls12`, 12 padding bytes, a 256-byte literal dictionary, then big-endian
(packed size, output size, file offset) triples at 0x110; a zero packed size
terminates the directory. Equal lengths mean an uncompressed block. The
bounded decoder validates the directory, references, output size, integer
code lengths, and overlapping back-references. Synthetic fixtures exercise
malformed inputs; actual archives provide the edition fixtures.

## Pixels

- Palettes store B,R,G triples (not RGB or BGR). First entries agree with
  the fixed palette in the executable's RT_BITMAP resources.
- Hm00: 400 sequential 48×48 indexed tiles, assembled into a 960×960 map.
- Hexzmap block 0: two dimensions in 16-pixel subtiles (60,60), then 400
  terrain IDs for the 20×20 battlefield. Movement costs are now verified below;
  movement range and pathfinding are still unresolved.
- Mmap block 71: 640×400, sequential 8×8 tiles. Pmpalet block 71 palette.
- Face: 64×80, 228 portraits. Unit_mov: 11 frames of 48×48, 151 units.
- Unit_spc: 5 frames of 48×48. Unit_atk: 12 frames of 64×64.
- Pmapobj: 20 frames of 48×64 per direction sheet. The opening's eight
  direction sheets are imported; frames are retained without resampling.
- Item: 104 images, 32×32. U_select: five interface images.
- Meff/Mcall effects are still unresolved and are not represented by fake effects.
- PE RT_BITMAP resources are read as data, preserving resource ID/language/RVA.

## EEX

Complete structural decoding succeeds for R_00 (2,700 bytes) and S_00
(15,497 bytes). The first section offset also bounds the variable-length
section table. S_00 sections start at 22, 7678, and 15468, containing 1, 18,
and 1 events respectively. Every instruction retains its original offset,
parameter tags, values, and end offset. Nested blocks retain their tests.
GBK decoding is strict; malformed/unknown tokens fail with file and offset.

Required edition corrections to historical Style.txt:

- Both variable lists in opcode 5 use parameter tag 53.
- Opcode 75 is a single 28-byte player-position instruction. Fifteen such
  instructions occur in S_00. The old definition incorrectly groups them
  into one 420-byte record.
- The body length excludes the enclosing event terminator.

The complete typed listings are R_00-listing.txt and S_00-listing.txt.
Decoding an instruction does not establish its gameplay semantics.

## Data and randomness

Data block 0 contains 512 32-byte character records. Names are GBK;
face base is little-endian at 13–14, story sprite at 15, base combat statistics
at 18–22, initial HP at 23–24 (cap 500), MP at 25 (cap 250), class at 26,
level at 27 (minimum 1), experience at 28, equipment IDs at 29–31.
The loader at `0x408be3` and portrait selection at `0x4075a7` were executed
against all 512 records for comparison fixtures. With campaign flags/stage zero,
Cao Cao uses face 0, Xiahou Dun face 223, and the other characters base + 7.
This edition's Xu Zijiang has base 206 and face 213; another research edition's
mapping must not be substituted. Block 3 contains 53 27-byte class records. Raw blocks and
unresolved fields are retained; the engine does not invent derived stats.

The executable's RNG at VA 0x47fae7 uses unsigned 32-bit state at 0x48ecd4:
`state = state * 1103515245 + 12345`; output `(state >> 16) & 32767`.
The bounded helper at 0x47fb13 returns zero without consuming state for
bounds below two, otherwise uses modulo. 160 outputs across five seeds
were reproduced using isolated x86 CPU emulation and checked by C++ tests.
Battle call ordering and seed initialization from the original UI are unverified.

An initial disassembly of physical damage at VA 0x43bc4b identifies terrain
scaled attack/defense, level + 25, class multipliers, equipment effects,
counterattack scaling, and a seven-way random adjustment. The random
adjustment maps outputs 0,1,2,3,4,5,6 to 0,-1,-2,+1,+2,+3,+4.
`physical_damage_arithmetic` matches 784 bounded x86 probe fixtures with
explicitly stubbed stat, terrain, class, equipment and hit-result getters.
This initial fixture remains an isolated arithmetic comparison. The 0.6
physical-resolution section below adds unhooked real getters and hit/RNG
checks. Action-level sequencing and presentation are still unresolved.

## Persistent statistics, joining and equipment

R_00's opcode 59 is dispatched through `0x4148fe` to `0x40c28c`.
Joining can reload the character's data, calculate level relative to the party,
apply level growth at `0x4073b8`, update class and initialize equipment.
These calculations now run in the replacement and match the original CPU code:
3,104 growth cases, 5,300 default-equipment cases, seven party averages, and 80
joins. Class bytes 2–6 contain statistic growth factors; bytes 7–8 contain HP/MP
growth; bytes 9–26 are equipment-type permissions. Attribute grades are 1/2/3/4
at thresholds 25/35/45. Each statistic's growth rate is the integer average of
its class factor and attribute grade. Caps are 255 statistics, 500 HP, 250 MP,
and level 50. Joining resets level to zero before applying target-level growth;
NPC battle construction instead adds growth for level minus one.

The party-average routine at `0x40c14f` contains an order-sensitive arithmetic
quirk: it reuses a sorting temporary as the sum's initial value. The replacement
preserves it, including results 5 for levels `[10,1]` and 6 for `[1,10]`.
The NPC promotion-segment calculation is independently checked at levels
1/14/15/16/29/30/31/49/50, for all three promotion modes and 13 opening-related
characters (351 fixtures). These are campaign-stage-zero checks only.

Data block 1 has 104 item records of 25 bytes. Raw byte 17 is type, 18 special
type, 21 strength, 22 special strength, 23 weapon/armor growth or accessory class
restriction, and 24 collection flag. Bytes 19–20 remain unresolved. R_00's
equipment command is executed with its original argument decoder in the probe:
Cao Cao receives item 23, 倚天剑, level 1. Its collection write is recorded as a
local state flag; the original collection file is never written. The original
story handler's weapon-argument removal check also affects armor/accessory;
the replacement preserves that behavior. Full item combat effects remain open.

## Opening deployment and live units

The original handlers decode raw S_00 bytes for opcodes 74, 68–72 and 75.
Twenty-one command checkpoints match, including the selected party, scenario
configuration, all descriptors, hidden flags, position overrides and equipment.
The opening's one-member deployment choice is implemented; arbitrary interactive
deployment constraints are explicitly rejected.

Scenario descriptors at `0x494fd4` have 64-byte stride: ID at 0, x/y at 4/5,
flags at 8, AI at 12, target at 14, promotion mode at 16, level at 17, equipment
overrides at 20/24/28 and weapon/armor levels at 32/33. The opening's level base
is Cao Cao's 3 minus scenario adjustment 2. The trimmed party average used here
is distinct from the joining routine's order-sensitive average.

Live construction `0x44cc85` produces 26 units, compared byte-for-byte after
initialization, alongside all persistent records. Live records at `0x4b2c50`
have 36-byte stride: ID at 0, slot/side at 4/5, position at 6/7, target at 8–10,
lifecycle/flags/AI/direction at 12–15, HP/MP at 16/20, six status grades at 24–29,
abnormal status at 30. Hidden units retain live position (0,0) and lifecycle 1;
their intended positions remain in descriptors. Visible units have lifecycle 2.
Blocked/occupied placement fallback and targeted AI modes are not implemented.

S_00's three opcode-77 commands subtract 80 HP from units 234, 235 and 334,
leaving 10 each. Their actual script handler and HP setter execute in the probe.
Other selection/status/MP forms remain rejected. The current event reaches its
first dialogue at `0x1903`. All section-0 instructions now execute through the victory/defeat text.
The next stop is turn dispatch at the section boundary `0x1dfe`.

## Terrain and battle presentation

Data block 4 holds 27 records of 60 bytes. Each has 30 terrain-effect entries
followed by 30 movement costs. Class group is class/3 through class 38, otherwise
class minus 26. Cost 255 is impassable. Equipment effect 34 reduces passable
costs to one without making impassable terrain passable. Function `0x43f7cf`
matches 3,180 lookups covering 53 classes, 30 terrains and the presence/absence
of that equipment. Movement ranges, occupation rules and cancellation are not
established by this lookup test.

Sprite selection `0x43fc48` matches all 26 opening units. The idle-frame routine
`0x43e572` matches 208 cases (eight tick values per opening unit after scripted
HP changes), including weakened frames below one fifth of maximum HP. This
verifies frame selection, not the wall-clock cadence. The frontend uses original
48-pixel cells. Camera paths and six scripted animations are now verified as
described below; idle cadence and full combat animations remain incomplete.

`tools/probe_vm.py` permits only explicit code ranges and hooks, with an
instruction budget and executable-hash gate. The battle probe omits only tile
redraw, Win32 unit-window update, pixel transformation/copy, and previously
documented archive/collection I/O. Growth, equipment, unit placement, argument
decoding, HP changes and frame selection execute original machine instructions.
No Windows runtime is loaded and no original save or executable is modified.

## Scripted battle animations (0.4.0)

Opcode 80's handler is at `0x416129`; it dispatches to `0x45778f` with unit ID,
animation kind, prewait and postwait flags. The new oracle tests the latter
routine directly, not the script argument decoder. Only visible live units
(lifecycle 2) enter the rendering path. Before rendering, the routine requests
camera centering on the unit. Camera movement is a captured boundary in this
probe; all fixtures hold the camera at tile (0,0).

| Kind | Original resource and sequence | Sound | Internal pause |
|---|---|---|---|
| 0 | Redraw 3×3 region starting at max(relative x−1,0), max(relative y−1,0) | None | None; pre/post flags ignored |
| 3 | `Unit_spc.e5`, frame 3, 48×48, no mirror | 8, flag 1 | None |
| 4 | `Unit_mov.e5`, frame 9, 48×48, no mirror | 8, flag 1 | None |
| 6 | `Unit_atk.e5`, four 64×64 frames, tile position offset (−8,−8) | 7, flag 1 | One tick after each frame |
| 7 | Same attack frames twice, followed by normal unit redraw | 7 before each repetition | Two ticks after each frame |
| 9 | `Unit_mov.e5`, frames 9/10 repeated three times | 8, flag 1 | Eight ticks after each frame |

For attack direction 0, frame indices are 4–7; direction 2 uses 0–3;
directions 1 and 3 use 8–11, with horizontal mirroring only for direction 1.
Kinds 3/4/6/7/9 optionally wait eight ticks before and/or after their sequence.
`0x47eb97` calls imported `GetTickCount` and divides the result by 50, establishing
the original 50 ms timer tick. This does not establish idle-animation cadence
or script-delay units. The original frame renderer adds a 56-pixel top offset;
the frontend now uses this 56-pixel battlefield origin.

The oracle supplies decompressed original atlas bytes at the archive-read
boundary and captures frame bytes before Win32 composition. It executes the
original horizontal mirror loop, hooks only byte copying, and checks persistent
and live unit records for mutation. All 1,920 captured frame hashes are compared
against the C++ selection applied to imported raw atlas blocks. This proves
indexed frame selection, not final palette composition, camera pans, background
restoration, sound-file mapping, playback quality or retained-pose restoration.
The current frontend integrates the verified subset; broader composition and
listening comparisons are still required.

## Camera and sound integration (0.4.0)

The viewport initialization call at `0x44d2ba` supplies (0,56,624,384).
`0x44c38b` flag 0 (new-battle caller `0x44d9b6`) chooses visible party,
then enemies, then allies: opening tile (9,11). Flag 1 is the load path and
chooses party, then allies, then enemies: tile (10,17). The 0.3 frontend used
the load-path focus by mistake; 0.4 corrects it. `0x45503f` centers using integer half-width/height in 48-pixel tiles,
clamps to map bounds, then `0x454cef` recomputes each axis step as
min(4, abs(remaining)/4+1). The original loop adds the step if below the target,
then subtracts it if above; the distance is recomputed on every iteration.
`0x454b7e` changes pixel camera coordinates and requests `0x453b13` full redraw.
1,200 paths cover every focus tile in the 20×20 map from three initial camera
positions, including the prior prototype's unaligned (0,560) input. The core
requires aligned camera positions for current snapshots.

`0x45218f` paints the requested terrain rectangle then calls `0x43eb1f` for
occupied cells; thus a regional redraw restores ordinary unit sprites. The
replacement stores scripted poses separately from unit statistics, clears
those in a redrawn region, and clears all held poses when the camera changes.
No animation queue is allowed in a stable saved snapshot. Restored poses must
match visible units, original sprite selection and a verified animation frame.

`0x474777` formats verified SE06.WAV, SE07.WAV, SE08.WAV, SE16.WAV,
SE35.WAV, SE36.WAV and SE_E_01.WAV.
Original `sndPlaySound` calls are recorded with flags 3 for asynchronous
one-shot playback, preceded by stopping the previous effect. Script wrapper
`0x474a07` mode 0 requests looping (flags 11), 1 plays once and 255 stops.
The frontend has one interruptible effect channel and retains looping-effect
identity in snapshots. Other sound IDs/repeat modes remain explicitly rejected.
SDL decoding/routing is exercised, but listening quality and exact loop seam
continuity have not been verified.

## Removal, facing and new-battle entry (0.4.0)

Opcode 83 handler `0x41557b` calls `0x45a7b7`, then `0x441744` and animation
`0x4419b6`. Only the required individual non-party removal form with mode 0 is
implemented. It sets temporary flag 128, makes the unit visible in the viewport
without recentering an already-visible unit, sounds 16, and toggles occupancy
hidden/shown twice with three-tick waits. The live lifecycle stays 2 during the
blink. Finally occupancy clears, lifecycle becomes 3, status becomes 0 and
mirror/temporary flags clear. HP is not zeroed. Other units targeting that slot
with AI 0 or 4 receive target 255 and AI 1, including a self-target if present.
The original code contains duplicate redraws after showing a unit; the event
fixture preserves them. Player-unit death callbacks and other removal modes
still stop explicitly. `0x454f10` computes visibility-preserving camera targets;
1,200 paths are checked independently of the 1,200 centering cases.

Opcode 79 handler `0x415f65` dispatches `0x457428`; direction selection is
`0x4351da`, mirror update `0x435829`. Vertical distance must exceed horizontal
distance to choose vertical facing; ties choose horizontal. Unchanged facing
returns before any wait/sound, even with reverse requested. Reverse applies
only for target-based turning after that early check. Both-acted flags 2/4 are
cleared for redraw then restored. Tests cover the two removal commands and
seven facing commands from section 0 independently and sequentially, plus
1,596 direction inputs, 160 facing variants and 35 removal variants.

`tools/battle_entry_probe.py` executes the original cleanup basic block
`0x44d8a0..0x44d94e`, using scratch EBP locals and stopping before the next
basic block. It clears live flag 128/status bits 30 and sets nonempty party
side-0 units in slots below 35 to control mode 7. 108 cases cover lifecycle,
slot group, side and flag/status combinations. The surrounding Win32 setup,
fade, sprite cache and full turn loop remain separate/unverified.

Next dispatch investigation: `0x44ed93` starts the turn and calls `0x44c601`
to run scheduled events. Opcode 63 (`0x411652`) compares stored turn-byte + 1
using >= / < / == for modes 0 / 1 / other. Opcode 64 (`0x4116ee`) compares the
current phase byte returned by `0x41c330`. These are disassembly leads only;
no event scheduler is enabled until complete call order/state fixtures exist.


## 0.5 event dispatch, placement and reinforcement evidence

`0x4100a1` scans every event in the current section. It ANDs boolean condition
returns without short-circuiting and copies the cursor after each passing event;
the last saved cursor wins. `0x44c601` bounds dispatch calls by section event count.
`0x411652` compares the one-based turn using modes >=, <, ==. `0x4116ee` reads
controller byte +0x10 (player 0, allied 1, enemy 2). `0x411b50` checks occupied
positions (1025 means live slot <35, not only the player's units). `0x411a54`
mode 1 accepts all eight adjacent tiles, requiring both units visible and HP>0.
`0x4112e9` attribute 7 reads HP even for hidden/removed units, if their slot exists.
`0x411d63`/`0x411d8e` evaluate and then read sticky script outcome blocks 5/6;
these are separate from the battle controller's final outcome. Opening config
has no nominated enemy and nominates Cao Cao as the defeat target.

The original reinforcement handler `0x414f42` first argument is selector mode
(type 0x40), then unit ID, then party slot. Mode 0 uses `0x4570ee`; mode 1
accepts slot <15. Both require lifecycle 1. `0x442b03` sets lifecycle 2,
updates mirror bit 8 from the descriptor, finds placement, writes occupancy,
and redraws. Mode 0 focuses the actual fallback position; mode 1 focuses the
requested descriptor position after fallback. Both use sound 15 and wait 4.
IDs 162..169 and classes 49/50 take an additional apparition animation path;
that path is still unsupported.

`0x43f040` checks the requested tile and its cardinal neighbors, then advances
the center cumulatively in up/right/down/left order. After four such passes it
scans the entire map in row-major order, retaining the nearest free passable
tile by Manhattan distance with strict-less comparison (first tie wins).
No available tile returns (255,255); the portable engine reports a diagnostic
rather than writing that invalid position. No art or rule result is substituted.

Next rule entry points: `0x44ed93` player-side turn start (event dispatch then
`0x43c7d6`, `0x43c975`, `0x44df61`, `0x43cddd`); `0x44e0d9` player input;
`0x44e851` allied/enemy phases; `0x44da7a` AI ordering; `0x43ad1c` AI action.
`0x44eec5` advances turns and handles the turn limit separately from condition 67.
`0x4132d4` opcode 50 calls `0x459487` for battlefield movement. These remaining
execution paths must be recovered, compared and integrated before playability.

### Turn-start recovery rules (0.5 continuation)

`0x43c975` handles grades 0..5 in order: grade >=5 decrements; <=1
increments; grade 4 has chance 10 if base luck >50, otherwise 60-luck;
grade 2 has chance base luck or 100 with effect-20 equipment. Successful
random recovery requests attribute 0..5 value 3 before setting the grade.
Every chance check consumes the original bounded-100 draw.

`0x43c7d6` skips non-visible units. If any status bit is set,
`0x43d108` scans the eight original neighbors for visible class-group-12
units with the same slot-range allegiance (<35 versus >=35), animates all
providers in scan order, then clears encoded mask 0x9e without RNG. Without
a provider, it checks masks 2,4,8,16 in order. The base recovery chance is
`UnitRecord.base_stats[4]` (persistent +0x25), or 100 for equipment effect 20.
Confusion (8) uses max(10,chance-15), except exact chance 100 bypasses RNG.
Other masks always draw. The encoded clear also removes bit128.

`tools/recovery_probe.py` executes the actual rules, neighbor checks and
setters, capturing the status-effect presentation boundary and aura camera
requests. It omits archive/raster/audio/wait presentation work explicitly;
neighbor and persistent records are asserted unchanged. 2,417 C++ comparison
cases include inactive lifecycles, luck boundaries, equipment, all grade values,
combined masks, enemy slot ranges, mixed grades and multiple providers.
These logical effects are not yet integrated into the engine's animation queue.

### AI order and player-phase transition (0.6)

`tools/ai_priority_probe.py` executes `0x44da7a` without rule hooks and compares
3,182 orders. Only live byte +11 changes. Visible units are assigned priorities
in ascending slot order across six passes: terrain 18..22; low HP; groups
8/10/11/18/19/20; groups 2/4/5; groups 0/3/11/13/17/20; remaining units.
The former “weapon predicate” description was incorrect: `0x406906` is a class
group getter. Low HP means nonzero HP <= 2/5 max for IDs <174, otherwise <=1/5,
except job 49 or group 50. The latter is the executable's unusual literal test.
Caller filters (AI !=7, flags 2/4 clear) occur after priorities are assigned.

`tools/player_phase_probe.py` maps the supplied hash-checked AutoDLL image in
Unicorn, with `EnableAutoSave=0`. The patched jump at `0x44e1cd` reaches ASave
`0x100012b0`: no eligible player unit returns through `0x44e6ee`; otherwise
execution stops at the message-pump boundary `0x44e1d3`. Eligibility is a visible
slot <35 with AI7 and flag128 clear. Focus searches party, allies, then enemies
by slot and does not require the focused unit to be player-controlled. 271 cases
compare eligibility and focus. No host DLL loader or Windows process is used.

`tools/phase_transition_probe.py` executes `0x44ed93` and `0x44e851` together
on the post-introduction state with event 0x1e00 completed. It runs the original
selector batches, maintenance, patched player gate and AI priority/filtering.
Each empty batch scans 18 times; focus is (10,17); first actor is 234/slot20;
phase=1 and input-active=0. Unit records and RNG are unchanged except priority
bytes. UI/OS boundaries are explicitly omitted; the probe stops before the
first AI action. C++ preflights maintenance transactionally and advances this
normal path; required recovery/number/XP visuals still stop explicitly.

### Poison and regeneration (0.6)

`0x44df61` applies poison by slot-range faction without a lifecycle check.
Loss is min(max(1,maxHP/10), HP-1), preserving the HP0 -> loss -1 edge.
`0x43cddd` requires visible lifecycle2. Terrains18..22 heal 25/20/20/15/20 percent
of max HP. Equipment effect18 adds maxHP/divisor, effect19 adds fixed MP; the
last matching equipment slot supplies strength. HP/MP requests clamp to maxima.
Only side0 receives effects21/22/23 for unit/weapon/armor XP, multiplied by
max(10,equipment bonus8)/10. Numeric-display call `0x450984` receives slot,255,
HP loss,MP loss,unit XP,weapon XP,armor XP,1; negative loss means healing.
`tools/maintenance_probe.py` captures 1,913 cases with original setters executing.
XP requests are verified below level-up boundaries, not level-up presentation.

### Movement search (0.6)

`0x43f74f`: class movement byte0 plus equipment bonus7, capped255; status2 sets0;
grade5>=4 adds2 (capped255), grade5<=2 subtracts1 (floored0). 2,968 comparisons.
`0x437d59`: cardinal adjacent occupied opposing slot range causes ZOC, unless
an equipped effect33 disables it. Display side and HP are not allegiance tests.
Terrain getter `0x43f7cf` preserves impassable255 even with effect34 (Di Lu).

`0x438308` searches by distance layer, rows top-to-bottom, columns left-to-right,
and N/E/S/W neighbors, assigning each unseen tile once. Friendly occupied cells
remain in the distance grid. Enemy cells block unless ZOC is disabled AND
flag128 is set. With ZOC enabled, controlled non-origin cells do not expand.
The original frontier bookkeeping is reproduced rather than replacing it with
a differently ordered generic pathfinder. Mode2 returns1 immediately when the
target is discovered; ordinary exhaustion returns255. If origin==target, it
returns before clearing the caller's grid. `tools/movement_probe.py` checks
344 whole grids/return values using real opening and explicit synthetic inputs.
Mode8 returns the visible unit referenced by the first newly discovered tile's
threat cell; it does not test allegiance again. Missing/non-visible references
are ignored. `range_probe.py` adds 120 whole-grid/return comparisons, using
original preparation maps and explicit stale/competing references. Mode4 AI
action scoring and movement presentation remain unresolved. No player movement
command is yet enabled.

### Route reconstruction and fallback scores (0.6)

`0x437e2e` descends the existing distance grid from destination toward origin.
It considers N/E/S/W, replacing the selection only for a strictly lower cost.
It charges the newly considered tile's movement cost against the supplied
budget, and skips controlled tiles except the origin or with flag128 set.
After descent it reverses directions and adds2 modulo4 in caller-owned scratch.
There is no appended terminator. On failure, the original loop increments its
count once without writing a direction, so one pre-existing byte is included
in the reverse/invert operation. The API preserves scratch explicitly instead
of inventing a normalized path. Movement callers must stop at their endpoint.

Failure with score flag1 invokes `0x438178` at the reached tile with value20
if occupied, otherwise10. A zero score is filled only on empty tiles or the
actor's current tile. Recursive cardinal values are value-10 above10, otherwise
value-1, stopping below9. Jobs15/16 stop after the center; this is the class ID,
not its class group. Existing nonzero cells are preserved. The terrain-factor
getter's !=255 test is always true because that getter maps255 to0. There is
no terrain-cost or occupancy check on recursion itself. 550 unhooked original
cases compare the entire scratch buffer and score grid, with no unit/RNG changes.

### Range shapes and AI preparation (0.6)

`0x43f8be` selects class byte1, overridden by equipment effect42 (last strength).
`0x436586` enumerates ordered shapes0..15; range255 returns before mutation,
and the range high bit is stripped. Mode bit1 includes the center and selects
allies; otherwise only opponents are collected. Slot ranges define allegiance.
Mode bit128 preserves distance/threat grids. Without it both grids receive the
actor slot at every enumerated tile. A nonzero mask argument instead sets the
separate mask grid and skips collecting targets. Candidates require occupied
tiles and HP>0. Return selection is the preferred slot if present, otherwise
the last collected slot, or255. Order and duplicate visits are preserved.
1,376 cases compare all output grids, ordered target lists and return values;
106 cases compare all class range getters with and without effect42 equipment.

`0x43a28a` clears the threat grid to255 and paints visible opposing units in
ascending slot order using the ACTOR's range, overpainting the distance scratch
without clearing it. Scores reset to0. Low-HP actors whose AI is neither2 nor4
receive flag1 and score50 on terrain18..22. Best score resets0; target/tactic
and best-tile validity byte reset255. 1,596 cases cover original actors and
class/HP/AI combinations. No AI action or RNG draw executes in preparation.

### Physical hit chance and returned damage (0.6)

`0x43f4fe/0x43f595/0x43f5e3/0x43f631/0x43f67f` read persistent stats0..4 plus
equipment bonus0..4, capped400 for the first three or300 for agility/luck.
Grade>=4 multiplies12/10; grade<=2 multiplies7/10; final values cap400.
`0x43f84d` reads class-group terrain affinity at group*60+terrain, mapping255
to0. The equipment predicate preserves zero-strength effects; its strength
getter uses the last matching equipment slot.

Physical chance `0x43b50d` first blocks ranged groups2/4/5 against effect56,
then returns100 for a confused defender. Otherwise, for attacker agility A and
defender D: A>=2D gives100; A>=D gives90+(A-D)*10/D; A>=floor(D/2) gives
60+(A-floor(D/2))*30/floor(D/2); remaining cases give
30+max(0,A-floor(D/3))*30/floor(D/3). All divisions truncate. Effect39 adds a
fixed chance capped100; defender effects53 and55 subtract in order, floored0.
The original pathological zero-divisor input raises an explicit diagnostic
instead of inventing a chance. The opening records do not encounter it.

`physical_resolution_probe.py` executes these getters and `0x43bc4b` without
rule stubs. Randomized damage always draws bounded100 for hit, including
chance0/100. A miss returns0 without the bounded7 damage draw. Nonrandomized
estimates bypass the hit check and all RNG. Effect61 reduces ranged damage;
the older arithmetic fixture's `mounted_resistance_percent` is a legacy label.
2,184 five-stat cases and 2,106 full chance/damage/RNG cases pass. This numeric
return is separate from action-level critical/double/counter selection,
HP/XP application, death triggers and animation, which remain unresolved.

### Single-target AI attack score (0.6 continuation)

`0x438965` collects opposing targets in the original range order, evaluates
`0x438a20`, and retains the first strictly highest nonzero score. The physical
estimate has randomization/counterattack disabled and reads the actor's actual
current terrain, not the proposed destination. Base score is80 for a kill;
otherwise16 for damage of at least one tenth of max HP or target HP at most40%
of max; otherwise10 for AI1, or2 for other modes. Confusion adds8. Named IDs<174
add8 for slot35 or the priority ID, otherwise4. Priority ID comes from the
16-bit field at reader+0x3d06; the isolated API accepts it explicitly.

Class groups8..11 and18..20 reject scores below75. Job16/17 uses area1, job43
uses area4, and other jobs may use equipment effect43; nonzero/non255 areas
remain an explicit unsupported diagnostic. Normal scores above1 lose1 when
the candidate tile lies in the target's attack-range mask. This is a range
penalty, not a simulated counterattack. 1,720 unhooked original cases compare
score and selected target; a separate test requires the unsupported area path
to stop. No live/persistent record or RNG changes during scoring. Tactic
scoring and area effects remain open; combined decisions are covered below.

### Tactic loader and combined AI decision (0.7.0)

`Data.e5#5` is 73 records of 70 bytes. Original loader `0x41b84f` calls
`0x40ddaf` for only the first 68. It copies name bytes 0–9 and writes a NUL at
runtime byte 10, skips disk byte 10, and maps disk bytes 11–16 to runtime
15,11,12,13,14,16 respectively. The 53 class-required-level bytes at 17–69
copy unchanged. These are not raw runtime records; copying the whole disk
record directly would read the wrong tactic cost/range. The importer exports
raw bytes, decoded GBK names and the normalized runtime fields.

`0x438308` mode 4 scans tactic IDs 0–67, admitting each whose required level
for the actor's job is nonzero and no greater than its level, and whose MP cost
(runtime byte 14) is affordable. 2,226 original cases cover 53 classes, seven
levels and six MP values. This does not establish tactic effects or scoring.

Mode 4 scores the origin before its passability test unless flag128 is set,
then every newly reached cell in original search order. `0x43a529` rejects
occupied cells other than the actor's current tile. Physical and tactic scores
are compared, and their maximum is added to the byte preference **with wrap**.
Terrain bonus is 8 on terrain IDs 18–22, otherwise max(0,current-terrain-factor
minus 10); this addition saturates at 255. The current tile gains another 1
when physical score is zero. Excluded target tiles are forced to zero, and
only strict improvements replace the chosen action, preserving ties.

`ai_decision_probe.py` executes original `0x43ad1c` through entry to movement
`0x43a956`. 129 cases compare complete grids, search/candidate order and
selected destination/target. The generic port covers low-HP leader seeking,
ordinary seeking, hold/defensive and targeted modes, target invalidation and
scripted-movement flags. The original allied low-HP branch uses hidden slot
0's coordinates; this is why a visible-leader-only simplification diverges.
Unported nonempty tactic and area scoring still fail explicitly. Inactive
units return before movement. Unused scratch targets for movement-only units
are normalized; they are never executed as attacks.

### Walking and AI movement (0.7.0)

`0x44201b` emits 48-pixel tile steps with one original tick before each frame.
Ordinary classes advance 48/12/6 pixels at settings 0/1/2; class groups
0,3,4,11,13,17,20 advance 48/24/12. Walking selects frames 0/1 south, 2/3 north,
4/5 horizontal, choosing the second frame after pixel 24. East sets flag8;
west clears it. North/south preserve the flag but their pixels are not
mirrored: `0x43fd34` only transforms horizontal frames 4/5 and idle frame 8.
The oracle now runs that pixel-transform loop and verifies emitted pixels
against decoded original frames, instead of inferring pixels from the flag.

Viewport-edge steps request one-tile scrolling before emitting walking
frames. Original old/new occupied-tile redraw order depends on direction;
the port records these overlay requests explicitly. 1,749 cases cover all
53 jobs and three speeds at central and edge positions. Host raster calls,
background copies, message pumping and camera raster composition remain
separate presentation boundaries.

`0x43a956` rebuilds its route from the decision distance grid, issues selection
or camera focus, removes starting occupancy, and calls walking for each route
direction until the exact destination. It then sets flag4 unless flag128 is
active, restores occupancy and requests redraw. 45 original cases compare
unit state, occupancy, viewport and presentation requests. Selection UI
`0x441488` is a captured boundary here, not a stubbed game rule; its separate
selection/display comparisons below support the 0.7 controller integration.
No attack, experience, death event or phase completion is implied by these
movement comparisons.

### Selection display and normal unit panel (0.7.0)

`selection_probe.py` captures 280 original `0x441488` sequences, retaining the
initial camera/cursor, optional normal panel (omitted after moved flag4), two
passes, tick increments modulo 8, poll-before-wait behavior and cleanup order.
Mode 0 adds the movement and attack overlays in pass 2; attack/tactic modes
show their range in pass 1 and focus the target/preview panel in pass 2.
Early input at either poll skips that pass's four-tick wait. The controller
currently uses the uninterrupted mode-0 branch and a provisional 50 ms tick.

The normal panel (`0x4404bd` mode 0) is four by two 48-pixel cells. Placement
branches at viewport-relative x=6 and y=4. `panel_probe.py` executes the real
formatter and GBK cursor through `0x40f548`, where glyph rasterization is
recorded. It verifies exact text positions, colors, background mode and HP/MP
bar requests for 265 inputs, including every class and terrain. Name cache
`Unit+0x37..0x46` removes ASCII digits; no other unit bytes or RNG change.
Bar fill uses floor(floor(current*100/maximum)*128/100), not direct division.
The native/browser frontend uses the imported font at these positions; the
Windows font's bitmap shapes and full original dialogue UI are not reproduced.

`overlay_probe.py` compares 136 original display-range masks and 384 tile-layer
sequences. The order is map, reachable checker/range mark, cursor, then unit.
The original checker affects pixels with equal x/y parity. Mark.e5 is an
uncompressed 116,704-byte indexed pixel resource: HP icon offset 384 (24x24),
MP icon 960 (24x24), HP/MP patterns 128/64 (8x8), range 0xcc0 (48x48), neighbor
cursor 0x15c0 (48x48), center cursor 0x1ec0 (48x48).

`palette_probe.py` runs original nearest-palette lookup `0x42fb10` and darkening
`0x430480` for all 256 entries. The original palette input is quantized by
shifting each component right four bits; matching considers indices 10..245
and requires both maximum-channel error and total error to improve or tie.
Later equal matches win. This verifies the panel darkening lookup; the final
Windows palette upload/color-depth conversion remains a separate unknown.
`ui_assets.py` reads Chinese class/terrain names and panel labels directly from
the hash-checked PE image. The importer adds those glyphs to the atlas and
preserves original Mark pixels without loading any Windows library.

### Physical strike preparation and counter gate (0.7.0 module evidence)

`strike_probe.py` executes `0x405e03`, its real damage/getter/experience calls,
and `0x406181` without rule hooks. The caller supplies the ordered target list;
area-target assembly and complete action application are outside this probe.
1,297 cases compare HP/MP damage arrays, primary critical flag, actor XP/weapon
XP maxima, target armor XP and RNG before/after the double roll. The actual
first archer/target pair is case 0. All live/persistent bytes remain unchanged.

Critical (`0x405d69`, luck) and double (`0x405ce6`, agility) chances share a
piecewise ratio: 100 at >=3x, 20..99 between 2x and 3x, 2..19 between 1x and
2x, otherwise 1. Both use equipment/grade-adjusted battle stats. Effect41
guarantees critical. 392 boundary/equipment cases compare these getters.
Only the primary target receives a critical check, and only when its HP is
nonzero. The check still occurs after a miss. Critical multiplies already
HP-capped base damage by 3/2, followed by another HP cap. Effect57 cancels its
damage but retains the critical flag. Effect58 cancels second-strike damage.
Effect60 with any remaining MP redirects the hit to min(MP,damage) and sets HP
loss to zero; excess damage does not spill into HP. A counter or second strike
skips the double roll. A synthetic zero-divisor input faults at `0x43b66a` in
the original; the port raises an explicit diagnostic without consuming RNG.

Actor experience requests depend on displayed side 0, the target's level and
whether the planned HP loss reaches its current HP (including the zero-HP
edge). Kill XP is 32 plus eight per higher level (cap 200), or minus four per
lower level (floor 10); priority-ID kills double it. Nonkill XP is 8 plus two
per higher level (cap 100), or minus one per lower level (floor 1). Equipment
stat8 scales XP by max(10,value)/10. Weapon XP applies only to physical weapon
types; armor XP depends on target side 0. Across targets/strikes the controller
retains maximum actor/weapon requests, not their sum. XP application, level-up
and equipment XP mutation have not yet been connected or established here.

`counter_probe.py` executes `0x40639a` with real range, HP, confusion and
equipment getters. Its 958 cases cover every class and ranged/melee distances,
dead/confused targets, effect44 counter prevention and effect40 countering a
counter. Depth >1 resets the shared counter depth; at depth 1, absence of
effect40 also resets it. Eligible targets must find the original attacker via
the original preferred-target lookup. Facing `0x435829` and recursive attack
`0x4064f1` are captured requests, not executed effects. This does not establish
recursive damage, HP/death processing or animation.

Next dispatch investigation: `0x4064f1` calls initialization `0x4061fc`, then
strike preparation `0x405e03`, presentation `0x405744`, application `0x405889`
and double selection `0x406181`. Actor XP finalization `0x405c6c` and the
counter gate follow. Crucially, presentation `0x405744` itself contains a 25%
RNG check when the critical flag is set (`0x40582a..0x405838`) before an optional
speech. Therefore the strike fixture's plan-then-double RNG result must not be
treated as a complete attack RNG trace. Those intervening routines still need
their own original-code probes and must preserve any additional random draws.

### Attack application and experience (source work after 0.7)

`experience_probe.py` compares persistent records through `0x408950` (2,264
cases) and `0x4082d6` (2,187 cases). Character XP is an absolute total, equipment
XP an increment. Characters with ID >=26 or level >=50 ignore character XP.
Every 100 points calls original growth, including repeated cap-level requests
when one grant crosses level 50. XP becomes 255 at the cap. Equipment uses cap
9 for odd item types below 18, otherwise 3; accessory/empty slots ignore XP.
All original growth and getter instructions run. Equipment-level windows and
unit-window refresh are captured requests. Character growth uses the original
no-animation argument: this does not prove level-up presentation or live HP.

`strike_application_probe.py` executes `0x405889`, HP/MP setters and armor-XP
application in 1,957 cases. A nonzero HP or MP request enables status application.
Equipment effects 35/36/37/38 add confusion/poison/paralysis/seal respectively;
classes 50/49/42 independently roll 60% for the first three effects, even when
the bit was already present or supplied by equipment. Numeric display is
requested before HP/MP mutation, followed by armor XP. The original unsigned
setters cap after subtraction; even synthetic underflow cases match.

Effect 62 triggers automatic item 87 only after positive signed HP damage with
remaining HP. Displayed side 0 additionally requires a positive inventory
count; other sides request free use. The probe **suspends before `0x41dc43`**
in eleven cases. The returned `pending_item` result similarly represents that
boundary, not a successful item action. The caller must execute the item and
then clear flag 32 before advancing to another target or the double/counter
gate. Non-item paths clear flag 32 directly, including misses. No death or
battle event dispatch is implied by these numerical mutations.

`critical_speech_probe.py` checks the presentation RNG gap in 5,120 cases.
Critical strikes roll 25%; no roll occurs when the critical flag is false.
The table at `0x48b230` maps 21 named heroes to Imsg.e5 records 700–720, without
another random draw. Other characters use `721 + group*3 + rng.bounded(3)`.
Group is source hero byte 16, copied to persistent offset 0x26. The original
message reader takes 200 bytes, stops at NUL and expands literal `\\n` before
display; all 1,024 selected speeches match GBK bytes and decoded Chinese.
The probe explicitly captures facing, camera, archive, redraw and animation
boundaries; it establishes speech selection/RNG only. `combat_assets.py` is a
read-only decoder for this text, pending inclusion in a later asset manifest.

`combat_palette_probe.py` compares all 4,096 quantized RGB lookup entries at
`0x42fb10`. This extends the earlier darkening-only palette check for attack
color effects. Windows display palette upload remains outside that probe.

`attack_animation_probe.py` then executes original sprite preparation
`0x403b8f`/`0x403d85` and the complete single-target `0x4044e9` loop at those
explicit raster boundaries. Its 189 cases cover all 53 actor classes, four
attack directions, hit/miss/MP-only/zero-HP targets, critical flags, lethal
requests and tick wrap. All 2,830 emitted frames are compared as indexed bytes,
including original mirrored and transformed sprites. The replacement emits
the same ordered frame, sound, wait, overlay and number requests and final
unit state/tick. Persistent unit records and RNG remain unchanged by this
animation oracle. It does not invoke the speech or HP-application routines.

The animation loop has 27 steps; activity occurs at steps 0, 2, 8, 12–15, 17
and 25. Each step waits one tick. Critical charging adds six waits/brightening
operations (1,1,2,2,3,3), each quantized against the prior transformed pixels;
adding the six amounts and transforming once is incorrect. Bow groups 2/4
add eight ticks after charging. Hit reactions brighten by 12, then optionally
tint the next frame using original parameters (15,15,4). Transparent index 0
is preserved. The target's low-HP flag can change before actual HP mutation,
and miss reactions update facing. Secondary targets and exact frame-by-frame
state handoff to the renderer still require work.

### Integrated physical controller (0.8)

`attack_controller_probe.py` executes `0x4064f1`, target assembly `0x4061fc`,
planning `0x405e03`, presentation `0x405744`, application `0x405889`, double gate
`0x406181`, XP finalization `0x405c6c` and recursive counters through `0x40639a`.
The 73 cases contain 121 strikes, 40 counters and 8 second strikes; 1,536 frame
hashes compare pixels while the original mirror/color routines execute.
Critical display `0x45965d` executes its actual wrapper, including recentering
through `0x45503f`. Text window `0x42d5f1`, input flush, scrolling, background
raster I/O and host waits are captured boundaries. No DLL is loaded or executed
by an operating-system loader. Automatic item use and level windows suspend.

The integrated controller is limited to single-target physical attacks. Class
16/17 implies area 1, class 43 area 4, otherwise equipment effect 43 supplies
area strength; any nonzero area stops before caller-state mutation. Ordinary
NPC XP requests resolve to zero. Party numeric/XP panels and animated level-ups
are not accepted. Strike records are simulated on copies and presented in order;
no snapshot is accepted while that action remains pending.

The first allied archer's actual runtime selection/movement ticks end at 4;
its attack increments at animation steps 0 and 8 to final tick 6. Its preview
uses deterministic damage 28 and chance 90. Actual critical damage is 45,
critical speech 743 is selected, target HP becomes 45, actor flags become 6,
and final RNG is 2516284547. The controller oracle uses an isolated tick-0,
flags-0 input; integrated tests separately compare the final relevant results
and selection tick contributions. This is not a full original interactive trace.

`attack_panel_probe.py` compares 264 preview panels. `attack_camera_probe.py`
compares 10,816 panel-avoidance positions and 3,200 camera destinations. Panel
avoidance deliberately preserves the original vertical stop's use of the
13-tile viewport width, rather than changing it to the 8-tile height.
`damage_panel_probe.py` executes the NPC branch of `0x450984` in 32 cases. Its
five displayed values are `current - loss + trunc(loss/4)*n` for n=4..0;
unsigned HP/MP percentages and negative healing requests match. Player branch
`0x450036` remains an explicit stop. Glyph rasterization is not in that oracle.

The importer now includes Imsg records 700–750 with raw byte provenance, attack
labels from hash-checked executable addresses, Mark.e5's loss pattern, and the
quantized combat palette. The frontend applies each sequential color transform
and preserves transparent index 0. Exact background restoration/composition,
font shape and wall-clock cadence still require original visual comparison.
The original number routine `0x44faf1` uses `%u`, 8-pixel glyph advances and
unit-tile coordinates relative to the camera; its separate full-width formatter
call does not supply the rendered string. Floating-number raster placement has
been inspected but is not yet covered by a dedicated glyph oracle.

### Post-action caller investigation (not connected in 0.8)

`post_action_probe.py` executes `0x44e851` around an explicit first-action input
boundary. The archer's separately compared move/attack result is installed at
`0x43ad1c`; that AI call itself is not executed by this oracle. Everything after
its return runs through the original action-event wrapper `0x45b5fe`, original
notification `0x45b910`, death scan `0x44cc03`, redraw wrapper `0x43fcc9`, event
selector and next-actor filters. Only the OS message request, cursor, regional
raster draw and the two AI entry boundaries are hooked after setup. The allowed
hook set is asserted. A six-million-instruction budget bounds the CPU probe.

The action wrapper checks the battle **outcome** byte (not the phase). For a
live actor and unfinished battle it stores unit ID and tile in script storage
block 3, then runs an event batch. The caller next scans slots 0..114 and invokes
`0x441744(0,0)` for lifecycle-2, zero-HP units. It then redraws the acted unit and
runs another event batch when its acted flag remains set and the outcome is
unfinished. The first attack is nonlethal, so no death handler executes here.

This trace has three event batches of 18 scans: allied entry and two after the
first attack. Priority ordering executes once. The next selected actor is slot
18, unit 210, with unchanged RNG 2516284547. The original caller only recomputes
priority when controller field +0x504 is set; blindly sorting after each action
would not preserve this behavior. Capturing this trace does not yet establish
death scripts, notification-dependent conditions, reordering triggers or full
phase completion. The 0.8 runtime still stops before this sequence.

## 0.9 allied caller and NPC defeat

`0x44e851` keeps rank bytes across allied actions. `0x45b5fe` writes the
actor ID and coordinates to script storage block 3, then calls the bounded
event batch. `0x44cc03` scans live slots in ascending order for lifecycle 2
and HP 0. After this scan, a completed actor is redrawn and another event
batch runs. The continuous oracle records seven batches of 18 scans for the
first three allied actions and stops at the enemy-turn banner.

`0x441744(0,0)` on non-party NPCs requests visible camera framing, sound 16,
two invisible/visible blink cycles with 3-tick waits, final occupancy removal,
lifecycle 3 and status 0, minimap refresh, sprite refresh and mirror reset.
Followers targeting the removed slot switch from AI 0 or 4 to AI 1 and clear
their target slot. Unlike script opcode 83's outer wrapper, combat defeat does
not set or clear flag 128. No persistent record or RNG changes occur in the
302 covered non-party cases. Party retirement, alternate arguments and the
separate minimap window remain outside this verified implementation.

Portable snapshots retain `script_fields.ai_order`, `ai_cursor`, `ai_pending`,
`event_return` and `last_action`. Event return values in this build are
`ai_deaths` and `ai_pick`; malformed orders, targets, coordinates and return
points reject before changing live state. Saves during pending animations
remain unavailable.

## 0.10 enemy banner and first reinforcement event

`0x44ac28(turn, enemy, preserve_flags)` reads viewport columns/rows from the
camera. It generates a 48×48 alternating checker (45 for player, 63 for enemy),
then redraws one column at a time: increasing x for player, decreasing x for
enemy. Enemy text comes from `0x48e160` (敌军阶段); player text from `0x48e154`.
Requested font height is 48 below a 720px viewport and 64 otherwise. The two
shadow rectangles shift all edges by 2 and 1 pixels. Enemy colors are 58/53/18;
player colors are 42/36/18. The supplied client rectangle determines centering.

Player banners wait 10 ticks, shift the rectangle's top edge by `2*height+16`,
and show `第%2u回合` (`0x48e178`) or `最终回合` (`0x48e16c`) when the low byte of
turn is at least the configured limit. All banners then wait 20 ticks. If
preserve_flags is zero, `0x406690(6)` clears action/movement flags for **all 115
slots**, without checking lifecycle. Minimap refresh follows. Checker color
becomes 255 and the same column sweep restores the battlefield. This sequence
must not be replaced by a cosmetic title plus a phase-number change.

`0x44e851` calls the banner with `(turn_byte+1,1,0)` before setting phase 2 and
dispatching events. The opening state selects event `S_00.eex:0x1ea4` immediately,
before enemy recovery, priority assignment or AI. Its three opcode-76 calls
bring in IDs 32, 3, 33; camera becomes (336,0). The two opcode-79 calls face
Guan Yu and Zhang Fei toward Liu Bei. The port reaches the first opcode-50 move
at `0x2033` with the same roster/RNG/tick/camera. `original_enemy_entry.json`
documents that the event body is a command-level oracle, with text as a boundary;
its original selector and preceding allied/banner execution are continuous.

96 isolated banner cases verify checker hashes, rectangles, labels, waits,
column order and flag changes. Font rasterization and original client dimensions
remain unknown. The engine clears the finished allied scheduler fields before
starting this event so stable dialogue snapshots cannot repeat allied actions.


## Scripted entrance movement (0.11.0)

Opcode 50's original handler `0x4132d4` resolves the unit selector and calls
`0x459487`. The three commands at `0x2033`, `0x2053`, and `0x2073` target unit
IDs 32, 3, and 33, with destinations (11,5), (10,5), and (12,5), facing south.
This edition uses the same AI route search and walking renderer for these
scripted moves. Flag 128 gives a movement budget of 100 and bypasses attack
selection/resolution. The original route descent still uses its own 255 budget.

The wrapper saves the AI mode, flags and target coordinates; clears moved/acted
bits 6; enables flag 128; and assigns AI mode 0. An occupied destination uses
`0x43f040` placement fallback. `0x44268c` assigns coordinates and clears the
followed target slot. After walking, `0x440252` changes a coordinate-following
unit that reached its target to AI 3. The wrapper then restores the old AI,
clears temporary bit 128 and ORs the old flags except mirror bit 8. Restoring
coordinates through `0x45b6d0` leaves target_slot at 255: it does **not** restore
the old followed unit. Direction 0..3 runs the original facing/mirror update
and unit redraw; other direction values leave walking's facing intact.

`tools/script_movement_probe.py` executes the original handler for the actual
three commands and the original wrapper for 35 explicit synthetic variations.
It checks 38 paths, intermediate/final unit states and camera positions, 484
indexed frame hashes, wait requests, tile overlays, destination collisions,
action flag restoration, invalid destinations, direction sentinels and an
immobilized unit. No RNG or persistent record mutation is allowed. Synthetic
flag changes also mirror the input pixel buffers, preventing an inconsistent
reference sprite/flag pair.

The probe starts from the original allied caller and the command-level
reinforcement appearance/facing continuation. It explicitly captures raster,
wait, camera and OS requests; it does not measure wall-clock timing or replay
an original Windows dialogue window. After the three entrance moves it invokes
all 80 enemy slots' status/grade recovery, then poison, regeneration and
priority routines in the order at `0x44e8e7..0x44e9b3`. This is a source-ordered
routine continuation, not a complete original interpreter/caller playthrough.
The resulting visible/hidden/retired unit state, persistent records, camera,
RNG and tick match the integrated engine. No enemy action has yet completed.

The active-unit movement branch is implemented. Hidden/retired-unit movement,
invalid saved target coordinates and party-slot selection stop explicitly;
they are not silently treated as successful moves. The first enemy action
currently stops at tactic scorer `0x4391dc` (tactic 23), with the source event
end reported as `S_00.eex:0x20a1`. Player commands, movement cancellation,
tactic effects and full battle outcomes remain unresolved.

## Opening enemy tactic decisions (0.12.0)

`0x4391dc` enumerates the admitted tactics, then targets in the original range
order. The opening leaders' class 30 / level 5 / MP 48 admits 23 (钝兵),
25 (虚脱), and 33 (毒烟). All use range 13; the first two have area 1 and
毒烟 has area 0. `0x4395ae`'s actual switch returns zero for 23 and 25: this
is verified original behavior, not a missing-effect fallback. They consume no
RNG while scoring. Actual application of those debuffs remains unimplemented.

For 毒烟, `0x4390ef` requires opposing allegiance and `0x420422` rejects an
already poisoned target unless the caster has player AI 7. Its hit chance
(`0x43b73f`) uses intelligence plus morale, the original piecewise ratio,
equipment effects 52/54/55, then a 9/10 multiplier. An immobilized eligible
target has chance 100. 钝兵 and 虚脱 target usability checks grade 3 and 4
respectively for equality to 2 (not less-than-or-equal), with the same AI-7
exception.

The scorer calls `0x43bfaa` with its final argument **zero**, so hit and damage
variation consume the original RNG during candidate evaluation. Damage starts
at level+25 with one-third of the intelligence difference added/subtracted,
then scales by 7/10. Target jobs 39/42/43 multiply by 11/10, jobs 49/50 by
13/10. Equipment effect 59 reduces damage before the bounded-7 variation.
The minimum is 1 and maximum is current target HP; a miss returns zero.

The individual score is 76 for a lethal result, otherwise 30/10/1 according to
whether damage is at least 30%/10%/less of maximum HP. A nonlethal result gains
8 when the target's byte-truncated HP percentage is at least 40. The outer
scorer adds 10 for the actor's followed target, stores a byte, and preserves
the first target on ties. `0x43a529` adds the larger physical/tactic score to
the candidate tile (byte wrap); physical wins an action tie. Decision RNG must
be committed even when that candidate/tactic is not ultimately selected.

`tools/tactic_scoring_probe.py` verifies 558 pair evaluations, 280 candidate
scores, and 24 whole AI decisions, with no rule hooks and no persistent/unit
mutations except the decision routine's documented actor target cleanup.
It includes poisoned/immobilized targets, AI-7 exceptions, grades, all 53 target
jobs, HP thresholds, equipment 36/47 and multiple RNG seeds. Other admitted
tactics remain explicit diagnostics. The port does not infer their rules from
these three branches.

`tools/enemy_phase_probe.py` continues the original reinforcement/maintenance
VM through the first actual enemy decision and walking. Slot 35 / ID 146 moves
from (9,11) to (9,13), selecting tactic 33 against slot 20. RNG becomes
835903122. Every represented live/persistent unit, RNG, tick and camera matches
the integrated engine at decision and movement completion. The reference stops
at `0x43b3a2` before tactic selection/application; raster/window/wait/audio
requests are explicit boundaries. The portable engine stops at tactic
resolution with `S_00.eex:0x20a1`; this is the event-end scheduler diagnostic,
not an unsupported instruction at that byte offset. Next: selection mode 2,
`0x43dc99`, damage/status/MP/XP application, animation, and post-action events.


## Opening enemy tactic preview and planning (0.13.0)

`0x441488` mode 2 runs the existing two-pass selector without changing facing.
After its range pass, it clears the panel, focuses the target, marks that tile,
and invokes `0x4404bd` mode 2. Preview damage calls `0x43bfaa` with final arg 1
(no randomness), while hit display uses the reversed-argument branch of
`0x43b73f`. Tactic 33 draws a loss-pattern HP bar and 损伤; 23 and 25 draw the
unchanged bar and their type labels 敏捷下降/士气下降. All display 命中. The
28 tactic-type strings come from PE pointer table `0x48bd40`, with damage/hit
labels at `0x48e0a4` / `0x48e0b0`. No UTF-8 string length is used for original
GBK layout: ASCII advances 8 pixels, Chinese 16. The 246 panel fixtures execute
original formatting, positioning and rule getters, capturing raster requests.

`0x43dc99` calls `0x425627` (controller at `0x497af8`). `0x4251e1` initializes
caster pointers, range/area bytes, XP and target arrays. The enemy selector
`0x45533d` returns the AI's target slot. For poison, `0x423549` checks target
usability, prepares targets with `0x425349`, rolls status via `0x43b4b0`, then
independently calls randomized `0x43bfaa` for damage. A successful status roll
stores **0x70**, which is an effect-application code, not the live status mask.
Its HP damage is stored at controller+0x84; targets are at +0xd, effect codes at
+0x255. NPC actor/target gates return zero experience at +0x2d0/+0x2d4/+0x2d8.
The port stops explicitly for party experience instead of inferring zero.

The source-ordered first enemy continuation reaches target slot 20 with a
10-damage/0x70 plan and RNG 1775811865. The 181 fixtures vary seeds, HP and
status, include an already-poisoned target exiting before animation, and assert
that all live/persistent units, tick and camera remain unchanged during planning.
No HP or poison is applied yet. Animation starts at `0x4214bf`; target effects
are applied later by `0x4224a8`, then caster finalization runs at `0x422f0d`.
The plan tests are not evidence of those later routines or a completed action.


## Opening caster pose and tactic caption (0.14.0)

`0x4218d6` reads the caster's original `UNIT_ATK.E5` block, waits 8 ticks,
requests sound 8 and centers the camera on the target via `0x45503f` (including
a scroll-wrapper call when already centered). It clears the cursor, applies
`0x435829` facing/mirror state and redraws the caster. The 64×64 caster frame is
0 facing down, 4 facing up, or 8 facing horizontally, with an original pixel
mirror when facing right. Its top-left is the tile's screen coordinate minus
8 pixels on both axes. Sound 39 accompanies the tactic-name caption. The original `0x47467a` filename
and API-flag probe verifies `SE39.WAV` with flags 3, for both caller flag inputs.

`0x42f699(2,"%s",name)` runs the original formatter and `0x42f433`. For the
three admitted two-character tactic names, the 624×384 battle viewport produces
a 96×48 window at (264,144), with glyphs at (296,160) and (312,160), foreground
palette index 172, transparent glyph backgrounds. `0x473202` paints the original
48×48 `Mark.e5:0x18dc0` tile and eight border lines (indices 131/133/135/128).
`tools/caption_window_probe.py` verifies all tile bytes and line requests for
three widths. The importer preserves this tile without generated substitutes.

The caption wait is `(message_speed+1)*10` ticks. The caster waits any remaining
part of 20 ticks after the caption closes, then restores the captured caster
background. The deterministic engine uses the original default message speed
0 and ten elapsed caption ticks, keeping host rendering latency out of state.
Exact original wall-clock playback remains unverified. The clock is an explicit
input in the reference probe, covering 0/1/19/20/21/100 elapsed ticks.

`tools/tactic_cast_probe.py` continues the source-ordered original enemy walk,
selector and poison plan. Its **235 cases** cover 53 classes, four directions,
three caption speeds, changed/unchanged camera focus, actor flag variations,
and the names 钝兵/虚脱/毒烟. Original sprite mirror loops and GBK cursor execute;
window/font/raster/audio/wait and host message pumping are explicit boundaries.
All persistent unit bytes, RNG and battle tick are unchanged. The first real
caster returns with camera (144,576), RNG 1775811865, and no applied HP/status/MP
change. This is not a completed tactic or recorded Windows playthrough.

The next call loads `MEFF.E5` record 23 through `0x4014f8`. Initial read-only
inspection finds a 20-byte header, 13 20-byte frame descriptors and 13 indexed
75×90 images (88,030 decoded bytes total), but descriptor interpretation and
playback callbacks are not yet verified or shipped. `0x401e5c` invokes
`0x421f04` during playback. Target application (`0x4224a8`), caster finalization
(`0x422f0d`), action flags and post-action events remain pending.


## MEFF#23 poison player — 0.15

Verified against this edition's `0x401000` parser, `0x4014f8` loader,
`0x401c88` target collector, `0x401e5c` player and `0x421f04` callback.
The 20-byte header is `<4B4I>`: 13 descriptors, 13 images, zero masks,
zero palettes, 100 ms, width 75, height 90, sound offset 19. Each descriptor
occupies 20 bytes: image index, palette index (255 = global), blend weight 7,
blend amount 16, u32 masked flag 0, signed dx 0, signed dy -14, callback byte
(1 on frame 1, otherwise 0), and three unused/preserved padding bytes.
Images follow the descriptors as 13 contiguous 6,750-byte indexed rasters.
Total decoded size is 88,030 bytes. Sound ID is 100 + 19, mapped by the
original sound routine to SE_M_19.WAV.

The single-target effect center is the target tile's screen position + (24,24),
then subtract half image width/height and add descriptor offsets. Each frame
waits two original 50 ms ticks. Frame 0 starts sound; frame 1 first restores
the prior background and calls the reaction callback, then captures the new
underlay. The callback reads Unit_spc: status success uses frame 3, misses use
north frame 1 / east mirrored frame 2 / south frame 0 / west frame 2. Flags 6
apply a saturating subtraction of 4 to each quantized color component.
It draws the planned damage number in foreground 18 on background 24, but
does not apply the planned HP/status changes. Final target redraw waits 10 ticks.

For nonzero source index A and background index B, let q be the original
4-bit RGB palette, brightness = floor((q[A].r+q[A].g+q[A].b)/3),
weight = (7*(256-16*brightness)) >> 4. Each output component is
min(15,(q[B]*weight+q[A]*256)>>8), followed by the original 4,096-entry
palette lookup. Source zero copies background unchanged. The exhaustive
65,536-pair original-code hash is
`c6e5df73f58be68237d7908831742abd05ae7dd4d391363ef9e37ec5f72cb8d0`.
Nine complete presentation cases and 244 independent reaction cases pass.
Synthetic raster captures isolate this verification from SDL/Windows screen
composition; exact original font raster and real clock latency remain unknown.
Other MEFF records, multi-target effects and the subsequent target/MP/XP
application routines are not yet enabled.


## Poison application and finalization (0.16)

Original `0x4224a8` sorts target tuples together, aligns tick when tick modulo 4
is below 2, redraws, and focuses each target. The admitted replacement path has
one poison target. Effect 0x70 sets status bit 16 and requests MEFF#49 only when
signed planned damage is below current HP. Effect 0 with zero damage requests
MEFF#45. Numeric HP loss caps positive overkill; subtraction and maximum setters
retain original unsigned behavior. The target MP setter also runs on nonzero
HP damage. Automatic healing (equipment effect 62, item 87) precedes armor XP
and the final external character-window notification. It suspends explicitly.
Target flag 32 is not cleared here, unlike the physical-strike application.

`0x42377d` sets controller success after application returns. `0x422f0d` returns
immediately when success is zero. For poison, cost is tactic field 14, optionally
integer-divided by equipment effect 49. MP subtraction saturates at zero, then
caps at the effective maximum. The numeric window receives the nominal cost,
even when available MP is lower. NPC character XP is rejected by the original
setter, while weapon XP still updates. Party XP/level display is not admitted.
After successful finalization, `0x43dc99` ORs action flags with 6 and clears the
camera controller's interaction byte. These rules consume no additional RNG.

`tools/tactic_application_probe.py` captures 1,108 target/finalization cases at
explicit MEFF/numeric/window boundaries. `tools/poison_controller_probe.py`
executes the complete first caller, including the original MEFF player and
compositor, to verify the actual lethal cast. Raster, audio, font, host clock,
message pumping and waits remain host boundaries. No Windows playthrough or
post-action event/death continuation is implied by these fixtures.


## Post-poison event and next enemy (0.17)

`poison_post_probe.py` continues original `0x45b5fe`, `0x44cc03`, the actor redraw
and `0x44c601` in their source order. It now runs original body dispatcher
`0x41084e`/`0x4105a8`, dialogue handler `0x413661`, speaker lookup `0x41eec0`,
text copying `0x4179a5`, and flag handler `0x4142b2`. Only the final text window
at `0x42d5f1` is a presentation boundary. Event `0x3af7` has body length at
`0x3b22`, dialogue at `0x3b24`, flag-55 write at `0x3b4c`, and ends at `0x3b5a`.
The speech is captured with flag 55 unset and archer 234 at HP 0/lifecycle 2.
The flag becomes 1 after speech; the death scan then retires slot 20. RNG and
persistent unit records do not change. The original caller performs 18 selector
scans before retirement and 18 after its actor redraw, without repeated speech.

The command-level reinforcement VM previously stopped after opcode 50 movement;
its trailing opcode 11 (variable 11) is now executed with the original handler.
Without this state, later original scans incorrectly selected reinforcement
again. This corrects harness continuation, not the parsed source or game rules.

`second_enemy_probe.py` continues this exact VM through slot 36's original
`0x43ad1c` decision, walk, target selection, planning, casting, MEFF#23 and
MEFF#49 application. It captures decision/selection/pre-cast/pre-application
states and original indexed-frame hashes. Actor 147 targets slot 18 (210),
with damage 0, effect 112 and RNG 2402686401. The original complete action
ends at infantry status 16/HP 110, caster MP 40 and flags 6. The replacement
currently matches only through primary effect playback and explicitly guards
the secondary effect before committing application or cost. The MEFF#49
reference frames are available for the next implementation; they are not
claimed as rendered by the SDL frontend yet.

Snapshot restore admits phase-2 AI continuations with original enemy slot bounds
35..114, tactic 33 or physical 255, a completed action flag and valid event
return. A finished tactic plan is no longer retained at the stable dialogue.
The original speech routine requests camera focus; it is already at [144,576]
on this path. General battle-speaker focus in other events remains to be checked.


## 0.18 secondary MEFF resources and remaining enemy actions

`tools/meff.py::decode_secondary` accepts only this edition's MEFF#45/#49
headers and descriptors. #49 contains thirteen descriptors and one 16×16
indexed sprite (536 bytes); #45 has nine descriptors and one 64×64 sprite
(4,296 bytes). Both use 50 ms intervals, palette 255, no mask and no callback.
The frame's dx/dy position is relative to the centered target rectangle. Restore
uses the previous rectangle; capture uses the current one. Sounds are 129/131.

Original compositor 0x42fd5a uses palette quantization and the RGB12 lookup.
For nonzero source pixels, brightness is the integer mean of RGB nibbles;
background weight is `(weight * (256 - brightness * amount)) >> 4`. Foreground
weight is `256 - ((background_weight * (16 - amount)) >> 4)`. Channels saturate
at 15 before lookup. The earlier MEFF#23 amount=16 case masked the latter term;
secondary resources require it. Thirteen exhaustive 65,536-pair original-code
fixtures check every used combination. Zero source pixels preserve background.

0x4224a8 sets poison status before #49 playback; damage/armor experience and
the MP-cost finalizer follow. `unit_state` commits only the status at this early
point. The complete request stream is validated before playback is queued.

`enemy_continuation_probe.py` retains the original priority bytes after the
second cast and executes eleven remaining unit callers. Their post-action
event/death/redraw calls follow 0x44eb39..0x44ebaf. Movement-only resolution
0x43aafb returns without attacking; 0x43b3df still sets bit 2 when the unit has
bit 4 and lacks bit 128. This matters for event dispatch and saved action flags.
No unexpected event body is ignored, and host hooks are explicitly checked.
The probe does not execute phase completion or a Windows interactive session.


### Follow-on turn-loop investigation (not integrated)

At 0x44eed0 the original increments controller byte +2, then compares it with
limit byte +3. Before a subsequent non-resumed turn, 0x44efd9 calls
0x449f91(255). This routine consumes bounded RNG(8), moves selector byte +4,
and looks up a visible classification at 0x48e100 + byte(+0x11)*6 + selector.
A changed classification requests another MEFF presentation before returning.
In `next_turn_probe.py`'s default zero selector/region case, the selector becomes
1, classification remains 0 and RNG changes 3885567453 → 340946258.
**The original initialization of those controller fields has not been recovered.**
The sequential unit-action VM does not initialize turn/limit bytes for the full
turn-loop caller either. Recover these sources before treating this isolated
case as the next original turn. The engine still stops before phase completion.

## Turn-two fire records (0.20 investigation)

This edition's opcode-33 handler is `0x413efb`; placement mode 0 calls
`0x456140(x,y,26,0,focus,silent)`. The controller table at `0x4b3d21` contains
250 records of five bytes `[x,y,new_terrain,old_terrain,effect_kind]`. The original
new-battle loop `0x44d6ee..0x44d7a3` sets every byte to 255. Placement chooses
the first unused record or matching coordinate, whichever appears first; a
repeated placement replaces `old_terrain` with the tile's current value (26
for repeated fire). A full table without a match returns without a change.

The map buffer's terrain plane starts at +0x640; effect marks start at +0x1f40.
Fire uses terrain 26 and mark 0. Focus precedes sound 19; the mark-plane write
then precedes a 1×1 redraw and a one-tick wait. Silent mode suppresses focus,
sound and wait, while retaining the state writes and redraw. Opcode defaults
-1 for focus/silent are decoded as zero. Removal mode 1 and other effect kinds
are not implemented.

`0x452744` selects fire pixels from `Mark.e5` offset
`0x8bc0 + (controller_tick % 4) * 0x900`, drawing 48×48 with mode 4. The original
background tile, fire overlay, selection graphics and unit sprites are separate
layers. The four imported frames use `Spalet.e5#0`. The engine's real-time idle
animation cadence still requires original interactive playback comparison.


## MCALL05 summon (0.21 investigation)

`Mcall05.e5` is an uncompressed, indexed resource with a 20-byte `<4B4I`
header: 30 frames, 30 images, 12 masks, no local palettes, 100 ms interval,
300×300 dimensions and sound index 12. It is followed by 30 20-byte descriptors,
30 90,000-byte image planes and 12 90,000-byte mask planes. SHA-256 is
`fe86b9b67f0672cd5812e755e579b5eb49228eef31702aa0c06606fcc3fb8ada`.

Descriptors use `<4BIiiB`: image, palette, mask index or blend weight, amount,
masked flag, signed x/y displacement, sound mode. Three trailing bytes are
ignored by the original and retained verbatim. The first 20 frames use masks;
the last ten use ordinary weight-16/amount-16 composition. Original loader
`0x402805` converts interval milliseconds into 50 ms ticks; worker `0x402574`
loads the image/mask table. Player `0x403014` places each frame at
center minus half size plus its displacement, restores the prior rectangle,
captures the next background, paints, waits, and finally restores and frees.
Sound 112 is requested on the first frame. No gameplay RNG is consumed.

Masked compositor `0x4300c8` uses the quantized shared palette and 4096-entry
lookup. Zero source pixels copy the background; mask 255 selects the quantized
source directly. Otherwise brightness is the integer average of the source's
three quantized components. Weight is
`((256-mask)*(256-brightness*amount)) >> 8`; foreground weight is
`256-((weight*(16-amount)) >> 4)`. Each component is the clamped 0–15 weighted
sum shifted by eight. A zero mask and zero amount copy the background.
This is not ordinary RGB alpha blending. Exhaustive matrix fixtures and all
30 full-frame synthetic-background hashes execute the original arithmetic.

Opcode 107 (`0x41405b`) uses any nonzero focus argument, tests the centered tile
against the viewport, preloads MCALL05 for ID 105, and plays the retained effect
for ID 109. Its wrapper `0x421879` adds a four-tick wait and message pump. The
loaded identity must survive the four intervening dialogue save points.
Other resource IDs remain unsupported by the engine.

Opcode 21 (`0x413911` → `0x459777`) falls back to the second speaker if neither
specified actor is active. In this opening both IDs are 157, **许子将**, with
lifecycle 1. His text and portrait are displayed without camera focus. This
required form is verified; general alternate-speaker cases are still guarded.
The sequential fixture continues through all 27 fires and subsequent panic
animations/dialogue, stopping before the unit-property change at `0x282c`.


## Area confusion and arrival (0.22 investigation)

Original opcode77 at `S_00.eex:0x282c` supplies
`[2,-1,0,0,0,19,19,5,-1,-1,8,0,0]`. Handler `0x4150e7` calls area
selector `0x45ade5`: y-major inclusive occupancy scan, lifecycle2 only, and
**displayed side** from `0x43fc1a`. Filters0–3 select a side,4 selects0/1,
5 selects2/3,6 selects all; other filters select none. Invalid side bytes read
as0. This is not slot-range allegiance. `confusion_probe.py` checks604 cases,
including poisoned and inactive units, scan order and reversed rectangles.
Coordinates255 are guarded, rather than admitting the original byte-loop wrap.

For each selected unit, `0x45aa5c` invokes `0x43e69f` before mutation. The
presentation centers that unit, runs MEFF48, and notifies the unit window;
then `0x4066c0` ORs status8, redraws the tile and notifies the window again.
The original sequence selects13 enemies, consumes no RNG, and leaves the
animation tick unchanged. Other opcode77 property/area forms remain guarded.

MEFF48 is536 bytes, with13 frames, one16×16 image,50ms interval and sound129.
It reuses the orbit positions of MEFF49 but amounts are0 and weights range0–4.
`0x42fd5a` is independently checked on all65,536 palette pairs for each weight.
The player at `0x40239c` **bypasses** this compositor when weight0 and palette255:
it restores the background in mode0, then draws raw indices in mode4. The
three such frames must not quantize the original image through the palette LUT.
The complete status trace records949 requests:169 frames including39 direct
frames,13 camera notifications,13 sounds and26 unit-window notifications.

Opcode78 maps to `0x4146d4`, not the neighboring death-scan routine. Its
single-unit mode defaults -1 to0. `0x459d8b` finds the first matching nonzero
lifecycle, rejects party-owner0, clears flag128 and maps non-targeted modes
0/1/2 to live AI3/1/2.225 original handler cases cover missing, inactive and
dead units, owner protection and invalid modes. The six actual opening calls
select mode1 for480–485; their current fields are already in the requested
state, so these six calls make no live changes in the sequential trace.

`arrival_event_probe.py` executes37 original source-ordered handlers through
`0x2ef7`, preserving units, persistent records, camera, tick, terrain and RNG.
Music argument5 becomes MCI track7 at the platform boundary. Fifteen original
speech calls become16 three-line prototype pages. Final RNG340946258, tick4,
camera(240,48); Cao Cao ends at(10,6), cavalry258 at(9,6), cavalry259 at(10,7),
and Liu Bei at(9,5). Occupied destinations explain why some script-requested
positions differ from their actual results. The next opcode26 is a dedicated
objective banner, not ordinary dialogue; its exact presentation is unverified.


## Objective and leader highlights (0.23 investigation)

`0x413c0e` dispatches opcode26 to `0x44bd27` in battle mode. The latter sweeps
**rows**, not columns, using `0x453b48(0,row,viewportWidth/48,1)`. Background
checker index31 clears to255. Font sizes are32 below720px,48 below960px,64
otherwise. Title/line/limit shadows have palette pairs42/40,24/21,31/24;
foreground18. Text requests have center=0: left alignment, vertical centering.
Each line starts with two spaces and consumes up to28 further GBK bytes.
The current port rejects an unverified double-byte split. Original opening text
is covered. Text pauses are20ticks each; `0x42c646(20)` finally checks input
after each one-tick wait. It can be dismissed, unlike the preceding text waits.
The native adapter currently uses50ms per tick. Final row clearing and a full
view redraw follow; no live/persistent units, controller bytes or RNG change.

Opcode92 handler `0x416e17` scans slots0..114 for matching ID and lifecycle2.
It centers the camera, opens the ordinary panel through `0x44049e`, then repeats
three cycles of target(slot), unit redraw, wait10, target255, redraw, wait10.
`0x453e32` closes the panel. `0x453a40` first zeros1600 target-mask bytes;
slot255 disables its style, otherwise one valid map position is marked1.
The 15 schedules and12 independent mask cases execute these original routines.
Four actual sequential instructions at2f13/2f28/2f2e/2f41 match before tutorial.

## Tutorial entry: verified and unresolved

Opcode16 dispatches `0x41590d` to `0x433359`. Setup `0x434f84` obtains live
slots for IDs157/0/32 and copies the guide's coordinate to scratch `0x4b2c18`.
`0x41a284` reads an indexed byte from `SVCMN.E5S`; missing-file handling may
create108 zero bytes in the original. The read-only probe replaces this host
boundary with explicit108-byte inputs and never opens/writes the source save.
Scratch `0x4b2c10` counts nonzero bytes0..2; `0x4b2c08` requires all50 items
whose original `0x409600` getter is nonzero to have a nonzero byte at item+3.
68 setup cases cover all8 completion-bit combinations, all-item status, each
missing collectible and nonboolean nonzero bytes.

The entry at `0x433404` offers the original accept/decline choice. Acceptance
returns1 and enters `0x43353c`; decline returns0 and skips the teaching stages.
The top-level caller subsequently checks completion rewards (`0x4347ff`) and
all-treasure rewards (`0x434d33`) even after declining lessons. Finally it says
farewell. 32 entry cases capture this order with dialogue/choice/facing requests
at explicit boundaries. They STOP at teaching/reward routines. Neither their
full effects nor a complete interactive tutorial has been verified or ported.
The current engine executes entry and the first basic lesson; later teaching
and reward routines still stop explicitly at their EXE addresses.


## First tutorial lesson: verified 0.24 behavior

`0x43353c` advances scratch x by1, paints map effect3, then moves y six times,
clearing the prior cell to255 and waiting4 ticks each time. `0x433726` paints
mark3; `0x43378d` clears255 rather than restoring an old value. Renderer
`0x452744` chooses `Mark.e5` offset0x1ec0. A collision is guarded in the port.
The Cao Cao panel stays open during four guide speeches; then it closes and
the cursor clears. The original continuation choice (`0x4333d2`) either enters
`0x4337f4` or says the summary and farewell. Initial decline skips the lessons.

Animation1 uses `UNIT_SPC.E5` frame4. Animation2 uses frames1/2/0/2 for
directions0/1/2/3 and mirrors direction1. Both use sound8 and optional8-tick
pre/post waits. 192 cases capture original sequencing and indexed frame hashes.

`original_tutorial_basic.json` records all three fresh-profile paths, with full
unit/RNG/camera/terrain states at dialogue/choice boundaries. Collection input
is explicit: byte26=1 for original opening collectible item23, others0. The
original save file is never opened. Tutorial instruction records retain EXE
routine/text addresses. Snapshot schema4 includes108 collection bytes and a
tutorial continuation with pc, answers, cursor, active mark and panel state.
Every currently ported speech fits one page; extra queued pages are rejected.
The following lessons and collection rewards require additional verification.


## Second tutorial lesson: ported in0.25

`0x4337f4` moves the guide from(9,0) to(10,0), using the original scripted walk
and mode2 path search. It points the tutorial cursor at(9,6), while Liu Bei's
actual unit remains(9,5); the port must preserve this original distinction.
At `0x4339d9..0x433a55`, Liu Bei's movement-point getter supplies6 to mode0
search with target255/255 and ZOC enabled. `0x4350f0(1)` displays the movement
overlay and `0x4539dc` paints4 attack-mask cells. 39 distance entries are non255.
The panel closes before the range explanation. Dismissal disables movement and
clears the attack mask, but does not erase distance scratch. The final guide is
at(10,0); RNG is unchanged. 118 events and16 speech boundaries are captured by
`original_tutorial_ally.json`. UI-mask storage is initially seeded clear;
subsequent movement search and mask writes execute original code. Native lesson and
continuation/snapshot handling now match the original interaction states.
Schema5 retains the initial cursor origin and validates the guide movement
separately. Both continuation choices execute the original top-level caller
in the extended fixture, stopping at third-lesson entry or returning normally.


## Third tutorial lesson: investigation boundary

`0x433bc8` first walks the guide back west, then demonstrates player movement
with a **blue** overlay. Setter `0x451c03` writes camera+0x64; the second lesson's
green overlay setter `0x4350f0` writes camera+0x68. They must not be conflated.
Cao Cao moves from(10,6) to(10,0). `0x4350c0(saved_coordinate,0,0)` forwards to
`0x442b03(x,y,0,0)`: active lifecycle2 clears old occupancy, redraws, updates x/y,
writes the new occupancy and redraws. Facing/flags and other live/persistent
bytes are retained. A nonzero final argument returns without mutation. The
154-event sequential fixture and24 varied direction/flag/early-return cases
confirm this; it is not evidence for the entire player cancellation controller.

Execution stops before command-menu initializer `0x45bba0`. Its following modal
caller `0x45bc05` requests RT_DIALOG0x117 from Ekd5.exe. Read-only template
extraction yields5 BUTTON controls and 55×77 dialog units with9-point宋体:
attack0x507(y1), tactic0x508(y15), item0x509(y29), standby0x50b(y46), cancel2(y63),
each width55/height14/x0. The tutorial sets dialog fields+0x38/+0x40 to1 after
initialization. Their enablement/behavior and the dynamic dialog placement,
painting and input callbacks still need original-code verification. No menu is
silently skipped or synthesized from the resource metadata alone.
