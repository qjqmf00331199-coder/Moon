# Story 005 Evidence: Movement Traceability and Static Regression Checks

> **Epic**: Player Movement Foundation Fixes
> **Story**: 005 — Movement Traceability and Static Regression Checks
> **Requirements**: TR-mov-002, TR-mov-009, TR-mov-010
> **ADR**: ADR-0009 (Decisions 1, 6, 7)
> **Date**: 2026-07-27

---

## Summary

Added the two Insights trace scopes ADR-0009 Decision 6 specifies to `AMoonCharacterBase`, and
consolidated static regression coverage for TR-mov-002/TR-mov-010 into one runnable suite that
delegates to existing scripts where they already cover the ground, rather than duplicating them.

## Files Changed

| File | Change |
|---|---|
| `Moon/Source/Moon/Character/MoonCharacterBase.cpp` | Added `#include "ProfilingDebugging/CpuProfilerTrace.h"`; added `TRACE_CPUPROFILER_EVENT_SCOPE(MovementInputTrace.InputTriggered)` at the top of `Move()`; added `TRACE_CPUPROFILER_EVENT_SCOPE(MovementInputTrace.VelocityUpdated)` in `Tick()` immediately after `Super::Tick(DeltaTime)`, wrapped narrowly around the existing Story 003 airborne-substate derivation. No other lines touched — Story 003/004 logic is byte-for-byte unchanged except being nested one brace level deeper. |

## Files Created

| File | Purpose |
|---|---|
| `tests/static/movement_regression_checks.ps1` | Consolidated TR-mov-002/TR-mov-010 static regression suite (this story's named path). Delegates to `movement_foundation_contract.ps1` for Spell Casting independence; adds new whole-file Time Dilation, montage-input-lock, and source-level root-motion checks; includes an AC-3 self-test that proves each detector actually fires on a known-bad fixture string before trusting a "clean" verdict on real source. |
| `tests/integration/movement/movement_traceability_test.ps1` | Source-level static verification (this story's named path) that both `MovementInputTrace` scopes exist with the exact required names and sit at the ADR-mandated positions (before the `bMovementLocked` gate in `Move()`; after `Super::Tick()` in `Tick()`). Explicitly documents that this is NOT a live Insights capture. |
| `production/qa/evidence/movement-traceability-and-static-regression-checks-evidence.md` | This file. |

## Trace Scope Implementation Detail

### Macro syntax verification (engine-header-verified, not assumed)

This session had direct access to the project's actual UE 5.8 install
(`C:\Program Files\Epic Games\UE_5.8`), so the macro syntax below was verified against the real
header rather than trained-data recall — flagged per this project's Engine Version Safety
requirement since UE5.8 postdates the LLM knowledge cutoff.

Read: `Engine/Source/Runtime/Core/Public/ProfilingDebugging/CpuProfilerTrace.h`.

Findings:
- `TRACE_CPUPROFILER_EVENT_SCOPE(Name)` expands (via `TRACE_CPUPROFILER_EVENT_SCOPE_CONDITIONAL` →
  `TRACE_CPUPROFILER_EVENT_SCOPE_STR_CONDITIONAL(#Name, ...)`) to a `#Name` **stringification** of
  its raw token argument. It is documented in the header itself: *"Do not use this macro with a
  static string because, in that case, additional quotes will be added around the event scope
  name."* — i.e. the macro wants a bare token, not a string literal.
- A dotted name like `MovementInputTrace.InputTriggered` is a valid bare-token argument: the
  preprocessor lexes it as three pp-tokens (`MovementInputTrace`, `.`, `InputTriggered`) with no
  intervening whitespace in source, so `#Name` stringification produces exactly
  `"MovementInputTrace.InputTriggered"` with no added spaces or quotes.
- This is not merely a derived reading of the grammar — Epic's own shipped engine source uses this
  exact bare-dotted-token pattern: `Engine/Source/Editor/UnrealEd/Private/Commandlets/DumpMaterialShaderTypes.cpp:1093`
  contains `TRACE_CPUPROFILER_EVENT_SCOPE(UDumpMaterialShaderTypesCommandlet.AssetRegistryScan);`.

**Conclusion**: the bare macro `TRACE_CPUPROFILER_EVENT_SCOPE(MovementInputTrace.InputTriggered)` /
`TRACE_CPUPROFILER_EVENT_SCOPE(MovementInputTrace.VelocityUpdated)` is correct and was used exactly
as written. The string-literal variant (`TRACE_CPUPROFILER_EVENT_SCOPE_STR("...")`) was **not**
needed and was **not** used.

### Documentation-vs-code discrepancy to flag (not fixed — outside this story's write boundary)

ADR-0009's "Architecture Diagram" and "Key Interfaces" sections both write the trace macro
invocation with the name **quoted**, e.g. `TRACE_CPUPROFILER_EVENT_SCOPE("MovementInputTrace.InputTriggered")`.
Per the header's own documented warning above, that literal form — if actually compiled — would
double-stringify and emit `"\"MovementInputTrace.InputTriggered\""` (embedded escaped quotes) as
the trace name, not the clean name the ADR intends. Read in context, the ADR's quoted form is almost
certainly documentation shorthand ("the trace scope named X") rather than literal C++ to
copy-paste — but it is worth a follow-up correction to the ADR text so a future reader doesn't
copy it verbatim into code. **This file (`MoonCharacterBase.cpp`) uses the correct bare-token form
regardless of the ADR's prose.** No ADR files were modified by this story (out of this agent's
write boundary — architecture docs require technical-director approval).

### Placement rationale

- **`MovementInputTrace.InputTriggered`** — placed as the very first statement in `Move()`, before
  the `bMovementLocked` early-return (Story 003). This measures Enhanced Input's `Triggered`
  delegate dispatch itself, so it must fire unconditionally regardless of whether the input is
  subsequently gated — exactly as the story's own implementation notes specify.
- **`MovementInputTrace.VelocityUpdated`** — placed immediately after `Super::Tick(DeltaTime)`,
  wrapped in a narrow brace scope around the existing Story 003 airborne-substate read (not around
  the rest of `Tick()`). Rationale: `Tick()` continues on to do unrelated per-frame work after the
  substate check (mana regen, dash-charge regen, tension decay, Overdrive state, jump-feel gravity,
  jump/locomotion animation swap, hitstop presentation) — none of that is movement-tick cost, and
  folding it into this scope's measured duration would produce a misleading number for the AC-4
  benchmark this scope is meant to eventually support (`tests/static` review caught this; see
  Deferred Items below for why the narrow scope matters to that specific future benchmark). The
  scope's **begin timestamp** — which is what actually matters for the p95-input-to-velocity
  latency and time-to-95%-speed Feel ACs — is identical whether the scope is narrow or wraps the
  rest of `Tick()`, since it starts at the same point either way; the narrow scope was chosen so the
  scope's **duration** also means something coherent (a movement-tick-adjacent measurement) rather
  than "everything this actor's Tick() happens to do this frame."
- Neither Story 003's airborne-substate logic nor Story 004's hitstop logic was reordered or
  otherwise modified — the airborne-substate code is nested one additional brace level (the new
  trace scope's `{ }`), and is otherwise byte-identical.

## Test Results

All commands run via `powershell -NoProfile -ExecutionPolicy Bypass -File <path>` from the repo root.

| Script | Result |
|---|---|
| `tests/static/movement_regression_checks.ps1` (new) | **PASS** — `movement regression static checks passed (Spell Casting independence delegated + verified; whole-file Time Dilation, montage input-lock, and source-level root-motion checks passed with self-tested detectors)` |
| `tests/integration/movement/movement_traceability_test.ps1` (new) | **PASS** — `movement traceability source-level static checks passed (AC-1/AC-2: both MovementInputTrace scopes exist with exact names, at ADR-0009-mandated positions — NOT a live Insights capture, see evidence doc)` |
| `tests/unit/movement/camera_yaw_facing_test.ps1` (Story 001, re-run) | **PASS** — `camera yaw facing unit checks passed` |
| `tests/static/movement_foundation_contract.ps1` (Story 001, re-run) | **PASS** — `movement foundation contract static checks passed` |
| `tests/static/movement_independence_check.ps1` (Story 001, re-run) | **PASS** — `movement foundation contract static checks passed` |
| `tests/unit/movement/airborne_and_grace_windows_test.ps1` (Story 003, re-run) | **PASS** — `airborne substate / jump buffer / coyote time unit checks passed` |
| `tests/unit/movement/movement_lock_contract_test.ps1` (Story 003, re-run) | **PASS** — `movement lock contract unit checks passed` |
| `tests/static/hitstop_no_time_dilation_check.ps1` (Story 004, re-run) | **PASS** — `hitstop no-time-dilation static checks passed (negative: zero CustomTimeDilation/SetGlobalTimeDilation refs; positive: capture-and-blend mechanism verified)` |
| `tests/unit/movement/tuning_clamp_and_joint_bound_test.ps1` (Story 004, re-run) | **PASS** — `movement tuning clamp / AirTime joint bound / Z-impulse hook unit checks passed` |

**Zero regressions** from touching `MoonCharacterBase.cpp` a further time — all 7 pre-existing
Story 001/003/004 scripts still pass unmodified against the new file contents.

## What's Automated and Verified

- **AC-1**: `MovementInputTrace.InputTriggered` exists with the exact name, at the top of `Move()`,
  before the `bMovementLocked` gate — verified by source-level regex + ordering-index comparison
  (`movement_traceability_test.ps1`). NOT a live Insights capture (see Deferred below).
- **AC-2**: `MovementInputTrace.VelocityUpdated` exists with the exact name, in `Tick()`, after
  `Super::Tick(DeltaTime)` — verified the same way. NOT a live Insights capture.
- **AC-3** (static regression suite, TR-mov-002/TR-mov-010 forbidden-pattern list): genuinely
  closed, not just vacuously passing — `movement_regression_checks.ps1` includes a self-test that
  feeds each detector (Time Dilation, montage input-lock, root-motion) a known-bad fixture string
  and asserts it fires, before trusting the "clean" verdict against real source. Also includes a
  regression guard proving the montage detector does NOT false-positive on this codebase's own
  benign "montage/slot system" prose comment (case-insensitive `-match` hazard caught during
  implementation).
- **TR-mov-002** (Spell Casting independence): fully covered — this story's script delegates to
  the existing `movement_foundation_contract.ps1` (Build.cs dependency check + `Move()` body
  check) rather than duplicating it.
- Macro syntax (`TRACE_CPUPROFILER_EVENT_SCOPE` bare-token dotted-name usage): verified against
  the actual UE 5.8 engine header on this machine, not assumed from training data. See "Macro
  syntax verification" above.

## Explicitly Deferred (with reasons and pickup instructions)

### 1. AC-4 — N≥100 actor provisional movement-tick benchmark

**Partially closed 2026-08-14 — stability proven, exact timing numbers still missing.** A real
`unreal-mcp` server (`http://127.0.0.1:8000/mcp`, live in the running `UnrealEditor.exe` this
session) was reachable this time and driven directly via raw HTTP JSON-RPC (no wired Claude Code
tool for it existed, so this was done through `Bash`/`curl` against the MCP protocol directly):

1. `editor_toolset.toolsets.programmatic.ProgrammaticToolset.execute_tool_script` looped 100 calls
   to `editor_toolset.toolsets.scene.SceneTools.add_to_scene_from_asset`, spawning 100
   `/Game/Moon/BP_MoonCharacter` instances (the actual player movement class, not a lighter stand-in)
   in a 10×10 grid, `snap_to_ground: true`. Confirmed: `{"spawned": 100}`.
2. `EditorToolset.EditorAppToolset.StartPIE` (level `L_CombatTest`) — confirmed clean start via log:
   `PIE: Server logged in`, `Play in editor total start time 0.478 seconds`, and exactly 100
   `LogSkeletalMesh: Recreating Clothing Actors for 'CharacterMesh0' with 'Aurora'` lines (one per
   spawned actor, correct count).
3. `IsPIERunning` re-checked ~80 seconds later (far past the AC's 300-frame / ~5s window at any
   plausible framerate) — still `true`.
4. `EditorToolset.LogsToolset.GetLogEntries` filtered for `Error|Fatal|Assert|Crash` and separately
   for all recent entries — **zero errors or warnings from the spawn/PIE window itself**. The only
   `Error`-tagged lines in the whole log are pre-existing, unrelated noise (a `LogTemp` unified-error-
   handling self-test from an earlier engine session, and one earlier `LogModelContextProtocol` error
   from this same session calling a nested tool without the `call_tool` wrapper — a tool-usage mistake
   on my part, not an engine/gameplay error).

**What this proves**: 100 real player-movement-class actors can be spawned and sustain PIE for well
over 300 frames with zero crashes, errors, or warnings attributable to movement tick load — a real,
verified stability result, not a placeholder.

**What this still does NOT provide**: the AC's actual ask — recorded per-actor movement-tick timing
(mean/p95/max ms, an Insights-trace-based number). The `unreal-mcp` server's exposed toolsets
(`EditorToolset.EditorAppToolset`, `LogsToolset`, `SceneTools`, `ProgrammaticToolset`,
`AutomationTestToolset`, and others — full list captured via `list_toolsets`/`describe_toolset`) have
**no stat-overlay capture, no cvar-set, no console-command-execution, and no Insights/profiling tool**.
`SearchCVars` can only search cvar names, not set them or read `stat unit`'s rendered values. This is
a genuine capability gap in the bridge, not a skipped step — there is currently no remote way to
toggle `stat unit`/`stat game` or start an Insights trace and pull back the numbers. Getting the
actual ms figures still requires a human at the editor keyboard to run `stat unit` (or a proper
Insights capture) while this same 100-actor PIE session (or a fresh repeat) is running.

**Original recipe for whoever picks up the remaining numeric-evidence step:**

1. Spawn N≥100 lightweight movement-capable actors (`TargetDummy` or an equivalent minimal
   movement-test actor — does not need to be the full `AMoonCharacterBase` if a lighter stand-in
   exists, but should exercise the same `CharacterMovementComponent` tick path).
2. Enable Insights tracing (`-trace=cpu` or in-session `Trace.Start`) targeting the `CpuChannel`.
3. Capture 300 consecutive frames of `MovementInputTrace.VelocityUpdated` timing across all
   spawned actors.
4. Record the result (mean/p95/max per-actor cost, total frame-budget contribution) as baseline
   evidence in a new evidence file before the Production gate — this story's evidence doc is not
   the place for that data once it exists; link it in from here or from a dedicated benchmark
   evidence file.
5. Cross-check the recorded numbers against the control manifest's Performance Budgets
   (`Movement overhead — GDD target <0.1ms per actor, pending hardware confirmation`) and the
   60fps/16.6ms frame budget.

### 2. TR-mov-010 — actual `.uasset` root-motion import setting

**Partially closed only.** The source-level grep in `movement_regression_checks.ps1` (and the
pre-existing check in `camera_yaw_facing_test.ps1`) confirms `MoonCharacterBase.cpp` never sets
`bEnableRootMotion = true` or `RootMotionMode = ...` in code. It does **not** and **cannot** verify
the actual import-time `bEnableRootMotion` setting baked into the `JumpStartAnim`, `JumpApexAnim`,
and `JumpLandAnim` `AnimSequence` assets themselves — `.uasset` files are binary, and this session
has no Unreal Editor/MCP access to open and inspect asset import settings. Pickup checklist for a
future session with Editor access:

- [ ] Open each of `JumpStartAnim`, `JumpApexAnim`, `JumpLandAnim` (and any other locomotion
      `AnimSequence` referenced by `AMoonCharacterBase`) in the Editor.
- [ ] Confirm `Enable Root Motion` is unchecked (`bEnableRootMotion == false`) on each asset.
- [ ] If any asset has root motion enabled, either disable it or escalate to
      `ue-blueprint-specialist`/`unreal-specialist` for a decision on whether the asset needs
      re-import or re-authoring.
- [ ] Once confirmed, update this evidence doc (or a follow-up note) to record TR-mov-010 as fully
      closed, and consider adding this to a CI-runnable asset-validation commandlet if one exists
      or is worth adding (out of scope for this story to build).

### 3. Live Unreal Insights capture for AC-1/AC-2

**Not performed.** Both `movement_traceability_test.ps1` and this doc are explicit that the
automated evidence is source-level only — it proves the instrumentation exists and is positioned
correctly, not that a real Insights trace session actually emits and displays the two named
scopes with plausible p95-latency/time-to-95%-speed numbers. A human or future session with
Editor access should:

- [ ] Launch a Development build with tracing enabled (`-trace=cpu,default`).
- [ ] Open Unreal Insights, filter to the `CpuChannel`, and confirm both `MovementInputTrace.InputTriggered`
      and `MovementInputTrace.VelocityUpdated` events appear during normal play.
- [ ] Record a screenshot/timeline export as evidence per this project's Visual/Feel evidence
      convention, and cross-reference it from this doc.

## Engine-API Uncertainty

- **Resolved, not uncertain**: the `TRACE_CPUPROFILER_EVENT_SCOPE` bare-token dotted-name syntax
  was verified directly against the actual UE 5.8 header on this machine
  (`C:\Program Files\Epic Games\UE_5.8\Engine\Source\Runtime\Core\Public\ProfilingDebugging\CpuProfilerTrace.h`)
  plus a real usage precedent elsewhere in Epic's own engine source. No uncertainty remains on the
  macro syntax question the story flagged.
- **Still open** (carried over from ADR-0009's own "Verification Required" field, not newly
  introduced by this story): whether Enhanced Input's `Triggered` delegate firing point behaves
  exactly as assumed (i.e., that placing the trace at the top of the bound callback is equivalent
  to "the callback start" with no earlier engine-side dispatch overhead worth separately
  measuring) is still marked by the ADR as something to confirm once a live Insights capture is
  possible — this story's source-level check cannot resolve that, only the deferred live capture
  (Deferred Item 3) can.

## Out of Scope (per Story 005's own boundary, not touched by this implementation)

- Production hardware pass/fail threshold finalization.
- Feature-layer Combo/Tension tick ordering.
- Camera or HUD tracing.
- Dash, camera, or any file outside `MoonCharacterBase.h`/`.cpp` and the new test files.
