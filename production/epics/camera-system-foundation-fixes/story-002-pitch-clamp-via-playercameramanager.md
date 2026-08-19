# Story 002: Pitch Clamp via PlayerCameraManager

> **Epic**: Camera System Foundation Fixes
> **Status**: Complete
> **Layer**: Core
> **Type**: Logic
> **Estimate**: 2h
> **Manifest Version**: 2026-07-27
> **Last Updated**: 2026-08-18

## Context

**GDD**: `design/gdd/camera-system-base.md`
**Requirement**: `TR-cam-002`
*(Requirement text lives in `docs/architecture/tr-registry.yaml` — read fresh at review time)*

**ADR Governing Implementation**: ADR-0005: Camera System (base) — SpringArm + Data-Driven Config
**ADR Decision Summary**: New `AMoonPlayerCameraManager : APlayerCameraManager` subclass owns the pitch clamp via a `ViewPitchMin`/`ViewPitchMax` override sourced from `UMoonCameraSettings` — matches the GDD's explicit assignment of this ownership to a PlayerCameraManager subclass rather than CMC or the SpringArm.

**Engine**: Unreal Engine 5.8 | **Risk**: LOW
**Engine Notes**: `APlayerCameraManager` is a stable pre-cutoff GameFramework API.

**Control Manifest Rules (this layer)**:
- Required: Camera pitch clamp belongs to `AMoonPlayerCameraManager`, not CMC or SpringArm ad hoc code — source: ADR-0005.
- Forbidden: (see Alternative 3 rejection in ADR-0005 — do not clamp pitch in `CharacterMovementComponent`)

---

## Acceptance Criteria

*From GDD `design/gdd/camera-system-base.md`, scoped to this story (QA-TEST-04):*

- [x] `IA_Look` (Axis2D) Yaw routes to `APlayerController::AddControllerYawInput` (Rule 2)
- [x] `IA_Look` Pitch routes to `APlayerController::AddControllerPitchInput` (Rule 2)
- [x] `AMoonPlayerCameraManager` clamps view pitch to `[CameraPitchMin, CameraPitchMax]` sourced from `UMoonCameraSettings` (default `-60.0`/`30.0`) — Formula 4
- [x] Camera never exceeds `30.0°` looking down or `-60.0°` looking up regardless of continuous max input; clamp holds at the boundary rather than oscillating

---

## Implementation Notes

*Derived from ADR-0005 Implementation Guidelines (Decision 3, Migration Plan step 3):*

Implement `AMoonPlayerCameraManager`, wire the pitch clamp from `UMoonCameraSettings`, and assign it as the player's camera manager class. This class is also where Story 009's camera shake dispatch will live — keep the class open for that addition rather than sealing it as pitch-only.

---

## Out of Scope

*Handled by neighbouring stories — do not implement here:*

- Story 001: `UMoonCameraSettings` DataAsset itself (this story only reads `CameraPitchMin`/`CameraPitchMax` from it)
- Story 009: Camera shake dispatch (same class, different story)

---

## QA Test Cases

*Written by /qa-plan sprint (2026-08-14). The developer implements against these — do not invent new test cases during implementation.*

- **AC-1**: `IA_Look` axes route to controller Yaw/Pitch
  - Given: player controller possesses the pawn, `IA_Look` is bound
  - When: `IA_Look.X` and `IA_Look.Y` receive non-zero input
  - Then: `AddControllerYawInput` is called with the X value, `AddControllerPitchInput` with the Y value
  - Edge cases: zero input must not call either function (no-op tick)

- **AC-2 (QA-TEST-04)**: pitch clamp holds at boundary
  - Given: `AMoonPlayerCameraManager` is active with `CameraPitchMin=-60.0`, `CameraPitchMax=30.0` from `UMoonCameraSettings`
  - When: continuous max-magnitude pitch input is applied for several seconds in each direction
  - Then: `ViewPitchMin`/`ViewPitchMax` clamp the resulting view rotation to exactly `[-60.0, 30.0]`, never exceeding either bound
  - Edge cases: input applied for a single very large delta (fast mouse flick) must not overshoot the clamp even for one frame

- **AC-3**: pitch clamp values sourced from data asset, not hardcoded
  - Given: a `UMoonCameraSettings` asset with non-default `CameraPitchMin=-45.0`
  - When: `AMoonPlayerCameraManager` reads its clamp bounds
  - Then: the effective clamp is `-45.0`, not the GDD default `-60.0`
  - Edge cases: no asset assigned — must fall back to a safe default without crashing

---

## Test Evidence

**Story Type**: Logic
**Required evidence**:
- Logic: `tests/unit/camera/pitch-clamp_test.cpp` — must exist and pass

**Status**: [x] COVERED (MoonPlayerCameraManagerTests.cpp)

---

## Dependencies

- Depends on: Story 001 (`UMoonCameraSettings` must exist)
- Unlocks: Story 009 (shares `AMoonPlayerCameraManager`)
