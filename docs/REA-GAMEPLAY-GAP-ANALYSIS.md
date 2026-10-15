> **Implementation update, 2026-10-08:** This is the preserved 0.8.0 baseline audit. Prototype 0.9.0 repairs many entries below; see [current changes, evidence and remaining gaps](REA-GAMEPLAY-FIXES.md). Historical findings below are not the current implementation status.

# Native gameplay comparison with the original game

Audit date: 2026-10-07. Target: the shipped native prototype **0.8.0**, launched by `dist/prototype-macos-executable/Start Game.command`, across 颍川之战、汜水关之战、虎牢关之战、董卓追击战.

**The native game is playable but still differs materially from the original.** The largest differences concern combat sequences, progression, allied/enemy behavior and scenario events. Most are retained simplifications from the agreed prototype, rather than regressions caused by recent interface work. Five offensive tactics, three consumables, terrain recovery, location pickups, corrected equipment/scaling and detailed unit inspection are already present.

This audit changes no gameplay code or user saves. It compares the current package/source with edition-specific original script and executable evidence. It is not a new full playthrough of the original game, nor a declaration of full acceptance.

## What REA established

REA **5.0.0 CLI** inspected the exact shipped Mach-O, inventoried `Ekd5.exe`, and captured three bounded process runs. The active Codex chat still did not advertise REA MCP tools. Its registration passed the scoped doctor, but REA had no configured Ghidra/Hopper/IDA provider. Consequently **no new REA native decompilation was performed**. Original rule semantics below come from existing bounded x86 fixtures, decoded original scripts and recovered C++ helpers, cross-checked against the current Prototype.

| Identity check | Result |
| --- | --- |
| Original `Ekd5.exe` SHA-256 | `199b1b67cf966ead46c49b430b9aab84bdc7d4f0e8dfa8e0ebeec5da280b01e0` |
| Shipped Mac executable SHA-256 | `5af529ccd44f14632d037d60f392e0e2616ee1d9cf79cb59a2d793fba920492a` |
| Package manifest | All 1,088 listed files match their hashes |
| Assets versus current staging | All 1,061 packaged assets match |
| Build versus package | Every Mach-O `__TEXT` section matches; whole-file digests differ after signing |

Evidence is retained under [evidence/rea-gameplay-audit-2026-10-07](evidence/rea-gameplay-audit-2026-10-07/). The [evidence index](evidence/rea-gameplay-audit-2026-10-07/index.json) records complete REA Evidence IDs and source/fixture hashes.

| REA record | Evidence ID |
| --- | --- |
| Native Mach-O inspection | `ev_b1451da58e439ffd742e99a8c9644b404107dde78ea9ec381530b14de499d66a` |
| Original artifact inspection | `ev_eb44dd1330969afc0a3fdd63236fbab5336205eff8e804cbe7f851d37d26b207` |
| Packaged tactics run | `ev_6a8d09aad918586b6bb39b8f6fb43c639116db1638aec5574e817ae655238239` |
| Packaged items run | `ev_fb038bd1687a4af7355f930275cc1faf3de9d9f95ce977e609f5fd21fc935724` |
| Recovered physical-rule fixture run | `ev_3d14f274a9de7fe78e5dadd68753c5eb3ea2992768cf286d678f8d83d9b4ebf0` |

REA's process records report partial filesystem capture: selected output directories did not exist at the initial snapshot, and generated BMPs exceeded its default 10 MB per-file hashing budget. All exit codes and terminal assertions were captured; a separate [output hash inventory](evidence/rea-gameplay-audit-2026-10-07/output-files-sha256.json) hashes the retained files, including BMPs. This does not turn the initial filesystem snapshots into complete observations. Process captures also leave inherited environment, network activity and short-lived descendants incompletely observed. JEV was explicitly disabled for these offline runs; no JEV requests were made.

## Confirmed gameplay differences

### 1. Physical attacks stop after one strike

**High impact, confirmed.** [Prototype::attack](../src/prototype.cpp#L144) calculates recovered damage/hit behavior once, requests a noncritical animation, changes HP and ends the action. It does not run counterattacks, critical hits, second strikes, strike-related status effects or experience application. Its random-number sequence therefore also differs from a complete original attack.

This is not an allegation that normal hit checks or base damage are fabricated: current balanced battles use recovered formulas and no longer impose the historical 12–45 damage clamp. The missing layer is the complete attack sequence.

Original evidence: `original_attack_controller.json`, controller VA **0x4064f1**. Case 0 includes a critical strike; case 33 includes a counterattack; case 65 includes two attacker strikes and a counter. The existing [attack controller](../src/attack_controller.cpp#L30) implements verified sequences separately from the playable Prototype. Its 73-sequence/1,536-frame-hash check passed during this audit.

### 2. Units and equipment earn no experience

**High impact, confirmed.** Attacks and casts do not update unit records. [Snapshot validation](../src/prototype.cpp#L276) requires each record to remain identical to its scenario preset. The detailed panel can display experience, but that does not mean gameplay awards it.

Original character/equipment XP behavior is already represented by [experience.cpp](../src/experience.cpp), backed by `original_experience.json` and routines **0x408950 / 0x4082d6**. The 2,264 character and 2,187 equipment XP checks passed. Integration must include level-up effects, presentation and revised snapshot validation; merely calling the arithmetic helper would leave saves incompatible.

### 3. Several learned tactics are unavailable

**High impact, confirmed.** The playable catalog admits only **灼热、浊流、旋风、落石、毒烟** (IDs 0/5/10/15/33). [Target selection](../src/prototype_tactics.cpp#L27) only admits opponents, so healing and other friendly support actions cannot occur. [Unit details](../src/unit_details.cpp#L32) deliberately labels other learned tactics 暂未开放.

Concrete examples from the current balanced roster and original 68-record tactic table:

| Unit and scenario | Learned examples missing from play |
| --- | --- |
| 刘备, opening, level 7 | 小补给 |
| 张宝 / 张梁, opening, level 5 | 钝兵、虚脱 |
| 曹操, added battles | 小补给、霸气 |
| 李肃, 汜水关, level 12 | 诱惑、定身、小补给 |
| 李儒, 虎牢关, level 12 | 火阵、小补给、气合 |
| 李儒, 追击, level 22 | 水阵、地阵, in addition to earlier omissions |

These are class/level learning facts; they do not imply every spell would be selectable under every MP, terrain or weather condition. Source evidence: `tests/fixtures/original_tactics.json`, `docs/evidence/terrain-benefits/balance-regression.json`, current packaged tactic catalog. The basic enemy and player casting paths passed the fresh packaged checks described below.

### 4. Allied and enemy decisions are different

**High impact, confirmed.** The Prototype lets the player control every non-enemy unit and transitions directly from player to enemy phase ([prototype.cpp](../src/prototype.cpp#L179)). The original has an allied phase, with bounded execution evidence for VA **0x44e851** in `tools/allied_phase_probe.py`.

Native enemies act in sorted hero-ID order and use the local heuristic or a JEV-selected legal action. Original ordering and priorities differ; recovered primitives are in [ai.cpp](../src/ai.cpp#L25), with original VA **0x44da7a** and `original_ai_priority.json`. The [native candidate scoring](../src/prototype_enemy.cpp#L21) is a deliberate replacement. JEV can make different decisions within legal choices; it does not establish original AI equivalence.

### 5. Ambushes, duels and scripted attack orders are omitted

**High impact, confirmed.** The scenario importer retains original hidden/AI fields, but [native initialization](../src/prototype.cpp#L68) makes every unit active and visible and assigns simplified AI.

| Battle | Original behavior omitted |
| --- | --- |
| 汜水关 | Six originally hidden enemies are visible from the start. Guan Yu versus Hua Xiong's duel event is omitted. |
| 虎牢关 | Enemy activation/attack orders and the Lü Bu duel events are omitted; enemies use the ordinary planner from the first enemy phase. |
| 董卓追击 | Eighteen originally hidden units are visible from the start, replacing staged ambushes with an already populated battlefield. |

Original byte offsets: `S_01.eex` duel **0x2716**, removal **0x290d**, item request **0x2937**; `S_02.eex` regional attack-order event **0x1f48**, separate Lü Bu activation **0x22de**, duel events **0x290a / 0x2be2 / 0x309e**; `S_03.eex` ambush **0x24a5**, reveals **0x2580–0x2787**, further branches **0x2ed8 / 0x31b1 / 0x346a**. These are offsets in original EEX files, not JSON offsets. Decoded trees are in `assets/expansion/S_01.json` through `S_03.json`; original deployment checks are in `docs/evidence/scenarios/`.

Restoring equipment and level scaling corrected important balance errors but did not restore these event conditions or effects.

### 6. Two important victory paths differ

**High impact, confirmed.** The [opening objective and evaluator](../src/prototype_scenarios.cpp#L27) require every enemy dead. Original `S_00.eex` objective at **0x1d9c** requires killing Zhang Bao and Zhang Liang. Their death events set flags 50/51; event **0x333d** tests them and removes remaining enemies at **0x3371**. The native game therefore requires extra cleanup combat.

The pursuit battle always requires defeating Li Ru. The original has a retreat branch changing the objective to **曹操到达西端 OR 击退李儒** (`S_03.eex` **0x1e36 / 0x294a**). Its western-edge event **0x39ce** uses condition **0x39d5**, gated by flag 2. The native game has neither that choice nor the corresponding escape condition.

Cao Cao's defeat and a 20-turn limit are present. All four original scripts specify a 20-turn limit; exact final-phase ordering has not been freshly compared and is not classified as a demonstrated off-by-one defect. The 虎牢关 regional objective is based on original script evidence, but its condition-65 interpretation still lacks a complete independent runtime oracle.

### 7. Rewards and campaign carryover remain absent

**High impact for progression, confirmed.** Choosing another battle calls a fresh `start_scenario`, rebuilding units and inventory from presets. Later scenarios do not inherit earned progress. The inventory accepts only the three supported location consumables (IDs 87/88/92).

Original script requests omitted from these paths include Zhang Bao's **太平清领道** grant (`S_00` **0x387b**, item 76), Hua Xiong's **布手套** (`S_01` **0x2937 / 0x3007**, item 70), and 虎牢关 reward requests containing **皮盔** (`S_02` **0x373d / 0x3e71**, item 64). Selected original speeches are displayed, but their presence does not prove their surrounding reward operations execute.

Full reward quantities, complete opcode-89 sequencing and campaign continuation remain partially researched. This audit does not invent gold totals or claim those systems are ready to connect without further work.

### 8. Confusion recovery ignores original equipment and luck rules

**Medium to high impact, confirmed.** Every confused enemy loses one action, then clears confusion unconditionally ([prototype.cpp](../src/prototype.cpp#L239)). Original **0x43c7d6** considers healer neighbors, equipment, luck and randomness; recovered logic is in [recovery.cpp](../src/recovery.cpp#L31).

This affects an actual current unit: Zhang Bao carries **太平清领道**. The original recovery fixture's case 56, equipment slot 2/item 76, clears confusion without an RNG draw, whereas the Prototype still skips his action. Phase timing must be preserved when integrating the recovered helper. Existing recovery/maintenance checks passed during this audit.

### 9. Equipped treasures do not all have complete effects

**Confirmed omission, partly unresolved semantics.** Lü Bu's **方天画戟** is present, but the one-strike attack path cannot perform its pursuit action. The separate faithful Engine also marks the pursuit continuation at **0x43ac63** unresolved (`src/engine.cpp:551`), so this is not fully recovered functionality awaiting a simple switch.

Do not generalize this to all equipment: stat bonuses, evasion effects and 赤兔马's zone-of-control bypass are already used.

## Other limits and unknowns

- **Opening sequence and starting state:** the Prototype starts from the screenshot-matched turn-2 board and restores full HP. It does not replay R_00, the full tutorial or the captured save's prior damage. Matching positions and unit count does not establish an identical game state.
- **Saves:** schema 8 preserves current prototype state, with checksums and compatibility handling. The UI provides one slot per battle, not the earlier acceptance requirement of ten manual slots or snapshot import/export controls. This is a requirement gap; this audit did not establish the original game's exact slot count.
- **Fire/weather:** the opening's 27 fire tiles persist in native play. Complete original future fire/weather behavior is unresolved; no specific extinguishing turn is claimed.
- **Presentation/audio:** walking, attacks, spell effects and selected WAVs exist. Original wall-clock timing, some palette compositing, item effects and campaign music remain incomplete. These are secondary to the rule/event gaps above.
- **Coverage:** no complete original-versus-native campaign trace, fresh Linux execution or browser UI run was performed for this audit. Existing cross-platform tests do not establish original-game equivalence.

## Fresh checks and what they mean

The exact packaged Mac executable passed under REA capture:

- Enemy 李肃 poison and 李儒 elemental casts, with 26 and 21 presented effect frames respectively; player spell picker/targeting; invalid target rejection; MP/status save restoration.
- Item view buttons, Cao Cao self-heal of 30 HP, target/move cancellation, the post-move menu and save/reload.

These use synthetic legal contact/HP placements in the existing smoke routines. They demonstrate those bounded paths, not ordinary AI navigation through an entire battle or a live-model decision. Sources: `prototype_frontend.cpp:751–775` and `896–944`.

REA also captured a successful C++ comparison against retained original-code fixtures: 2,184 stat getter cases; 2,106 physical damage/hit/RNG cases; 5,120 critical-speech cases; 392 critical/double chances; 1,297 strike plans plus one original-fault guard; and 958 counterattack gates. That process ran the recovered C++ test executable, **not Ekd5.exe**. These passing core tests make the missing integration visible; they do not prove the playable Prototype uses all those mechanics.

Additional read-only core checks passed for attack-controller sequences, recovery, XP and AI. Their scopes and worker-reported results are recorded in the evidence index; those four checks do not have separate REA capture logs. No binaries were rebuilt or replaced.

## Recommended implementation order

1. Correct opening leader-only victory and implement its verified treasure grant. Restore the pursuit retreat choice/condition with focused script tests.
2. Integrate original status maintenance, especially Zhang Bao's guaranteed recovery; preserve phase order and RNG.
3. Connect the recovered complete physical-attack controller, then character/equipment XP with a compatible save migration.
4. Restore separate allied turns and scenario-specific activation, ambushes and duel effects, one battle at a time.
5. Add current-roster healing/support/area tactics before expanding to every tactic or later campaign battles.
6. Complete reward/campaign state carryover and multi-slot backups when linking the independent battles into a campaign.

The best opportunity now is reuse of verified core mechanics and faithful scenario conditions. More interface polish alone will not close these gameplay differences.
