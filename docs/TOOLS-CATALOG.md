# Development tools catalog — 三国志曹操传

Last inventoried: **2026-10-04**. This is the working index of the tools built for this project, their use cases, outputs, and important limits. Update it whenever a tool is added, renamed, retired, or changes its interface.

**2026-10-07 addition:** REA 5.0.0 is installed for Codex. Its CLI artifact inspection and bounded process capture were used for the [current gameplay gap audit](REA-GAMEPLAY-GAP-ANALYSIS.md). Its native decompiler provider is not configured; it does not replace the existing Cerberus/x86 evidence workflow yet. This addition is not a fresh inventory of every tool listed below.

The inventory covers **all 112 Python files currently in `caocao-engine/tools/`**, all eleven engine build/setup files in `scripts/`, the native trace/render utilities, the Python and C++ test suites, the PDF report generator, and the earlier Electron/Wine support tools. Generated build files, cached downloads, vendored third-party tools, application implementation modules, and one-off commands are excluded. A file's inclusion means it exists and was inspected; it does **not** mean its checks were rerun on this date or that gameplay acceptance is complete.

## Which tool should I use?

| Task | Start here | What it helps establish |
| --- | --- | --- |
| Find the original function responsible for behavior | [Binary feature map](/Users/henryzhang/Documents/workspace/caocao-engine/output/reverse-engineering/Ekd5-binary-feature-map.txt) → `disassemble.py` | Candidate entry points, original instructions, callers, and evidence scope. |
| Get a decompiled C view and function context | `scripts/cerberus/run.sh export function-dossier` | Static Ghidra dossiers; confirm inferred types and incomplete transfers with original-code probes. |
| Identify a shipped build and retain bounded process observations | `npx -y rea-agents@5.0.0 inspect-macho`, `inspect-artifact`, `capture-process` | REA Evidence IDs, binary identity, terminal/exit and opt-in filesystem observations. See the dated audit scenarios; process capture is not a sandbox or an equivalence test. |
| Understand a memory field or unexplained state change | [Game state dictionary](/Users/henryzhang/Documents/workspace/caocao-engine/docs/GAME-STATE-DICTIONARY.md) → `game_state.py lookup/decode/diff` | Named fields, flags, signedness, pointer resolution, unknown bytes. |
| Reproduce an original rule before porting it | `probe_vm.py` + the relevant focused probe below | Bounded original-code results with explicit host boundaries. |
| Locate the next script instruction/event to implement | `eex.py`, `script_listing.py`, R_00/S_00 listings | Typed arguments and original byte offsets. |
| Recover original art, text, maps, or effect frames | `import_assets.py` + the appropriate decoder | Edition-specific asset data and source hashes. |
| Debug a native rule regression | Relevant CTest target + its `original_*.json` fixture | Replacement behavior against retained original-code observations. |
| Inspect or validate a prototype save | `caocao_trace --restore FILE` | Restored replacement-engine state/hash, without writing a save slot. |
| Compare native and browser simulation | `parity.py`, after the three builds are current | Matching replacement traces on macOS, Linux, and Wasm/Node. |
| Inspect a scene without a long manual playthrough | `caocao --render-…-smoke` | A selected scene's rendered output; not full interactive acceptance. |
| Ask JEV to review a bounded decision | `exe_map_review.py` for map packets; `jev_decide.py` to submit | Advisory review with an auditable request/result record. |
| Package or test relocation | `package-checkpoint.py`, `check_relocation.py` | Existing packaging workflow; refresh their 0.22 assumptions first. |

## Operating conventions

- Run engine commands from `/Users/henryzhang/Documents/workspace/caocao-engine` using `.venv/bin/python`. [requirements.txt](/Users/henryzhang/Documents/workspace/caocao-engine/requirements.txt) pins Pillow, pefile, Capstone, and Unicorn; [dependencies.lock.json](/Users/henryzhang/Documents/workspace/caocao-engine/dependencies.lock.json) and [CMakeLists.txt](/Users/henryzhang/Documents/workspace/caocao-engine/CMakeLists.txt) record build dependencies.
- The source edition is `/Users/henryzhang/Downloads/sgzccz`. Original-code probes run bounded x86 CPU emulation; they do not launch Wine or Windows. Hooks stand for documented host/presentation boundaries, so an intercepted request is evidence about that request, not proof of the intercepted implementation.
- Most probes accept `--source GAME_DIRECTORY` and `--output FILE`. **Exceptions:** `executable_probe.py` and `combat_probe.py` take a positional EXE path and `--out`; `unit_probe.py` takes a positional game directory and `--out`; `controller_initialization_probe.py` is a library helper. The state-dictionary probe's `--output` is a directory. Inspect CLI source/help when unsure.
- Probes generally write their default fixture files. For a fresh investigation, send output to a new `.cache/…json` path and review differences before replacing established fixtures. Sequential probes can replay a long prefix and take several minutes.
- Historical1.0.0 captures use the archived dictionary at docs/evidence/game-state/dictionary-1.0.0.json via the --dictionary option; its identity matches all seven retained captures. Current captures use the default dictionary.
- Dictionary captures are **read-only memory observations**, not restorable original-VM checkpoints. Prototype engine snapshots are a separate format. Keep original `.e5s` saves untouched.
- A passing probe or cross-platform trace supports only its documented cases. Current acceptance and dated verification records are in [COMPATIBILITY.md](/Users/henryzhang/Documents/workspace/caocao-engine/docs/COMPATIBILITY.md); field/format evidence is in [FORMATS.md](/Users/henryzhang/Documents/workspace/caocao-engine/docs/FORMATS.md).

## Complete Python tool inventory

Each file has exactly one primary entry below. Output paths are relative to the engine root. For probe rows, `original_…json` abbreviates `tests/fixtures/original_…json` unless another directory is shown. Library rows name their callable interfaces rather than implying a standalone command.

### Binary investigation, state inspection, and JEV

| Tool | When to use it / scope | Main output or interface |
| --- | --- | --- |
| [disassemble.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/disassemble.py) | Inspect the exact x86 instructions in a bounded virtual-address range before changing a rule. | `Assembly on stdout; start/end are virtual addresses.` |
| [exe_function_map.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/exe_function_map.py) | Build the PE/function-candidate inventory: file offsets, virtual addresses, direct calls, imports, resources, and probe references. Candidate ranges are not recovered original symbols. | `output/reverse-engineering/Ekd5-function-inventory.json` |
| [render_exe_map.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/render_exe_map.py) | Validate the inventory against original bytes and produce the human-readable binary feature map. Retains the historical 29 map-review records/285 questions; later entries without an entry-specific JEV review are explicitly labeled. | `output/reverse-engineering/Ekd5-binary-feature-map.txt` |
| [exe_map_review.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/exe_map_review.py) | Prepare bounded function-map review packets for JEV; packet creation itself sends no requests. Existing packet names cannot be overwritten. | `docs/evidence/jev/NNN-exe-map-*.request.json` |
| [jev_decide.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/jev_decide.py) | Send a selected structured decision packet to JEV and save an immutable request/result record. Use for prioritization and interpretation review; confirm semantics against original code. | `Explicit new record path; credentials stay outside the project.` |
| [probe_vm.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/probe_vm.py) | Reuse the edition-checked, bounded x86 emulator, memory access, hook tracking, and dictionary captures when adding a focused original-code experiment. Library, not a standalone CLI. | `In-memory VM and read-only observation objects.` |
| [game_state.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/game_state.py) | Look up a known memory address, decode an observation, and explain field-level changes with types, flags, pointer targets, evidence, and explicit unknown bytes. | `CLI text/JSON; describe can write a Markdown reference.` |
| [state_dictionary_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/state_dictionary_probe.py) | Regenerate independent original-code checks for tutorial setup, movement cancellation, first strike planning, and RNG; verify that capturing memory does not change it. | `docs/evidence/game-state/: captures, diffs, verification.json` |

### Asset import and script decoding

| Tool | When to use it / scope | Main output or interface |
| --- | --- | --- |
| [import_assets.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/import_assets.py) | Import original graphics, audio, text, scripts, data, and the supplied Chinese font into a versioned asset tree with source hashes. Publishes staging output only after checks. | `assets/generated/ and manifest; retains generated.previous.` |
| [ls12.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/ls12.py) | Decode bounded KOEI LS11/Ls12 compressed blocks for the importer or a focused resource investigation; use the Python library API. | `Decoded Block objects via decode(...).` |
| [eex.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/eex.py) | Parse EEX scripts into typed, offset-preserving instructions; malformed or unsupported input raises an error instead of resynchronizing or silently skipping it. | `Typed script data via parse(path); uses eex_schema.json.` |
| [script_listing.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/script_listing.py) | Generate readable R_00/S_00 instruction listings and an opcode inventory from the imported script JSON; use to locate the next unported event. | `docs/R_00-listing.txt, S_00-listing.txt, instruction-inventory.json` |
| [tactics.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/tactics.py) | Decode this edition's Data.e5 block 5 tactic definitions; use the library when investigating availability, costs, and tactic data. | `Decoded tactic data via decode_tactics(...).` |
| [combat_assets.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/combat_assets.py) | Decode Imsg.e5 fixed-record messages and critical-speech strings without changing original text bytes. | `Decoded messages via decode_message/decode_critical_speeches.` |
| [ui_assets.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/ui_assets.py) | Extract edition-specific UI labels/tutorial text and palette transforms from PE bytes for readable Chinese panels and captions. | `Decoded labels, tutorial text, and palette data.` |
| [meff.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/meff.py) | Decode the supported poison and secondary-effect MEFF resources; use when reproducing verified effect frames, not as a general decoder for every effect. | `Decoded supported frames via decode_poison/decode_secondary.` |
| [mcall.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/mcall.py) | Decode MCALL05 frame descriptors and original indexed image/mask planes for the summon sequence. | `Decoded MCALL05 data via decode_mcall(...).` |

### Original-code probes: initialization and core rules

| Tool | When to use it / scope | Main output or interface |
| --- | --- | --- |
| [executable_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/executable_probe.py) | Recover RNG and isolated arithmetic results from bounded original x86 routines; generate deterministic comparison inputs. | `original_rng.json` |
| [unit_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/unit_probe.py) | Compare original data loading and portrait selection against imported unit data. | `original_units.json` |
| [initialization_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/initialization_probe.py) | Verify growth, default equipment, party averages, joins, and initialized persistent/live unit records. | `original_initialization.json` |
| [deployment_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/deployment_probe.py) | Recover deployment script-handler effects using the original argument decoder. | `original_deployment.json` |
| [controller_initialization_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/controller_initialization_probe.py) | Execute the opening controller's bounded initialization block and terrain-effect clearing; imported initialize(v) helper, not a standalone command. | `initialize(v) returns before/after controller observations.` |
| [battle_initialization_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/battle_initialization_probe.py) | Recover the populated live battle state with explicit rendering and I/O boundaries. | `original_battle_initialization.json` |
| [battle_entry_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/battle_entry_probe.py) | Check new-battle cleanup and focus selection before the introduction starts. | `original_battle_entry.json` |

### Original-code probes: movement, range, and placement

| Tool | When to use it / scope | Main output or interface |
| --- | --- | --- |
| [movement_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/movement_probe.py) | Compare movement points, terrain/zone-of-control restrictions, and distance searches; its shared live_json decoder now uses the state dictionary. | `original_movement.json` |
| [route_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/route_probe.py) | Recover path reconstruction and fallback AI-position scoring without replacing those rules with hooks. | `original_routes.json` |
| [range_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/range_probe.py) | Compare attack masks, ordered target lists, and preparation used by AI targeting. | `original_ranges.json` |
| [placement_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/placement_probe.py) | Resolve occupied or impassable deployment destinations in original fallback order. | `original_placement.json` |
| [movement_animation_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/movement_animation_probe.py) | Match walking frame, timing, and composition requests at raster boundaries. | `original_movement_animation.json` |
| [script_movement_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/script_movement_probe.py) | Recover opcode-50 movement and its wrapper after reinforcement dialogue. | `original_script_movement.json` |
| [reinforcement_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/reinforcement_probe.py) | Verify opcode-76 appearance/placement, lifecycle, and mirror changes using original placement code. | `original_reinforcements.json` |

### Original-code probes: AI, phases, and maintenance

| Tool | When to use it / scope | Main output or interface |
| --- | --- | --- |
| [ai_attack_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/ai_attack_probe.py) | Compare single-target AI attack scoring; area-attack scoring is explicitly excluded. | `original_ai_attack.json` |
| [ai_decision_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/ai_decision_probe.py) | Recover the chosen action and destination up to movement entry; movement and the action have not yet executed. | `original_ai_decision.json` |
| [ai_movement_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/ai_movement_probe.py) | Continue original AI decision and movement to the action boundary. | `original_ai_movement.json` |
| [ai_priority_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/ai_priority_probe.py) | Recover AI unit ordering from the state after the introduction. | `original_ai_priority.json` |
| [player_phase_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/player_phase_probe.py) | Investigate automatic exit from the player phase and this edition's disabled-autosave patch; not an interactive player-action implementation. | `original_player_phase.json` |
| [player_movement_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/player_movement_probe.py) | Execute player movement eligibility, tile selection/retry/cancel, original messages, route/walking and state changes;502 cases. Explicit picker responses; no Windows input or action-menu claim. | original_player_movement.json |
| [player_action_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/player_action_probe.py) | Execute1106 movement/menu/wait/cancel combinations, including availability grids/targets and rollback. Shared configure_action/answer helpers for caller probes; attack/tactic/item calls stop. | original_player_action.json |
| [player_input_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/player_input_probe.py) | Execute140 original outer-loop sequences with real movement/menu callbacks, action-event/death scans and end confirmation. Supplies explicit host input, focus and window boundaries; not physical mouse or continuous turn-two evidence. | original_player_input.json |
| [phase_transition_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/phase_transition_probe.py) | Trace the phase caller after the first infantry event up to its first AI invocation. | `original_phase_transition.json` |
| [allied_phase_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/allied_phase_probe.py) | Follow the allied caller through AI, walking, physical attacks, and the first NPC death; also supports an enemy-entry capture mode. | `original_allied_phase.json` |
| [enemy_phase_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/enemy_phase_probe.py) | Recover the first enemy's decision and walk after reinforcement and maintenance in source order. | `original_enemy_phase.json` |
| [second_enemy_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/second_enemy_probe.py) | Follow the second enemy's decision, poison cast, and application after the archer's death. | `original_second_enemy.json` |
| [enemy_continuation_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/enemy_continuation_probe.py) | Follow the remaining first enemy-phase actions and their post-action calls in source order. | `original_enemy_continuation.json` |
| [turn_start_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/turn_start_probe.py) | Check the order of turn-start calls established by the original phase/controller code. | `original_turn_start.json` |
| [recovery_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/recovery_probe.py) | Compare turn-start grade/status recovery and its RNG consumption. | `original_recovery.json` |
| [maintenance_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/maintenance_probe.py) | Compare poison and regeneration calculations during maintenance. | `original_maintenance.json` |
| [environment_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/environment_probe.py) | Recover environment selection and RNG for each tested class/outcome. | `original_environment.json` |
| [next_turn_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/next_turn_probe.py) | Follow enemy completion into turn two's banner and selected event. | `original_next_turn.json` |

### Original-code probes: physical combat and aftermath

| Tool | When to use it / scope | Main output or interface |
| --- | --- | --- |
| [combat_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/combat_probe.py) | Isolate physical damage arithmetic using explicitly stubbed getters; useful for formula boundaries, not complete combat fidelity. | `original_physical_arithmetic.json` |
| [physical_resolution_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/physical_resolution_probe.py) | Compare physical hit/damage with real original getters and RNG rather than the isolated arithmetic stubs. | `original_physical_resolution.json` |
| [strike_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/strike_probe.py) | Recover strike plans, critical/double rolls, and experience requests, including array contents and RNG ordering. | `original_strikes.json` |
| [strike_application_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/strike_application_probe.py) | Compare HP/MP/status application up to an automatic-item request; the automatic-item path remains a boundary. | `original_strike_application.json` |
| [counter_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/counter_probe.py) | Recover the counterattack gate up to facing/recursive-attack requests. | `original_counters.json` |
| [experience_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/experience_probe.py) | Compare character/equipment XP mutations at display boundaries; do not infer full level-up UI behavior. | `original_experience.json` |
| [critical_speech_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/critical_speech_probe.py) | Recover the critical-speech gate and text selection separately from the animation. | `original_critical_speech.json` |
| [attack_animation_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/attack_animation_probe.py) | Match physical-strike animation requests at raster, audio, and wait boundaries. | `original_attack_animation.json` |
| [attack_controller_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/attack_controller_probe.py) | Follow the physical attack controller including presentation RNG and captured pixels. | `original_attack_controller.json` |
| [post_action_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/post_action_probe.py) | Recover phase-caller behavior around a supplied completed first attack. | `original_post_action.json` |
| [defeated_unit_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/defeated_unit_probe.py) | Recover the observed non-party defeated-unit handler path with arguments (0,0). | `original_defeated_unit.json` |
| [battle_actions_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/battle_actions_probe.py) | Verify opening removal/facing handlers and their rendering/audio requests. | `original_battle_actions.json` |

### Original-code probes: tactics, poison, and effects

| Tool | When to use it / scope | Main output or interface |
| --- | --- | --- |
| [tactics_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/tactics_probe.py) | Compare original tactic loading and mode-4 availability; tactic effects do not execute here. | `original_tactics.json` |
| [tactic_scoring_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/tactic_scoring_probe.py) | Compare opening-enemy tactic hit, damage, and AI scoring while preserving decision-time RNG draws. | `original_tactic_scoring.json` |
| [tactic_plan_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/tactic_plan_probe.py) | Recover the first poison plan, independent hit checks, damage, and XP; stop before casting animation/application. | `original_tactic_plan.json` |
| [tactic_cast_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/tactic_cast_probe.py) | Match original caster poses and tactic-caption requests. | `original_tactic_cast.json` |
| [tactic_effect_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/tactic_effect_probe.py) | Follow poison MEFF loading/playback, reaction callback, frame order, compositing, sounds, and waits; excludes HP/status/MP/XP application. | `original_tactic_effect.json` |
| [secondary_effect_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/secondary_effect_probe.py) | Recover poison-status/miss MEFF playback and integer compositing. | `original_secondary_effect.json` |
| [tactic_application_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/tactic_application_probe.py) | Compare poison target mutations and caster finalization at display boundaries. | `original_tactic_application.json` |
| [poison_controller_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/poison_controller_probe.py) | Follow the first enemy's poison caller from selection through application and action flags. | `original_poison_controller.json` |
| [poison_post_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/poison_post_probe.py) | Investigate post-poison event/death ordering; exploratory output defaults to .cache, not the fixture directory. | `.cache/original-poison-post.json` |

### Original-code probes: scripted battle events

| Tool | When to use it / scope | Main output or interface |
| --- | --- | --- |
| [battle_events_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/battle_events_probe.py) | Exercise the section-1 event selector and opening conditions to establish event ordering and trigger semantics. | `original_battle_events.json` |
| [fire_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/fire_probe.py) | Verify opcode-33 fire placement, terrain mutations, and original Mark fire-frame data. | `original_fire.json` |
| [fire_event_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/fire_event_probe.py) | Continue turn two through the first three scripted fires in sequence. | `original_fire_event.json` |
| [mcall_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/mcall_probe.py) | Recover MCALL05 preload, frame selection, and presentation order. | `original_mcall.json` |
| [mcall_blend_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/mcall_blend_probe.py) | Verify the masked summon compositor independently of Windows and host APIs. | `original_mcall_blend.json` |
| [summon_event_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/summon_event_probe.py) | Continue the fire event through MCALL05 and the remaining fire tiles. | `original_summon_event.json` |
| [confusion_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/confusion_probe.py) | Recover area selection/status mutation and MEFF48 compositor boundary cases. | `original_confusion.json` |
| [status_event_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/status_event_probe.py) | Continue the actual fire event through the area-confusion command. | `original_status_event.json` |
| [script_ai_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/script_ai_probe.py) | Verify opcode-78 single-unit, non-targeted AI assignment. | `original_script_ai.json` |
| [arrival_event_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/arrival_event_probe.py) | Recover sequential turn-two AI changes and dialogue leading into Cao Cao's arrival. | `original_arrival_event.json` |
| [objective_event_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/objective_event_probe.py) | Follow the objective screen, two target highlights, and guide appearance in sequence. | `original_objective_event.json` |

### Original-code probes: panels, palettes, camera, and rendering

| Tool | When to use it / scope | Main output or interface |
| --- | --- | --- |
| [animation_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/animation_probe.py) | Capture bounded original animation sequences and indexed pixels before Win32 drawing for frame comparisons. | `original_animations.json` |
| [presentation_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/presentation_probe.py) | Recover camera progression and sound filename/API-flag requests; does not play audio or open a host GUI. | `original_presentation.json` |
| [selection_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/selection_probe.py) | Recover unit-selection presentation order at documented boundaries. | `original_selection.json` |
| [overlay_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/overlay_probe.py) | Match selection masks and tile-redraw layering at raster boundaries. | `original_overlay.json` |
| [panel_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/panel_probe.py) | Recover normal unit-panel layout, displayed values, and raster requests. | `original_panel.json` |
| [palette_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/palette_probe.py) | Verify quantized color matching and darken mappings without display APIs. | `original_palette.json` |
| [combat_palette_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/combat_palette_probe.py) | Verify all 4,096 entries of the original quantized combat-palette lookup. | `original_combat_palette.json` |
| [attack_panel_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/attack_panel_probe.py) | Recover mode-1 attack preview text and values using original formatters/getters. | `original_attack_panel.json` |
| [attack_camera_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/attack_camera_probe.py) | Recover attack-camera destinations and panel avoidance without raster hooks. | `original_attack_camera.json` |
| [damage_panel_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/damage_panel_probe.py) | Compare272 NPC damage-panel sequences, including absent-opponent255 and all viewport positions; original glyph requests and HP/MP interpolation. | `original_damage_panel.json` |
| [tactic_panel_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/tactic_panel_probe.py) | Recover mode-2 previews for opening tactics 23, 25, and 33, including RNG/state non-mutation checks. | `original_tactic_panel.json` |
| [caption_window_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/caption_window_probe.py) | Recover type-0 caption background and border raster requests. | `original_caption_window.json` |
| [turn_banner_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/turn_banner_probe.py) | Recover banner sweep, text requests, and action-flag resets with an explicit supplied client rectangle. | `original_turn_banner.json` |
| [objective_banner_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/objective_banner_probe.py) | Recover the objective banner sequence and its interruptible terminal pause. | `original_objective_banner.json` |
| [highlight_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/highlight_probe.py) | Recover opcode-92 scheduling, first-active-slot lookup, and target masks. | `original_highlight.json` |
| [terrain_panel_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/terrain_panel_probe.py) | Recover terrain-panel layout, predicates, glyph requests, and indexed icons; supplied empty tile and host font boundaries limit full-screen claims. | `original_terrain_panel.json` |

### Original-code probes: tutorial and command menu

| Tool | When to use it / scope | Main output or interface |
| --- | --- | --- |
| [tutorial_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/tutorial_probe.py) | Recover setup and entry/skip branches with explicit collection bytes instead of accessing a user's SVCMN.E5S; unexplored teaching/reward paths stop. | `original_tutorial_entry.json` |
| [tutorial_basic_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/tutorial_basic_probe.py) | Follow entry and the first lesson through dialogue, facing, poses, cursor writes, and continuation choice. | `original_tutorial_basic.json` |
| [tutorial_ally_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/tutorial_ally_probe.py) | Follow the ally/enemy lesson, guide walking, cursor mutations, and range masks; captured popups do not establish player actions. | `original_tutorial_ally.json` |
| [tutorial_actions_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/tutorial_actions_probe.py) | Follow the movement/action lesson, blue range, Cao Cao's walk, cancellation, and command-menu entry. | `original_tutorial_actions.json` |
| [tutorial_cursor_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/tutorial_cursor_probe.py) | Verify cursor lifetime during camera scrolling, including unchanged destinations. | `original_tutorial_cursor.json` |
| [tutorial_menu_resource.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/tutorial_menu_resource.py) | Extract the original command-menu dialog template without loading a DLL or executing the PE; metadata alone does not prove callbacks or rendering. | `original_tutorial_menu_resource.json` |
| [tutorial_menu_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/tutorial_menu_probe.py) | Recover menu initialization, positioning, enabled controls, result writes, and cancellation with explicit Win32 inputs. | `original_tutorial_menu.json` |
| [tutorial_menu_event_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/tutorial_menu_event_probe.py) | Continue after each enabled demonstration-menu response through continuation/decline; the Windows message pump remains a supplied host boundary. | `original_tutorial_menu_event.json` |
| [tutorial_terrain_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/tutorial_terrain_probe.py) | Follow the fourth lesson, terrain/affinity panel requests, summary, and farewell for the fresh collection profile; that profile has no rewards. | `original_tutorial_terrain.json` |

| [disappearance_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/disappearance_probe.py) | Recover mode1 retirement, cleanup, cropped sprite frames and static terrain masks at the lower raster boundary; animated terrain13 remains unresolved. | `original_disappearance.json` |
| [start_banner_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/start_banner_probe.py) | Recover opcode90 checker sweep, Chinese text requests, waits and cleanup for supplied viewport/client sizes. | `original_start_banner.json` |
| [tutorial_exit_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/tutorial_exit_probe.py) | Replay the fresh-profile tutorial and exact exit/facing/banner/completion instructions; capture state, RNG and occupancy. | `original_tutorial_exit.json` |

| [phase_maintenance_probe.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/phase_maintenance_probe.py) | Continue original fresh-profile tutorial exit through ordered recovery, no-opponent poison panel/HP write and regeneration. Explicit host viewport; other recovery/party-XP presentations stop. | `original_phase_maintenance.json` |

### Cross-platform comparison and packaged validation

| Tool | When to use it / scope | Main output or interface |
| --- | --- | --- |
| [parity.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/parity.py) | Compare full JSON traces/events/state hashes for both choices across native macOS, Linux x86-64, and Wasm/Node. This compares replacement builds, not the original game's full behavior. | `docs/evidence/parity-VERSION.json and trace-choice-*-VERSION.json` |
| [check_relocation.py](/Users/henryzhang/Documents/workspace/caocao-engine/tools/check_relocation.py) | Run the packaged Linux checkpoint in a no-network container with no development mount and a Chinese/spaced path. Currently tied to 0.22 evidence/snapshot filenames. | `docs/evidence/relocation-0.22.json and relocated render images.` |

## Shared schemas and research references

These are maintained inputs/reference artifacts, rather than executables.

| File | Use case | How it stays current |
| --- | --- | --- |
| [game_state_dictionary.json](/Users/henryzhang/Documents/workspace/caocao-engine/tools/game_state_dictionary.json) | Canonical typed memory definitions: 114 fields across 23 structures/buffers in dictionary 1.0.2, with evidence and explicit unknowns. | Edit the JSON, validate schema/evidence, run focused tests/probes, regenerate [the reference](/Users/henryzhang/Documents/workspace/caocao-engine/docs/GAME-STATE-DICTIONARY.md). |
| [eex_schema.json](/Users/henryzhang/Documents/workspace/caocao-engine/tools/eex_schema.json) | Instruction names/argument structure used by strict EEX decoding. Decodable does not mean implemented in the replacement engine. | Change only with edition-specific evidence; update importer/script tests and listings. |
| [exe_map_annotations.json](/Users/henryzhang/Documents/workspace/caocao-engine/tools/exe_map_annotations.json) | Analyst labels, scope, and evidence for function candidates. | Update with original-code evidence; regenerate and validate the inventory/map. |
| [JEV-DECISIONS.md](/Users/henryzhang/Documents/workspace/caocao-engine/docs/JEV-DECISIONS.md) | Track delegated questions, calls, dispositions, and verification. | Append each new issue/result and update totals from immutable records in `docs/evidence/jev/`. |
| [DEVELOPMENT-TOOLING-PRIORITIES.md](/Users/henryzhang/Documents/workspace/caocao-engine/docs/DEVELOPMENT-TOOLING-PRIORITIES.md) | Track the five proposed development accelerators and their implementation status. | Keep planned capability separate from existing partial tools. |
| [COMPATIBILITY.md](/Users/henryzhang/Documents/workspace/caocao-engine/docs/COMPATIBILITY.md) | Determine what gameplay/build behavior has actually been tested and what remains unresolved. | Add dated evidence and exact scope after meaningful verification. |

## Build, test, and packaging utilities

| Tool | Use case | Dependencies / result / current limit |
| --- | --- | --- |
| [bootstrap-mac.sh](/Users/henryzhang/Documents/workspace/caocao-engine/scripts/bootstrap-mac.sh) | Set up a missing local development toolchain. | Homebrew CMake/Ninja if missing, Python venv and pinned packages, Emscripten 6.0.10. Installs/downloads dependencies. |
| [build-linux.sh](/Users/henryzhang/Documents/workspace/caocao-engine/scripts/build-linux.sh) | Build Linux x86-64 and run native tests and basic rendering checks. | Requires an already running Docker-compatible daemon; builds the image, runs CTest and two Xvfb smoke modes. Does not start OrbStack itself. |
| [Dockerfile](/Users/henryzhang/Documents/workspace/caocao-engine/scripts/Dockerfile) | Recreate the Linux compiler/display/audio dependency environment. | Pinned Debian base, specified CMake/Ninja packages, GCC and Xvfb; image tag `caocao-engine-build:0.1`. Other apt packages follow repository availability. |
| [build-web.sh](/Users/henryzhang/Documents/workspace/caocao-engine/scripts/build-web.sh) | Compile the same core and SDL frontend to WebAssembly. | Requires bootstrapped Emscripten and imported assets; writes `build-web/`, copies the Chinese font. |
| [test-web-core.sh](/Users/henryzhang/Documents/workspace/caocao-engine/scripts/test-web-core.sh) | Run the 18 compiled C++ test executables under Wasm/Node. | Needs current `build-web/` test outputs. This is not Chrome/Safari UI verification. |
| [serve.sh](/Users/henryzhang/Documents/workspace/caocao-engine/scripts/serve.sh) | Serve the local browser prototype over HTTP. | Binds 127.0.0.1, default port 8765 (`PORT` override); serves `build-web/`. |
| [package-checkpoint.py](/Users/henryzhang/Documents/workspace/caocao-engine/scripts/package-checkpoint.py) | Assemble source, Linux, and web archives, notices, and checksums. | Uses current build outputs but **hardcodes 0.22.0-investigation and its descriptive text**. Update before delivering a new version; rebuilds files in `dist/`. |
| [cerberus/setup.sh](/Users/henryzhang/Documents/workspace/caocao-engine/scripts/cerberus/setup.sh) | Build the pinned static-analysis environment. | Downloads reviewed Cerberus commit/Ghidra ZIP; builds `caocao-cerberus:12.0.4`. |
| [cerberus/run.sh](/Users/henryzhang/Documents/workspace/caocao-engine/scripts/cerberus/run.sh) | Run doctor, PE import, or a function dossier offline. | Hash-gated read-only EXE and dedicated writable workspace; no credential/home/socket mounts. See [workflow](CERBERUS-RE.md). |
| [cerberus/Dockerfile](/Users/henryzhang/Documents/workspace/caocao-engine/scripts/cerberus/Dockerfile) | Define Java21, Ghidra12.0.4 and the non-root Cerberus image. | Pinned base digest and ZIP checksum; Ubuntu apt packages are not snapshot-pinned. |
| [cerberus/requirements.lock.txt](/Users/henryzhang/Documents/workspace/caocao-engine/scripts/cerberus/requirements.lock.txt) | Recreate the tested Cerberus Python dependency versions. | Setup consumes the frozen direct/transitive packages before installing the pinned source without dependencies. |
| [CMakeLists.txt](/Users/henryzhang/Documents/workspace/caocao-engine/CMakeLists.txt) | Configure the shared core, trace utility, test executables, and optional SDL frontend. | `-DCAOCAO_RENDERER=OFF` supports focused native core development. |
| [caocao_trace](/Users/henryzhang/Documents/workspace/caocao-engine/src/trace.cpp) | Emit ordered events/state hashes, capture a supported prototype snapshot, or validate/inspect a restored one. | Built as `build-core/caocao_trace`, `build-linux/caocao_trace`, or `build-web/caocao_trace.js`; stops at explicit unimplemented boundaries. |
| [caocao rendering smoke modes](/Users/henryzhang/Documents/workspace/caocao-engine/src/frontend.cpp) | Render selected original-art scenes into BMP files for inspection without advancing manually. | Requires an SDL build/display or Xvfb. Fixed smoke points are rendering checks, not a full battle. |
| [build_progress_report.py](/Users/henryzhang/Documents/workspace/caocao-engine/output/pdf/build_progress_report.py) | Rebuild the previously requested ten-page progress PDF. | Uses ReportLab/Pillow, saved evidence and a local bundled font path. Content/output are dated 2026-10-01; review/update facts before reissuing it as a current report. |

### Native and Wasm test suites

Run a focused CTest target after a corresponding rule change, or the full suite at a gameplay checkpoint. The 18 native CTest registrations correspond to the following sources; the web script invokes their compiled Wasm counterparts.

| Test source | Main use case |
| --- | --- |
| [core_tests.cpp](/Users/henryzhang/Documents/workspace/caocao-engine/tests/core_tests.cpp) | Script execution, engine state, deterministic traces, snapshots, and rejection of unsupported behavior. |
| [unit_tests.cpp](/Users/henryzhang/Documents/workspace/caocao-engine/tests/unit_tests.cpp) | Imported unit data, initialization, equipment, deployment, and record behavior. |
| [animation_tests.cpp](/Users/henryzhang/Documents/workspace/caocao-engine/tests/animation_tests.cpp) | Original indexed animation frames and supported presentation/effect behavior. |
| [battle_actions_tests.cpp](/Users/henryzhang/Documents/workspace/caocao-engine/tests/battle_actions_tests.cpp) | Removal, facing, and battle-entry actions. |
| [battle_events_tests.cpp](/Users/henryzhang/Documents/workspace/caocao-engine/tests/battle_events_tests.cpp) | Battle event predicates, triggers, and event order. |
| [recovery_tests.cpp](/Users/henryzhang/Documents/workspace/caocao-engine/tests/recovery_tests.cpp) | Turn recovery, maintenance, and associated state/RNG changes. |
| [ai_tests.cpp](/Users/henryzhang/Documents/workspace/caocao-engine/tests/ai_tests.cpp) | Supported AI priority, scoring, choices, and movement behavior. |
| [movement_tests.cpp](/Users/henryzhang/Documents/workspace/caocao-engine/tests/movement_tests.cpp) | Movement distance/route rules and restrictions. |
| [range_tests.cpp](/Users/henryzhang/Documents/workspace/caocao-engine/tests/range_tests.cpp) | Attack ranges and ordered targeting. |
| [physical_resolution_tests.cpp](/Users/henryzhang/Documents/workspace/caocao-engine/tests/physical_resolution_tests.cpp) | Physical hit/damage formulas and RNG outcomes. |
| [experience_tests.cpp](/Users/henryzhang/Documents/workspace/caocao-engine/tests/experience_tests.cpp) | Character and equipment experience mutations. |
| [strike_application_tests.cpp](/Users/henryzhang/Documents/workspace/caocao-engine/tests/strike_application_tests.cpp) | Strike result application to unit state. |
| [attack_controller_tests.cpp](/Users/henryzhang/Documents/workspace/caocao-engine/tests/attack_controller_tests.cpp) | Physical controller sequencing and supported combat effects. |
| [tactic_application_tests.cpp](/Users/henryzhang/Documents/workspace/caocao-engine/tests/tactic_application_tests.cpp) | Tactic target/caster state application. |
| [objective_tests.cpp](/Users/henryzhang/Documents/workspace/caocao-engine/tests/objective_tests.cpp) | Objective-screen sequence and pauses. |
| [highlight_tests.cpp](/Users/henryzhang/Documents/workspace/caocao-engine/tests/highlight_tests.cpp) | Target highlighting and masks. |
| [tutorial_exit_tests.cpp](/Users/henryzhang/Documents/workspace/caocao-engine/tests/tutorial_exit_tests.cpp) | Guide exit, crop/mask pixels, start banner, turn-two poison display-before-HP-write, original sequential state, save continuation and cross-platform trace digests. |
| [tutorial_tests.cpp](/Users/henryzhang/Documents/workspace/caocao-engine/tests/tutorial_tests.cpp) | Supported tutorial branches, panels, movement/cancellation, menu responses, and snapshot continuation. |
| [test_importer.py](/Users/henryzhang/Documents/workspace/caocao-engine/tests/test_importer.py) | Strict resource/script import, data validity, and publication behavior. |
| [test_combat_assets.py](/Users/henryzhang/Documents/workspace/caocao-engine/tests/test_combat_assets.py) | Combat strings and supported combat asset decoding. |
| [test_game_state.py](/Users/henryzhang/Documents/workspace/caocao-engine/tests/test_game_state.py) | Dictionary schema/evidence, decoders, unknown bytes, capture integrity, dynamic pointers, diffs, and CLI lookup. |

### Available rendering smoke switches

These 50 switches are read from the current frontend source. Run one per invocation. The BMP is written in the working directory; reuse can replace a previous image.

```text
--render-after-defeat-smoke
--render-after-reinforcement-smoke
--render-all-fires-smoke
--render-archer-defeat-smoke
--render-archer-dialogue-smoke
--render-arrival-smoke
--render-attack-smoke
--render-banner-smoke
--render-battle-smoke
--render-confusion-direct-smoke
--render-confusion-smoke
--render-critical-smoke
--render-damage-smoke
--render-defeat-smoke
--render-enemy-end-smoke
--render-enemy-ready-smoke
--render-enemy-walk-smoke
--render-event-smoke
--render-fire-smoke
--render-guide-exit-smoke
--render-highlight-smoke
--render-intro-smoke
--render-next-banner-smoke
--render-next-turn-smoke
--render-objective-smoke
--render-phase-poison-smoke
--render-poison-smoke
--render-poison-status-smoke
--render-pose-smoke
--render-preview-smoke
--render-reinforcement-smoke
--render-scripted-walk-smoke
--render-second-poison-smoke
--render-selection-smoke
--render-smoke
--render-summon-dialogue-smoke
--render-summon-end-smoke
--render-summon-smoke
--render-start-banner-smoke
--render-tactic-cast-smoke
--render-tactic-cost-smoke
--render-tactic-damage-smoke
--render-tactic-preview-smoke
--render-three-fires-smoke
--render-tutorial-blue-smoke
--render-tutorial-cursor-smoke
--render-tutorial-menu-smoke
--render-tutorial-panel-smoke
--render-tutorial-range-smoke
--render-tutorial-terrain-smoke
--render-walking-smoke
```

## Practical command recipes

All commands in this section start in the engine project. Files under `.cache/` are investigation outputs; choose a fresh filename when keeping previous results.

```sh
cd /Users/henryzhang/Documents/workspace/caocao-engine
```

### Inspect state and explain a change

```sh
.venv/bin/python tools/game_state.py validate
.venv/bin/python tools/game_state.py lookup 0x004b2c5d
.venv/bin/python tools/game_state.py decode docs/evidence/game-state/tutorial.json
.venv/bin/python tools/game_state.py diff docs/evidence/game-state/cancel-before.json docs/evidence/game-state/cancel-after.json
.venv/bin/python tools/game_state.py lookup 0x00818c80 --capture docs/evidence/game-state/cancel-before.json
.venv/bin/python -m unittest discover -s tests -p 'test_game_state.py' -v
```

Dynamic buffer addresses are resolved from the capture's actual pointer/binding; the dictionary does not assume an emulator allocation address is universal. The example cancellation report names Cao Cao's Y-coordinate and the two occupancy cells that changed.

To refresh the dictionary evidence and its generated documentation:

```sh
.venv/bin/python tools/state_dictionary_probe.py --output .cache/dictionary-review
.venv/bin/python tools/game_state.py describe --output docs/GAME-STATE-DICTIONARY.md
```

### Investigate a rule and capture a fresh reference

```sh
.venv/bin/python tools/disassemble.py 0x0047fae7 0x0047fb20
.venv/bin/python tools/movement_probe.py --source /Users/henryzhang/Downloads/sgzccz --output .cache/movement-review.json
.venv/bin/python tools/strike_probe.py --source /Users/henryzhang/Downloads/sgzccz --output .cache/strike-review.json
```

Look up the function in the binary map first, consult its field definitions in the state dictionary, then compare fresh observations with the retained fixture and corresponding C++ test. An assembly address is a VA, not an EXE file offset.

### Import assets and refresh script listings

```sh
.venv/bin/python tools/import_assets.py /Users/henryzhang/Downloads/sgzccz --font /Users/henryzhang/Documents/workspace/caocao-test/vendor/fonts/CaoCaoSong.ttf
.venv/bin/python tools/script_listing.py
```

Import needs the accompanying font license and writes the generated asset tree. Decoding all instructions does not establish that the replacement implements them all.

### Develop natively, then compare platform builds

```sh
cmake -S . -B build-core -G Ninja -DCAOCAO_RENDERER=OFF
cmake --build build-core
ctest --test-dir build-core -R '^(movement|physical_resolution|attack_controller|tutorial)$' --output-on-failure
.venv/bin/python -m unittest discover -s tests -p 'test_*.py' -v
```

At a cross-platform checkpoint, with OrbStack/Docker running and imported assets present:

```sh
./scripts/build-linux.sh
./scripts/build-web.sh
./scripts/test-web-core.sh
.venv/bin/python tools/parity.py --mac-trace build-core/caocao_trace
./scripts/serve.sh
```

`parity.py` also compares the macOS core build. The browser can be opened at `http://127.0.0.1:8765/caocao.html`; browser UI/storage acceptance still needs separate verification.

### Inspect a prototype save or capture a tutorial checkpoint

```sh
./build-core/caocao_trace assets/generated --restore /absolute/path/to/backup.json
./build-core/caocao_trace assets/generated 1 --tutorial-menu-snapshot --snapshot-out .cache/tutorial-menu-review.json
```

`--restore` validates/restores the replacement state and prints it; that mode does not continue the simulation. The trace utility's snapshot options target known stable points, not arbitrary original-machine checkpoints. Never use an original `.e5s` file as a prototype snapshot destination.

For one Linux rendering check, run inside the configured Linux display/container environment:

```sh
SDL_AUDIODRIVER=dummy XDG_RUNTIME_DIR=/tmp xvfb-run -a build-linux/caocao assets/generated --render-tutorial-terrain-smoke
```

### JEV review workflow

1. Prepare a bounded question with relevant assembly, field definitions, observed results, and explicit uncertainty. The existing `exe_map_review.py` automates **function-map packets only**.
2. Choose unused request and record filenames under `docs/evidence/jev/`. Do not rerun the default packet numbering over existing immutable records.
3. Submit a selected packet using the existing client:

```sh
.venv/bin/python tools/jev_decide.py /absolute/path/to/new.request.json /absolute/path/to/new.record.json
```

4. Review the result, verify any semantic conclusion against original code, and update [JEV-DECISIONS.md](/Users/henryzhang/Documents/workspace/caocao-engine/docs/JEV-DECISIONS.md). The credential is read externally by the client; it must never be copied into a packet or this catalog. No JEV calls were needed to produce this inventory.

## Installed external analysis skill — 2026-10-04

| Skill | Use case | Installation status |
| --- | --- | --- |
| [Cerberus RE](/Users/henryzhang/.codex/skills/cerberus-re/SKILL.md) | Ghidra static analysis, decompiled C, function context, and references to complement our binary map and original-code probes. | Working static container at reviewed commit `3116c94de17386d18367ed14e55dd2c7d1c7fe14`: Java21/Ghidra12.0.4, PE import and fifteen function dossiers exported. No Frida, live bridge, companion or shared-note sync. Player-loop decompile is incomplete at the ASave patch. See [setup, evidence and limits](CERBERUS-RE.md). |

The [installation record](/Users/henryzhang/.codex/skills/cerberus-re/LOCAL-INSTALLATION.json) contains provenance and upstream file hashes. All 264 upstream files matched the reviewed commit before adding the documented local profile; 160 Python files passed syntax parsing. Original upstream instructions are retained in `SKILL.upstream.md`. These are installation checks, not runtime validation or a complete security audit. This external skill is separate from the 109 project Python tools inventoried above.

## Earlier Electron/Wine project utilities

These remain in `/Users/henryzhang/Documents/workspace/caocao-test`. They support the earlier macOS wrapper and are **not dependencies of the replacement engine**. They were inspected for this catalog, not rebuilt or rerun. The prepared Chinese font is reused as an importer input.

| Tool | Use case / output |
| --- | --- |
| [scripts/prepare-vendor.py](/Users/henryzhang/Documents/workspace/caocao-test/scripts/prepare-vendor.py) | Download/prepare Wine, GStreamer, original game copies, fonts, media compatibility copies, native helpers, and dependency notices under vendor/. Preserves the original source folder. |
| [scripts/relocate-runtime.py](/Users/henryzhang/Documents/workspace/caocao-test/scripts/relocate-runtime.py) | Merge unpacked GStreamer payloads, rewrite Mach-O library/search paths to bundled locations, and apply local ad-hoc signatures. |
| [scripts/bundle-engine.py](/Users/henryzhang/Documents/workspace/caocao-test/scripts/bundle-engine.py) | Give Ekd5.exe a macOS app/Dock identity through GameEngine.app and the Wine dispatch wrapper; leave service processes background-only. |
| [scripts/build-font.py](/Users/henryzhang/Documents/workspace/caocao-test/scripts/build-font.py) | Generate CaoCaoSong.ttf from Source Han Serif with fixed-cell-compatible names/metrics while preserving outlines. |
| [scripts/extract-icon.py](/Users/henryzhang/Documents/workspace/caocao-test/scripts/extract-icon.py) | Read the original PE icon and create macOS icon assets with sips/iconutil. |
| [scripts/copy-ui.mjs](/Users/henryzhang/Documents/workspace/caocao-test/scripts/copy-ui.mjs) | Copy launcher UI into the compiled output and Electron license notices into vendor/. |
| [scripts/after-pack.cjs](/Users/henryzhang/Documents/workspace/caocao-test/scripts/after-pack.cjs) | electron-builder hook applying the final local ad-hoc app signature. |
| [scripts/verify-bundle.py](/Users/henryzhang/Documents/workspace/caocao-test/scripts/verify-bundle.py) | Check packaged game hashes, deep signature, relative library dependencies, and symlink containment. Does not prove a complete gameplay session. |
| [native/game-host.c](/Users/henryzhang/Documents/workspace/caocao-test/native/game-host.c) | Windows helper used by the legacy wrapper to find/launch and monitor the game, track its window, and clean up the music helper after exit. |
| [native/compat-probe.c](/Users/henryzhang/Documents/workspace/caocao-test/native/compat-probe.c) | Windows/Wine compatibility diagnostic for code pages, font selection/glyph availability, and MCI media opening/playback. |
| [test/storage.test.cjs](/Users/henryzhang/Documents/workspace/caocao-test/test/storage.test.cjs) | Legacy launcher storage/save-initialization regression tests. |
| [test/runtime.test.cjs](/Users/henryzhang/Documents/workspace/caocao-test/test/runtime.test.cjs) | Legacy runtime path/environment configuration regression tests. |
| [package.json](/Users/henryzhang/Documents/workspace/caocao-test/package.json) | npm entry points for build, start, test, vendor preparation, helper compilation, app/DMG packaging, and bundle verification. |

The earlier wrapper's tested behavior and limitations are recorded in [VALIDATION.md](/Users/henryzhang/Documents/workspace/caocao-test/docs/VALIDATION.md). Run its npm commands from the Electron project, not from the engine directory.

## Planned tools versus what exists today

| Priority | Current status | What remains |
| --- | --- | --- |
| 1. Game state dictionary | Implementation 1.0.2 exists: shared JSON, lookup/decoder/diffs, probe integration, and tests. | Expand verified fields as new behavior is recovered; preserve unknowns and evidence scope. |
| 2. Original checkpoint replay and comparison runner | **Not built.** Replacement snapshots/trace tools and original memory observations provide parts of the groundwork. | Restore a complete original VM checkpoint, replay commands identically, and compare original/replacement state and events. |
| 3. Shared hook contracts | **Not built as a standardized system.** ProbeVM and individual probes have explicit hooks/boundaries. | Shared typed contracts, side-effect/RNG assertions, and migration of probe-specific hooks. |
| 4. Searchable dependency/evidence index | **Partially served by existing static references.** Binary map, dictionary evidence links, script listings, and this catalog are available. | Unified queryable relationships between functions, fields, rules, fixtures, and implementation. |
| 5. Automatic JEV investigation packets | **Function-map packet preparation exists.** Other decisions still use manually prepared requests. | General packet generation with relevant state/rules/callers/tests and verification feedback. |

## Keeping this catalog accurate

- Add a row under the correct category for every new reusable tool. Record its path, use case, input/output contract, supported scope, and known boundaries. Mark library helpers and exploratory probes clearly.
- Update the command recipes when flags or prerequisites change. Keep source inspection separate from a successful execution/test claim.
- After semantic changes, update the relevant fixtures, field/schema evidence, and dated compatibility report. Preserve original game files and user saves.
- Before packaging again, update the stale version/text in `package-checkpoint.py` and the version-specific assumptions in `check_relocation.py`, then rerun their checks. This catalog does not refresh those artifacts by itself.
- Check for new engine tool/build files with `rg --files tools scripts`. Compare that list with this catalog; all 109 current Python tools, three shared JSON definitions, eleven engine script/config files, and 21 test sources have entries above. Additions after this inventory date may need new entries.

## Simplified prototype tools (2026-10-04)

- `scripts/build-prototype.sh [all|linux|web]`: build only the prototype targets;
  run deterministic mechanics, persistence, Linux SDL and cross-platform checks.
- `scripts/stage-prototype-assets.py`: verify original resource hashes and UI,
  class, terrain and name glyph coverage; stage the 174 required resources without modifying the full importer output.
- `scripts/package-prototype.py`: make separate Linux/browser release folders,
  launch scripts, dependency notices and SHA-256 inventories.
- `scripts/package-prototype-mac.py`: package the local Apple Silicon C++/SDL
  executable, original assets and notices into `dist/prototype-macos/颍川之战.app`;
  apply/verify a local ad-hoc signature and emit a SHA-256 inventory. No Wine,
  Electron, external dylibs or notarization. Run after building `build-mac`.
  Also packages `dist/prototype-macos-executable/caocao-prototype`, adjacent
  assets and `Start Game.command`. `--executable-only` leaves the app unchanged.
- `tests/prototype_tests.cpp`: continuous victory/defeat, movement rollback,
  per-action save/restore, corrupt/incompatible rejection and portable next-attack fixtures.
- `tests/prototype_browser_check.js`: developer-only Chrome command-trace,
  victory/defeat and real IndexedDB tests; complement with actual canvas input checks.
- `caocao_prototype_app ... --smoke OUTPUT` / `--resume-smoke`: native SDL
  portrait/landscape input, letterbox/map bounds, modal input blocking, all
  toolbar actions, rendering/save checks and a separate-process load check. Set
  `CAOCAO_PROTOTYPE_SAVE_DIR` to an isolated test directory.

Read `docs/PROTOTYPE.md` for usage, deliberate rule simplifications and evidence.

## Screenshot layout evidence (2026-10-05)

- `tools/prototype_layout.py`: read-only, SHA-gated check of the exact supplied
  `Sv00s.e5s` used in the original screenshot. Verifies 115 packed slot indices,
  22 visible unit records and 27 fire records against
  `tests/fixtures/prototype-electron-layout.json`. `--write-header` regenerates
  `include/caocao/prototype_layout.hpp`; without it checks for drift. Rejects
  other saves. Unknown bytes retain raw evidence; this is not a general importer.
- Asset staging now extracts the original 16×16 confusion marker from
  `Mark.e5@0x1a2c0` using the original palette, hashes the source and output, and
  records provenance in `prototype-package.json`. Uses Python's standard library;
  the existing original manifest remains unchanged for save compatibility.
- Prototype tests now compare the exact layout, all friendly fire exclusions,
  confusion continuation, turn-limit defeat, and immutable schema-1 next-attack
  fixture. The native render smoke and browser use the same shared frontend.
- Cerberus `0x43eb1f` dossier: static sprite/status composition, including the
  confusion marker's resource offset, dimensions and alternating X offsets.
  Static interpretation plus rendered inspection; not a complete raster probe.

## 2026-10-05: prototype attack playback checks

- `PrototypePlayback` (`include/caocao/prototype_playback.hpp`,
  `src/prototype_playback.cpp`) consumes original presentation events without
  owning simulation/RNG state. It is shared by native SDL and WebAssembly.
- `tests/prototype_playback_checks.hpp`, run by `prototype_tests`, checks
  consume-once attack packets/sounds, tick boundaries, pre-impact HP, lethal
  outcomes, restoration and144 roster/facing asset combinations. Its checks
  are separate from the historical365-checkpoint simulation trace.
- `caocao_prototype_app ASSETS --attack-smoke OUTPUT_DIR` captures the actual SDL
  attack frames and tests input/storage guards, modal pause, transactional restore
  and enemy scheduling. Requires `CAOCAO_PROTOTYPE_SAVE_DIR` to isolate test saves.
  Use `SDL_AUDIODRIVER=dummy` for headless audio queue validation.
- `scripts/stage-prototype-assets.py` now stages210 hash-checked resources,
  including indexed attack/reaction records and original swing/hit WAVs.
- `tests/prototype_browser_check.js` waits for `prototype_busy()` to become false
  before following animation-sensitive commands or snapshots. It remains a
  development harness; it is not loaded into the shipping page.

Usage/evidence and reproduction commands: [PROTOTYPE-ATTACK-ANIMATION.md](PROTOTYPE-ATTACK-ANIMATION.md).

### Follow-up enemy attack regression

`caocao_prototype_app ASSETS --enemy-animation-smoke OUTPUT_DIR` runs the actual
timed frontend from a new game through the first confused phase and following
normal enemy phase. It checks that every HP reduction starts a presentation,
retains pre-impact HP, and displays all four swing frames, flash and number for
at least 1 second; writes screenshots and `result.json` without saving user progress.
The initial recorded run covers 14 skipped confused actions and 7 normal attacks.

### Slow-render enemy regression (0.2.2)

`--enemy-animation-slow-smoke OUTPUT_DIR` adds 120ms between rendered frames.
It exposed the previous 100ms catch-up skipping swing frame10 and impact ticks13/14.
Both native modes now accept omitted ASSETS to test bundled asset discovery.
`Start Game.command` forwards arguments, so test the exact standalone launcher
from an unrelated directory. Normal no-argument launch remains unchanged; the
window title identifies the current build. These diagnostics do not write save slots.

### Mixed player/enemy regression and local diagnostics (0.2.3)

`--mixed-enemy-animation-smoke OUTPUT_DIR` starts with a player move/attack through
the frontend's tile-click path, then uses End Turn twice. It checks the confused
phase and subsequent enemy attacks with the pointer still over the toolbar,
including action labels, pre-impact HP, every swing frame, flash and number.
An independent headless replay must reach the same state hash. Captures include
`enemy-confused.bmp`, each enemy attack and `result.json`. This mode does not
save user progress; use it with the exact `Start Game.command` launcher to check
the bundled binary/assets after a player action.

Normal native play records commands, loaded snapshots and attack presentation
in `~/.local/share/caocao-prototype/diagnostics/session-*.jsonl`. Use these to
investigate the user's real action sequence rather than relying only on smoke
tests. `CAOCAO_PROTOTYPE_LOG_DIR` selects a separate directory and enables logs
for test modes. `attack_begin` identifies actor/target/turn; `attack_presented`
records poses after successful SDL presents; `attack_end` records whether a
flash was presented; `enemy_action` records skips/waits. Renderer success does
not itself verify a physical display. Logs stay local, are capped at4MiB each,
and never overwrite save slots.

### Walking playback regression (0.2.4)

`--movement-smoke OUTPUT_DIR` captures a native five-tile player walk with120ms
between renders, checking all20 original movement poses, input/save guards,
modal pause, cancellation and valid/corrupt restoration. Set
`CAOCAO_PROTOTYPE_SAVE_DIR` to a temporary directory; the mode refuses to run
without that override. It writes `walk-NNN.bmp`, `destination.bmp` and
`result.json`. The existing mixed/slow enemy modes now include walking before
attacks; reported durations include preceding movement.

`tests/prototype_walking_checks.hpp`, included by `prototype_tests`, checks499
legal current/legacy paths, full-map coordinates, allied pass-through overlays,
final-frame visibility and queued enemy move/attack presentation. Use it after
changing movement routes, rendering integration or snapshot/playback boundaries.
Session JSONL logs add `movement_begin`, `movement_presented`, `movement_end`.
See [walking report](PROTOTYPE-MOVEMENT-ANIMATION.md).

### Original battlefield dialogue and snapshot checks (0.2.5)

`src/prototype_dialogue.cpp` builds a speech catalog directly from hash-checked
`S_00.json`, retaining instruction/event offsets. Use it to trace a visible line
to its source and to inspect exact adjacency/retreat conditions. It implements
the supported speeches in the simplified prototype, not general script execution.

`tests/prototype_dialogue_checks.hpp` runs production-mode full battles and
focused trigger/priority, branch, migration, exact-page and rejection cases.
`prototype_tests ASSETS OUTPUT` additionally exports `OUTPUT.dialogue.json` and
its `.next.json`, usable with `prototype_tests ASSETS --resume SNAPSHOT` on native
or Wasm to compare next-page state across processes/platforms. Historical animation
fixtures explicitly disable conversations; the new test runs the normal mode.

`--dialogue-smoke OUTPUT_DIR` uses the real SDL renderer and keyboard/click input
for animation-before-speech, enemy pause, F5/F9 exact-page restore, portraits,
all supported speech types and line/glyph bounds. It writes BMPs and result.json,
and requires `CAOCAO_PROTOTYPE_SAVE_DIR` to protect user saves. Follow it with
`--resume-smoke` under the same override to verify fresh-process recovery.
`prototype_dialogue()` exposes the current page in the Wasm diagnostic API.

`tools/battle_events_probe.py --output PATH` regenerates159 original x86 selector/
condition cases without modifying the retained fixture. `battle_events_tests`
compares the recovered C++ predicates. Cerberus dossier `0x4100a1` explains the
selection loop; JEV call064 reviews the persistence/action-boundary design.
See [evidence and limitations](PROTOTYPE-DIALOGUE.md).

## Reusable reconstruction skill — 2026-10-06

[legacy-game-reconstruction](/Users/henryzhang/.codex/skills/legacy-game-reconstruction/SKILL.md)
captures the workflow that produced the playable native battle through0.2.5.
Invoke it when extending original-game behavior, investigating a mismatch, or
reusing this approach for another legacy title. It covers source/asset evidence,
Cerberus and logged JEV decisions, deterministic simulation, animation sequencing,
dialogue, compatible saves, focused native/Wasm checks and the actual user package.

Its supporting references separate the general method from
[this project's case study](/Users/henryzhang/.codex/skills/legacy-game-reconstruction/references/caocao-case-study.md),
including code/tool locations, corrected failures, build commands and remaining
fidelity limits. It is an installed Codex workflow, not a new engine feature or
evidence that the full original acceptance checklist is complete.

## 2026-10-06 additions: standalone battle expansion

| Tool | Use | Evidence boundary |
| --- | --- | --- |
| `tools/prototype_scenarios.py --source GAME_DIR --verify-original` | Import S01–03, original map pixels/terrain, deployment/dialogue catalogs and extended font; check deployment through original x86 handlers. | Declared party and simplified events; no full campaign interpreter. Keeps `assets/generated` unchanged. |
| `prototype_scenarios_tests ASSETS [OUTPUT_PREFIX]` | Command-driven victories, defeat/limits, per-step save continuation, animation asset coverage; exports traces and pre-attack snapshot/next hashes. | Shared simplified engine, not original gameplay equivalence. `--resume FILE` checks a cross-platform next action. |
| `scripts/check-prototype-scenarios.sh [all\|mac\|linux\|web\|parity]` | Build/run platform tests and compare checkpoint traces; native renderer and fresh-process save tests. | Browser app compiles; Node/Wasm tests do not by themselves test browser UI. |
| `caocao_prototype_app ASSETS --scenarios-smoke OUTPUT_DIR` | SDL battle chooser, scaled coordinates, intro pages, walking, independent saves and victory buttons. | Requires isolated `CAOCAO_PROTOTYPE_SAVE_DIR`; synthetic outcome fixture tests controls only. |
| `caocao_prototype_app ASSETS --scenarios-resume-smoke OUTPUT_DIR` | Load all three native saves in a fresh process against recorded hashes. | Requires the smoke's same isolated directory. |

`--scenarios-combat-smoke OUTPUT_DIR` additionally presents each new battle’s first
enemy walking/attack/hit-flash sequence with the native renderer.

See [additional-battle report](PROTOTYPE-ADDITIONAL-BATTLES.md).

## Prototype balance and JEV runtime (0.4.0, 2026-10-06)

| Tool | Use case |
|---|---|
| `tools/prototype_balance_probe.py` | Execute original deployment/construction for all four declared standalone party presets; verify NPC level, promotion and equipment. Read-only originals; logs hook boundaries. |
| `scripts/check-prototype-balance.sh mac/linux/web/parity` | Compare108 NPCs and recorded-choice save traces across targets; legacy/corruption tests; native render and offline JEV tests. |
| `tests/prototype_balance_tests.cpp --recorded` | Replay a saved live JEV decision on another platform without network. Pass assets, `--recorded`, evidence result JSON. |
| `tools/prototype_balance_autoplay.cpp` | Observe corrected battle outcomes with the existing greedy player; does not certify human difficulty. Compile against the two core static libraries. |
| Native `--balance-smoke OUTPUT` | Render corrected rosters/stat display/AI information screen with isolated saves and no JEV calls. |
| Native `--hover-smoke OUTPUT` | Render the original-style hover panels for all 130 units; check nonmutating inspection, scaled pointer input, delay/dismissal, edge placement and click-through. Requires an isolated save directory; no JEV calls. See [panel report](PROTOTYPE-HOVER-PANEL.md). |
| Native `--menu-smoke OUTPUT` | Exercise the five-row unit action popup through SDL input in all four scenarios: availability, keyboard, scaling/edges, consecutive-unit move completion, no pre-move menu on selection/repeated clicks/Space, rollback, save/load and identical attack/RNG results. Uses isolated saves and local AI. See [menu report](PROTOTYPE-ACTION-MENU.md). |
| Native `--jev-smoke OUTPUT SCENARIO` | Opt-in API test of actual asynchronous turn scheduler, cached/validated choice, animation and pending-decision replay. Requires external key and isolated save path; sends game state, consumes API credit. |
| `tests/jev_enemy_tests.cpp` | Validate typed JEV requests, illegal/malformed choices, missing-credential fallback, cache and cancellation without network. |
| `src/jev_enemy.cpp` | Native fixed-endpoint HTTP worker; bounded curl process, safe credential handling, every-attempt logs and deterministic decision cache. Excluded from Wasm. |

See [balance, JEV and compatibility](PROTOTYPE-BALANCE-JEV.md). Runtime JEV is an
alternate opponent; original-code fixtures remain the authority for game rules.

### Basic prototype tactics — 0.5.0

| Tool | Use case |
| --- | --- |
| `tools/import_prototype_tactics.py ORIGINAL_GAME` | Read-only extraction of five offensive MEFF effects, original sounds and spell metadata into a separate versioned/hash-verified asset set. |
| `tools/prototype_tactics_probe.py` | Run 750 bounded original x86 hit/damage/estimate/RNG cases for spells 0/5/10/15/33; no Windows loader or user saves. |
| `prototype_tactics_tests ASSETS OUTPUT` | Compare original fixtures; test four caster scenarios, MP/poison snapshots, migration, legal JEV choices and deterministic replay; compare output traces between macOS/Linux/Wasm. Offline. |
| Native `--tactics-smoke OUTPUT` | Capture actual enemy poison/fire playback and player spell picker/targeting; test MP/status save/load with isolated saves. Offline, uses explicit legal-contact caster fixtures. |
| `jev_tactics_tests ASSETS OUTPUT` | **Makes two live JEV calls** using the native client, executes accepted choices and validates pending-choice replay. Requires external credentials; not an automatic offline regression test. |

See [tactics report](PROTOTYPE-TACTICS.md) for implemented spells and visual/rule limits.


## 2026-10-06 — terrain benefits and location pickups

| Tool | Use case / evidence boundary | Output |
| --- | --- | --- |
| `tools/prototype_terrain_assets.py [ORIGINAL_DIRECTORY]` | Edition-gated, read-only extraction of eight exact S_00..03 position/flag/grant events, three original Item.e5 images, Se14 and a supplemental Chinese atlas. Does not infer loot from terrain names. | `assets/terrain-benefits/`, separate verified manifest |
| `tools/terrain_pickup_probe.py` | Executes the original condition dispatcher on 480 synthetic occupancy/lifecycle/flag cases and 18 original stock increments. Does not run the whole animated grant body. | `tests/fixtures/original_terrain_pickups.json` |
| `prototype_terrain_tests ASSETS [REPORT]` | Native/Wasm original-condition comparisons, actual phase recovery, cancel/commit, eight pickups, transactional snapshots, migration and JEV context/replay. `--resume SNAPSHOT` verifies portable restoration and the next command. | Report, request fixture and snapshot |
| `caocao_prototype_app ASSETS --terrain-smoke OUTPUT` | Actual native rendering/input/presentation/storage paths with explicitly arranged legal test positions; requires isolated save directory. No full-battle equivalence claim. | Pickup/healing/inventory captures, saved snapshot and result |
| `jev_terrain_tests ASSETS OUTPUT` | Explicit **one-live-request** integration check with terrain-aware choices and deterministic saved decision replay. Not an ordinary offline test. | Durable request/response, chosen legal action and replay result |

See [terrain release report](PROTOTYPE-TERRAIN.md) and [JEV ledger](JEV-DECISIONS.md). These entries supplement the older inventory above.

## 2026-10-07 additions — consumables

| Tool | Use case | Evidence / limits |
| --- | --- | --- |
| `tools/prototype_items_probe.py` | Original manual item effects, stock/action commit and target range | 324 manual cases + eight range cases; documented UI/presentation hooks, not visual acceptance |
| `tools/prototype_item_assets.py ORIGINAL_DIR` | Edition-gated Se41 and Chinese font supplement | Separate manifest preserves older terrain snapshots; never rewrites original game |
| `prototype_items_tests ASSETS [OUTPUT]` | Rule fixtures, self/ally use, inventory invariants, migration, corrupt saves, deterministic replay | macOS/Linux/Wasm; `--resume SNAPSHOT` for cross-platform restoration |
| `caocao_prototype_app ASSETS --items-smoke OUTPUT_DIR` | Actual SDL item buttons, selection/cancel, feedback, post-move menu and F5/F9 | Requires fresh `CAOCAO_PROTOTYPE_SAVE_DIR`; synthetic legal placements |
| `jev_items_tests ASSETS OUTPUT_DIR` | New player-item context and persisted JEV decision replay | Explicit one-live-call integration test; output includes durable request/response log |

Details and reproduction: [consumables report](PROTOTYPE-ITEMS.md).

## Detailed unit inspector (0.8.0, 2026-10-07)

| Tool | Use case | Output / boundary |
| --- | --- | --- |
| `tools/prototype_unit_assets.py` | Stage all104 original Item images,68 tactic records and a Chinese font supplement | Verifies original-import hashes; separate UI manifest leaves save identities unchanged |
| `unit_details_tests ASSETS [REPORT]` | Original tactic-record comparison,130-unit details, empty equipment/MP handling, nonmutating snapshots and cross-platform parity | macOS/Linux/Wasm detail hash; no model calls |
| Native `--unit-details-smoke OUTPUT` | Render all390 roster tabs and check scaled input, Ctrl-click, modal isolation, pending moves, enemy pause and saves | Requires isolated save path; captures/verification JSON; no network calls |

See [unit-details report](PROTOTYPE-UNIT-DETAILS.md).

## Revised battle integration (2026-10-08)

| Tool | Use case | Evidence boundary |
|---|---|---|
| `tools/prototype_scenario_events_probe.py` | Recover occupied-rectangle, army-count and duel predicates | 1,240 hash-gated original x86 cases |
| `tools/prototype_pursuit_probe.py` | Verify 方天画戟 follow-up gate and target order | 72 bounded original dispatch cases |
| `tools/prototype_extended_tactics_probe.py` | Verify healing, support, status, area-spell arithmetic and XP | 1,500 original helper cases; not complete presentation |
| `tools/import_prototype_extended_tactics.py` | Rebuild a separate, hashed 15-spell original-art/audio supplement | Read-only original resources; old manifests unchanged |
| `scripts/check-revised-gameplay.py BUILD LABEL` | Run combat, event, tactic, campaign and complete battle trace checks on native or Wasm | Structured per-platform reports; no network calls |
| `caocao_prototype_app --revised-smoke OUTPUT` | Exercise native counters/healing, ten slots and backup restoration | Requires isolated `CAOCAO_PROTOTYPE_SAVE_DIR` |
| `jev_revised_tests ASSETS OUTPUT [--live]` | Validate current JEV context, decision persistence and cache | Offline default; `--live` makes one HTTP request |

See [release details and remaining limits](REA-GAMEPLAY-FIXES.md).

## Native diagnostic logging — 2026-10-08

`diagnostics_tests ASSETS ISOLATED_OUTPUT` validates log rotation/retention, concurrent sessions, recovery files, corrupt-checkpoint rejection and nonfatal logging failure. Native `--revised-smoke` now checks that recovery snapshots do not change during animation and match the engine afterward; set `CAOCAO_PROTOTYPE_LOG_DIR` to an isolated root to enable logs in smoke mode. Ordinary native launches enable them automatically. See [diagnostic paths, recovery procedure and bounds](PROTOTYPE-DIAGNOSTICS.md).

## Web deployment tools — added 2026-10-09

| Tool | Use case | Limits |
| --- | --- | --- |
| `web-game/scripts/import-presentation.py` | Read-only original title/palette and MP3 extraction with hashes. | Hash-gated supplied edition. |
| `web-game/scripts/build.mjs` | Bundle the web UI, version engine artifacts and split data below Cloudflare per-file limits. | Requires current `build-web` output. |
| `web-game/scripts/install-secrets.mjs` | Upload dedicated OAuth/JEV secrets via stdin; preserve session secret. | Checks the exact account/Worker; private files stay external. |
| `web-game/tests/accounts.test.ts` | Signed-session D1 isolation, save CAS/rollback, ranking and JEV fallback tests. | Local temporary database, not proof of Google OAuth itself. |
| `prototype_web_campaign_tests` | Export four schema10 victory/restore fixtures and compare native/Wasm hashes. | Synthetic casualties, not a real playthrough. |
| `web-game/scripts/seed-ui-fixtures.py` | Populate empty local UI slots 6–9 for campaign transition checks. | Isolated loopback test database only; refuses overwrites. |

See [deployment verification](WEB-GAME-DEPLOYMENT.md) and [commands](../web-game/README.md).

## Phone interaction tools — 2026-10-09

- `web-game/client/game-gestures.ts`: reusable camera transforms and touch
  arbitration; keeps long-press/pinch/pan from emitting battle commands.
- `web-game/client/game-touch.ts`: browser/SDL bridge, pointer capture,
  information-window framing and mobile zoom controls.
- `web-game/tests/game-gestures.test.ts`: deterministic threshold, cancellation,
  multi-touch, coordinate round-trip, bounds and camera-restore regressions.
- `prototype_web_tap`, `prototype_web_inspect`, `prototype_web_close_details`,
  `prototype_web_ui_state`: narrow hosted presentation entrypoints using logical
  game coordinates. Inspection does not cancel pending movement or consume RNG.

See [phone controls and test evidence](WEB-GAME-PHONE-CONTROLS.md).

Phone controls web.3: `GameGestures` supports one-finger panning after the 10px
threshold and seamless pinch-to-pan. `Prototype::player_turn_complete()` is a
read-only eligibility query for hosted automatic end-turn; the frontend waits
for animation/dialogue/modal completion and reuses `end_turn`.
`prototype_auto_turn_tests` runs in native and Node/Wasm builds to check action
eligibility and save/restore equivalence. `prototype_web_mobile_text(enabled)`
only changes glyph presentation, preserving snapshot and manifest identities.

`tools/generate_battle_font.py` builds the browser battle's Noto Sans CJK SC Bold
coverage atlas. Use it when changing battle typography or adding glyphs: it
checks pinned font/license bytes, glyph coverage and ink bounds, and retains the
canonical metrics so old snapshots remain compatible. Regenerate with the
project venv, then rebuild the Wasm app. The font manifest triggers relinking.

## Battle localization (web.6)

| Tool | Use case |
| --- | --- |
| `tools/build_battle_translations.py` | Build the English presentation catalog with source hashes; check four-battle scripts, UI, unit/item/class data, combat cries and font coverage before release. Edit the source TSV/JSON under `assets/localization`, then regenerate. |
| `battle_language_tests` | Verify exact translations and composed messages on native and Node/WebAssembly, including Chinese/English switching without changing source strings. |
| `prototype_web_ui_state` / `prototype_web_translate` | Inspect presentation language and translation output during local browser UI checks; no gameplay commands or credential access. |

Scope, build commands and evidence: [Battle language switch](WEB-GAME-LANGUAGE.md).

## Site translation catalog and coverage checks

- `web-game/client/site-language.ts`: 167 English site strings, dynamic values,
  stored language preference and locale selection; shared by homepage/dialog UI.
- `web-game/tests/site-language.test.ts`: detects untranslated static markers,
  server errors, dynamic site messages and mismatched substitution placeholders.
- Use `npm --prefix web-game test`; no game assets, saves or credentials required
  for the translation checks. The full suite also runs isolated backend tests.
- `web-game/tests/ui-worker.ts` supports `/__test/login?first=1` for disposable
  local first-login UI tests. This wrapper must never be deployed.

## Original full-campaign battle and unit atlas (2026-10-10)

- `tools/campaign_atlas.py`: reads all 117 original EEX files without changing them; preserves byte offsets and raw strings, records A140 display anomalies, exports listings and scenario jumps. Use for whole-campaign route investigation.
- `tools/campaign_rosters.py`: extracts all 58 battle deployment tables, party selection limits, candidate pools, and appearance/revival/hide/retreat conditions. Use to distinguish selected troops, hidden reserves, scripted returns, and possible unused records. Conditions are static lexical guards, not proof of complete runtime reachability.
- `tools/campaign_verify.py`: runs bounded original deployment handlers for all 2,378 nonempty ally/enemy rows; checks ID, slot, XY and hidden state. Also checks the original R32 route comparison at 12 boundary inputs. No Windows launch or original-save writes.
- `tools/campaign_atlas_render.py`: renders the campaign flow PNG, complete unit atlas PNG, and 58 individual battle PNGs using bundled Chinese fonts. Reuse when campaign interpretations are corrected.
- `tools/campaign_report.py`: emits the evidence report and verifies original input hashes and all 512 hero names. Does not establish full-campaign gameplay equivalence.
- Results: `output/campaign-atlas/REPORT.md`, `campaign-flow.png`, `all-battles-units.png`, `battle-pngs/`, `battles.json`, `all-units.csv`, and verification JSON files. One logged JEV methodology review is stored with these artifacts.

## 2026-10-13: Cao Cao native sprite export

- `tools/export_caocao_animation_events.cpp`: exports all three Cao Cao forms through the existing native movement, physical attack, reaction and scripted-animation helpers. Produces isolated event streams for four directions; does not mutate game assets or saves. Compile against the current `build-mac/libcaocao_core.a`.
- `tools/export_caocao_sprites.py`: validates decoded archive hashes, exports all 84 lossless character frames, transparent atlases and labeled PNG sheets, and builds 180 isolated GIF previews from those event streams. Also exports the existing voxel cutout, without inventing per-action voxel artwork.
- Deliverables: `output/character-art/caocao-native-sprites/index.html`, `manifest.json`, `verification.json`, and adjacent ZIP. GIF previews omit audio, other units and shared spell particles; script-helper support does not mean every animation occurs in the four-battle campaign.

## 2026-10-13: Cao Cao voxel sprite library

- `tools/package_caocao_voxel_sprites.py`: packages 28 imagegen-authored mounted Cao Cao poses into transparent 512px/256px frames, native-slot-compatible atlases, a labeled overview, a local interactive gallery and 60 GIF/WebP preview pairs. Use after replacing a retained source sheet or adjusting its crop/ground anchor; it makes no image-service calls and changes no game assets.
- Source artwork, all generation prompts, source hashes, frame rectangles and animation timing are retained under `output/character-art/caocao-voxel-sprites/`. Rebuild with `.venv/bin/python tools/package_caocao_voxel_sprites.py`; the adjacent ZIP is recreated and checked.
- Preview poses use the native frame order and waits, in place. Terrain overlays must not reset walking poses. Original palette effects, sound and shared spell particles are omitted. This is a new 2D voxel-style library for one mounted design, not a rigged 3D model or an installed game update.

## 2026-10-13: Reusable voxel character packaging and Dong Zhuo

- `tools/export_character_animation_events.cpp`: read-only exporter for a character present in the special-battle seed. Uses that character's actual ID, class and side to select its native sprite and exports 56 directional helper sequences. Compile against `build-mac/libcaocao_core.a`; arguments are `ASSETS CHARACTER_ID OUTPUT.json`. Dong Zhuo is character 127, class 39, enemy sprite 101.
- `tools/package_voxel_character.py OUTPUT_FOLDER`: reusable version of the Cao Cao packager. Reads `package-config.json` for character identity, generated-sheet crop rectangles, ground anchors and scales; uses `reference/native-events.json` for timing. Creates 28 transparent 512px frames, 256px copies, group/combined atlases, a labeled overview, 60 GIF/WebP preview pairs, local HTML gallery, metadata and checked ZIP. Does not generate artwork or mutate game assets.
- Dong Zhuo deliverables: `output/character-art/dongzhuo-voxel-sprites/` and adjacent ZIP. Preserves his blue/gold chariot-and-spear design. Includes original pixel-frame/portrait references, source hashes, all built-in imagegen prompts and `gallery-checks.json`. Asset review/export only; game integration remains separate.

## Special Hulao duel integration (2026-10-13)

- `scripts/stage-special-hulao.py`: verifies and losslessly crops six approved 28-frame libraries into runtime RGBA resources, retaining ground pivots and source hashes.
- `src/prototype_duel.cpp`: builds the isolated chosen-player / random-opponent scenario from Henry's immutable seed; shared prototype rules handle combat.
- `src/prototype_voxel.inc`: maps original movement/attack/special presentation packets to the authored voxel frames.
- `special_hulao_tests`: deterministic native/Wasm duel, save continuation, outcome and corruption checks. Gate checks in `tests/special_hulao_gate_checks.hpp` reproduce schema11 closed entry, verify the two-cell adaptation, schema11/12-to-13 migration, entry/exit routes, blocked walls and enemy pursuit.
- `caocao_prototype_app --special-smoke OUTPUT`: eight-matchup renderer, targeting and isolated native save checks, including visible gate crossing and restored courtyard move/cancel; requires `CAOCAO_PROTOTYPE_SAVE_DIR`.
- `scripts/package-special-hulao.py`: separate signed Mac app and launcher. See [current behavior and evidence](SPECIAL-HULAO.txt).

## 2026-10-13: Selectable voxel heroes

- `tools/package_voxel_character.py`: now used for Xiahou Yuan, Xiahou Dun, Li Dian and Lu Bu. Each retained output folder includes artwork, prompts, native timing, crop/pivot configuration, 28 frames, atlases and 60 animation previews; rerun after approved art edits. Combined index: `output/character-art/hulao-duel-heroes/index.html`.
- `src/prototype_special_choice.inc`: shared native/Wasm chooser rendering. `select_special_hero` draws a random opponent once, independently of combat RNG. Useful for adding another selectable hero without copying battle logic.
- `tests/special_hulao_hero_checks.hpp`: eight-matchup fixture validation, source stat preservation, chosen-hero defeat, strict roster validation and deterministic save continuation.
- `src/prototype_special_smoke.inc`: click each card, render both sides of each matchup, restore saves while choosing and validate isolated inspection. See `docs/evidence/special-hulao-heroes/` and [release instructions](SPECIAL-HULAO.txt).
