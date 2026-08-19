# Story 007: Execution Cutscene Camera Blend + Look-Input Suppression

> **Epic**: Camera System Foundation Fixes
> **Status**: Ready
> **Layer**: Core
> **Type**: Integration
> **Estimate**: 3h
> **Manifest Version**: 2026-07-27
> **Last Updated**: (set by /dev-story when implementation begins)

## Context

**GDD**: `design/gdd/camera-system-base.md`
**Requirement**: `TR-cam-006` (execution-blend half), `TR-cam-008` (look-suppression half)
*(Requirement text lives in `docs/architecture/tr-registry.yaml` — read fresh at review time)*

**ADR Governing Implementation**: ADR-0005: Camera System (base) — SpringArm + Data-Driven Config
**ADR Decision Summary**: Execution cutscene blend (Rule 8) lives as `Tick`-driven `FInterpTo`/`VInterpTo` state on `AMoonCharacterBase`. Character forward-declares `BeginExecutionCameraBlend()`/`EndExecutionCameraBlend()` for Core Extraction Execution to call once that GDD exists. This is a forward interface with no caller yet.

**Engine**: Unreal Engine 5.8 | **Risk**: LOW
**Engine Notes**: `FMath::FInterpTo`/`VInterpTo` are stable pre-cutoff utilities.

**Control Manifest Rules (this layer)**:
- Guardrail: Camera SpringArm/FOV/shake work is presentation-side and must not gate movement, damage, or cast judgment — source: ADR-0005.

**⚠️ No caller yet**: Core Extraction Execution does not have its own GDD/epic yet. This story implements `BeginExecutionCameraBlend()`/`EndExecutionCameraBlend()` as a speculative forward interface per the ADR's explicit instruction — verify manually (debug trigger) rather than via an end-to-end execution flow that doesn't exist yet. Health/Damage Core's `TryExecute`/`State.Executable` (TR-hp-005) already exists per that GDD and the signature-combat-chain-spike prototype — check whether it already calls into this interface before assuming it's fully speculative.

---

## Acceptance Criteria

*From GDD `design/gdd/camera-system-base.md`, scoped to this story (QA-TEST-09):*

- [ ] `AMoonCharacterBase::BeginExecutionCameraBlend()` blends SpringArm `TargetArmLength` to `ExecutionArmLength` (`150.0uu`) and `SocketOffset` to `(0,40,20)` within `0.2s`, using `FInterpTo`/`VInterpTo` at `InterpSpeed=15.0` (Formula 5)
- [ ] On execution entry, player look input (`IA_Look`) is ignored — no camera rotation drift from player input during the cutscene blend (Edge Case 5)
- [ ] `AMoonCharacterBase::EndExecutionCameraBlend()` blends back to the default `TargetArmLength` (`450.0uu`, sourced from `UMoonCameraSettings`) and `SocketOffset=(0,0,0)` within `0.3s` at `InterpSpeed=10.0`
- [ ] Look input is re-enabled only after the blend-back completes

---

## Implementation Notes

*Derived from ADR-0005 Decision 4, Migration Plan step 6, and GDD Formula 5 / Edge Case 5:*

Add the execution blend forward interface — no caller yet — as a stub until Core Extraction Execution exists. Do not build a full node-based cinematic system for this; it's a two-parameter lerp per the GDD's own framing, matching the same `Tick`-driven pattern as Story 006's FOV interpolation.

---

## Out of Scope

*Handled by neighbouring stories — do not implement here:*

- Story 006: Overdrive FOV interpolation (different trigger, different Tick state)
- Core Extraction Execution's actual trigger logic and animation montage (not yet designed as its own GDD)

---

## QA Test Cases

*Written by /qa-plan sprint (2026-08-14). The developer implements against these — do not invent new test cases during implementation.*

- **AC-1 (QA-TEST-09, part 1)**: entry blend timing/target
  - Given: `BeginExecutionCameraBlend()` is called
  - When: `TargetArmLength`/`SocketOffset` are sampled every tick via `FInterpTo`/`VInterpTo` at `InterpSpeed=15.0`
  - Then: `TargetArmLength` reaches `150.0uu` and `SocketOffset` reaches `(0,40,20)` within `0.2s`
  - Edge cases: `BeginExecutionCameraBlend()` called while a previous blend is still in-flight must not stack/compound the interpolation target

- **AC-2 (QA-TEST-09, part 2)**: look input suppressed during blend
  - Given: execution sequence has started (`BeginExecutionCameraBlend()` called)
  - When: player provides `IA_Look` input during the cutscene
  - Then: no camera rotation results from that input — look input is fully ignored
  - Edge cases: input buffered during suppression must not "replay" once suppression lifts (no queued rotation snap)

- **AC-3 (QA-TEST-09, part 3)**: exit blend timing/target + input restore
  - Given: `EndExecutionCameraBlend()` is called
  - When: `TargetArmLength`/`SocketOffset` are sampled every tick at `InterpSpeed=10.0`
  - Then: values return to default (`450.0uu` from `UMoonCameraSettings`, `SocketOffset=(0,0,0)`) within `0.3s`; look input re-enables only after the blend completes, not before
  - Edge cases: `EndExecutionCameraBlend()` called without a prior `BeginExecutionCameraBlend()` must not crash or apply a spurious blend (defensive no-op)

---

## Test Evidence

**Story Type**: Integration
**Required evidence**:
- Integration: `tests/integration/camera/execution-blend_test.cpp` OR documented playtest (manual debug-trigger acceptable given no caller exists yet)

**Status**: [ ] Not yet created

---

## Dependencies

- Depends on: Story 001 (`UMoonCameraSettings.ExecutionArmLength`, default `TargetArmLength`)
- Unlocks: Core Extraction Execution epic's future call sites (forward interface, not yet designed)
