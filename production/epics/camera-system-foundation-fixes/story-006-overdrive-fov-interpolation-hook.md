# Story 006: Overdrive FOV Interpolation Hook

> **Epic**: Camera System Foundation Fixes
> **Status**: Ready
> **Layer**: Core
> **Type**: Logic
> **Estimate**: 2h
> **Manifest Version**: 2026-07-27
> **Last Updated**: (set by /dev-story when implementation begins)

## Context

**GDD**: `design/gdd/camera-system-base.md`
**Requirement**: `TR-cam-006` (FOV half — see Story 007 for the execution-blend half)
*(Requirement text lives in `docs/architecture/tr-registry.yaml` — read fresh at review time)*

**ADR Governing Implementation**: ADR-0005: Camera System (base) — SpringArm + Data-Driven Config
**ADR Decision Summary**: FOV interpolation (Rule 7) lives as `Tick`-driven `FInterpTo` state on `AMoonCharacterBase`, consistent with the existing one-shot-anim/jump-state-machine Tick pattern already established in that file. Character exposes `SetOverdriveFOVActive(bool)`, wired to Luna Overdrive's `OnOverdriveStarted`/`OnOverdriveEnded` events.

**Engine**: Unreal Engine 5.8 | **Risk**: LOW
**Engine Notes**: `FMath::FInterpTo` is a stable pre-cutoff utility.

**Control Manifest Rules (this layer)**:
- Guardrail: Camera SpringArm/FOV/shake work is presentation-side and must not gate movement, damage, or cast judgment — source: ADR-0005.

---

## Acceptance Criteria

*From GDD `design/gdd/camera-system-base.md`, scoped to this story (QA-TEST-08):*

- [ ] `AMoonCharacterBase::SetOverdriveFOVActive(bool bActive)` public method exists, callable by Luna Overdrive's `OnOverdriveStarted`/`OnOverdriveEnded`
- [ ] On activation: FOV interpolates from `BaseFOV` (`90.0°`) to `OverdriveFOV` (`100.0°`) via `FMath::FInterpTo` with `InterpSpeed=6.0`, reaching target in `0.5s ± 0.05s`
- [ ] On deactivation: FOV interpolates back to `90.0°` with `InterpSpeed=4.0`, reaching target in `0.8s ± 0.08s`
- [ ] FOV values are sourced from `UMoonCameraSettings` (`BaseFOV`, `OverdriveFOV`), not hardcoded literals

---

## Implementation Notes

*Derived from ADR-0005 Decision 4 and GDD Formula 2:*

FOV interpolation runs on `AMoonCharacterBase::Tick`, matching the existing Tick-driven pattern in that class (not a new animation/blend framework). Luna Overdrive is not yet an epic in this project — `SetOverdriveFOVActive` is the forward interface; wiring the actual call site into Luna Overdrive's start/end events happens whenever that system's implementation lands. Until then, this story is verifiable by calling `SetOverdriveFOVActive` directly in a test harness or via a debug console command.

---

## Out of Scope

*Handled by neighbouring stories — do not implement here:*

- Story 007: Execution cutscene camera blend (different `Tick`-driven interp, different trigger)
- Wiring the actual Luna Overdrive `OnOverdriveStarted`/`OnOverdriveEnded` call sites (Luna Overdrive epic, not yet created)

---

## QA Test Cases

*Written by /qa-plan sprint (2026-08-14). The developer implements against these — do not invent new test cases during implementation.*

- **AC-1**: `SetOverdriveFOVActive` public interface exists
  - Given: `AMoonCharacterBase` instance
  - When: `SetOverdriveFOVActive(true)` is called
  - Then: the character's internal FOV-target state switches to `OverdriveFOV`
  - Edge cases: calling `SetOverdriveFOVActive(true)` twice in a row must not restart the interpolation from `BaseFOV` (idempotent while already active)

- **AC-2 (QA-TEST-08, activation)**: FOV interpolates 90→100 in 0.5s
  - Given: `SetOverdriveFOVActive(true)` is called at `FOV=90.0`
  - When: FOV is sampled every tick via `FMath::FInterpTo` at `InterpSpeed=6.0`
  - Then: FOV reaches `100.0 ± tolerance` at `t=0.5s ± 0.05s`
  - Edge cases: activation called mid-deactivation-interpolation (rapid toggle) must not produce a discontinuous jump

- **AC-3 (QA-TEST-08, deactivation)**: FOV interpolates 100→90 in 0.8s
  - Given: `SetOverdriveFOVActive(false)` is called at `FOV=100.0`
  - When: FOV is sampled every tick via `FMath::FInterpTo` at `InterpSpeed=4.0`
  - Then: FOV reaches `90.0 ± tolerance` at `t=0.8s ± 0.08s`
  - Edge cases: none beyond the activation-toggle case above

- **AC-4**: values sourced from data asset
  - Given: `UMoonCameraSettings` with non-default `OverdriveFOV=105.0`
  - When: overdrive activates
  - Then: FOV target is `105.0`, not the GDD default `100.0`

---

## Test Evidence

**Story Type**: Logic
**Required evidence**:
- Logic: `tests/unit/camera/overdrive-fov_test.cpp` — must exist and pass

**Status**: [ ] Not yet created

---

## Dependencies

- Depends on: Story 001 (`UMoonCameraSettings.BaseFOV`/`OverdriveFOV`)
- Unlocks: Luna Overdrive epic's future `OnOverdriveStarted`/`OnOverdriveEnded` wiring
