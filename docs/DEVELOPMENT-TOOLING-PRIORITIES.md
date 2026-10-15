# Development tooling priorities for the opening battle

Saved October 3, 2026. Status: priority 1 has an initial implementation; priorities 2–5 remain proposed.

The [development tools catalog](TOOLS-CATALOG.md) tracks existing tools, their use cases, command examples, and the remaining gaps behind these priorities.

These five priorities aim to reduce repeated reverse-engineering and debugging work while completing the opening-battle acceptance checklist. Extend the existing original-code execution harness, specialized probes, script definitions, fixtures, and cross-platform comparisons. Use the [Ekd5 binary feature map](../output/reverse-engineering/Ekd5-binary-feature-map.txt) as the starting index.

## Priority 1 Game state dictionary

Implemented version 1.0.0: [dictionary and usage](GAME-STATE-DICTIONARY.md), backed by a shared JSON schema, memory decoder, address lookup, and field-level difference reports. It covers 112 field definitions across 21 structures/buffers, focused on tutorial exit, movement/cancellation, and physical attacks. Unknown fields remain explicit. Diagnostic captures do not implement the checkpoint replay runner in priority 2.

Describe each known memory field: name, offset, type, signedness, flags, and supporting evidence. Record uncertain interpretations explicitly.

Use the dictionary to turn memory changes into explanations such as “Cao Cao’s action flag changed,” rather than unexplained byte differences. Share the definitions across probes, reports, and investigation packets to reduce inconsistent interpretations.

## Priority 2 Checkpoint replay and comparison runner

Restore an original-game checkpoint, execute one action, run the corresponding action in the C++ engine, and report the first difference in state, random-number generation, or event order.

Preserve the execution context needed for reliable replay, including relevant memory, registers, stack, RNG state, and documented hooks. Identify checkpoints by executable and harness versions. This should avoid repeatedly reconstructing the opening sequence and make mismatches easier to reproduce.

## Priority 3 Shared hook contracts

Document what each substituted Windows or UI routine reads, changes, and returns, including calling convention and side effects. Reuse consistent implementations where their assumptions match.

Catch inconsistent substitutes before they become misleading reference results. The earlier stale camera-cursor hook is a concrete example: the substitute omitted a cursor clear performed by the original camera routine, creating an apparent gameplay mismatch.

## Priority 4 Searchable dependency and evidence index

Connect each feature to its callers, memory fields, script handlers, existing probes, C++ implementation, and unresolved branches.

Make it possible to find the smallest missing piece blocking progress and distinguish observed behavior from static interpretations and unknowns. Extend the existing function inventory instead of maintaining another disconnected map.

## Priority 5 Automatic JEV investigation packets

Extend the existing packet generator with relevant callers, field definitions, observed differences, and competing explanations. Include an insufficient-evidence option.

Give JEV better evidence with less manual preparation. Retain request/response records and analyst decisions. JEV recommendations remain advisory; original executable evidence determines correctness, and prior model approval must not be treated as verification.

## Recommended implementation order

Build the game-state dictionary and checkpoint replay/comparison runner first. Add hook contracts for the routines they encounter, then extend the dependency index and JEV packet generation around actual investigations.

Use three upcoming gameplay tasks to evaluate the tooling:

1. Tutorial exit and guide removal.
2. Player movement and cancellation.
3. Player attacks.

Measure whether setup, reproduction, and mismatch diagnosis become faster. Keep the initial dictionary focused on these tasks rather than attempting to describe every field in the entire executable before continuing gameplay implementation.

## Effort and expected benefit

The proposed initial tooling investment is roughly **12–20 hours**. If it reduces repeated setup and diagnosis as expected, **10–30 hours of net savings** over the remaining work seems plausible after that investment.

These are low-confidence planning estimates, not measured results. The benefits overlap with the existing function-map savings and should not simply be added to them. Building tools does not itself satisfy the gameplay acceptance checklist.
