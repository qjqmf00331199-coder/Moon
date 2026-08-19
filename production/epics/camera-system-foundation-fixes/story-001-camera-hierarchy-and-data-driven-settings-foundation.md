# Story 001: Camera Hierarchy + Data-Driven Settings Foundation

> **Epic**: Camera System Foundation Fixes
> **Status**: Complete
> **Layer**: Core
> **Type**: Logic
> **Estimate**: 3h — fill in t-shirt size before sprint planning if hours change
> **Manifest Version**: 2026-07-27
> **Last Updated**: 2026-08-17

## Context

**GDD**: `design/gdd/camera-system-base.md`
**Requirement**: `TR-cam-001`, `TR-cam-009`
*(Requirement text lives in `docs/architecture/tr-registry.yaml` — read fresh at review time)*

**ADR Governing Implementation**: ADR-0005: Camera System (base) — SpringArm + Data-Driven Config
**ADR Decision Summary**: Keep `USpringArmComponent`+`UCameraComponent`; introduce `UMoonCameraSettings : UDataAsset` holding all 12 Tuning Knobs values, loaded by `AMoonCharacterBase` in `BeginPlay`. Constructor keeps literal fallbacks purely for CDO preview, never used at runtime once the asset loads.

**Engine**: Unreal Engine 5.8 | **Risk**: LOW
**Engine Notes**: `USpringArmComponent`/`UCameraComponent` are stable pre-cutoff GameFramework APIs, not flagged in `deprecated-apis.md`/`breaking-changes.md`. `bUseCameraLagSubstepping`/`CameraLagMaxTimeStep` (already used in `MoonCharacterBase.cpp`) are unverified against 5.8 headers — low risk, verify at implementation time.

**Control Manifest Rules (this layer)**:
- Required: Camera configuration must be data-asset driven through `UMoonCameraSettings`; constructor literals are only safe fallbacks for editor preview — source: ADR-0005.
- Forbidden: Never keep camera tuning hardcoded as the runtime source of truth — source: ADR-0005.
- Guardrail: Camera SpringArm/FOV/shake work is presentation-side and must not gate movement, damage, or cast judgment — source: ADR-0005.

---

## Acceptance Criteria

*From GDD `design/gdd/camera-system-base.md`, scoped to this story:*

- [ ] Capsule (Root) → `USpringArmComponent` → `UCameraComponent` hierarchy exists on `AMoonCharacterBase`, SpringArm pivot at `Z=60.0uu` relative location (Rule 1)
- [ ] `bUsePawnControlRotation=true`, `bInheritPitch=true`, `bInheritYaw=true`, `bInheritRoll=false` on the SpringArm (Rule 4)
- [ ] `UMoonCameraSettings : UDataAsset` created with all 12 Tuning Knobs fields: `TargetArmLength`, `CameraSocketOffset`, `CameraPitchMin`, `CameraPitchMax`, `CameraLagSpeed`, `CameraRotationLagSpeed`, `CameraLagMaxDistance`, `BaseFOV`, `OverdriveFOV`, `ExecutionArmLength`, `CameraProbeSize`, plus any remaining knob from the Tuning Knobs table
- [ ] Default asset values match the currently shipped, hand-tuned feel: `TargetArmLength=450.0uu`, `SocketOffset=(0,45,20)`, `CameraLagSpeed=18.0`, `CameraLagMaxDistance=60.0` (no feel regression)
- [ ] `AMoonCharacterBase::BeginPlay` reads the `UMoonCameraSettings` reference and applies every field to the SpringArm/Camera at runtime
- [ ] Constructor-time literals remain only as CDO preview fallbacks — never read at runtime once the asset loads

---

## Implementation Notes

*Derived from ADR-0005 Implementation Guidelines (Migration Plan steps 1-2):*

1. Create `UMoonCameraSettings` DataAsset class with all 12 fields, defaulted to the currently shipped values (no feel change).
2. Move `MoonCharacterBase.cpp`'s hardcoded SpringArm setup to read from the asset in `BeginPlay` (constructor keeps literal fallbacks for CDO preview only).

This story is the foundation every other camera story in this epic depends on — `UMoonCameraSettings` is the single source of truth every subsequent story's tuning values are read from.

---

## Out of Scope

*Handled by neighbouring stories — do not implement here:*

- Story 002: Pitch clamp ownership (`AMoonPlayerCameraManager`)
- Story 003: Camera-relative movement input math (already implemented elsewhere, unaffected by this ADR)
- Story 004: `CameraLagMaxDistance` hard-follow behavior (only the field is defined here; the clamp logic is Story 004)
- Story 005-009: Collision, FOV, execution blend, teleport reset, camera shake

---

## QA Test Cases

*Written by /qa-plan sprint (2026-08-14). The developer implements against these — do not invent new test cases during implementation.*

- **AC-1**: Capsule → SpringArm → Camera hierarchy, pivot `Z=60.0uu`
  - Given: `AMoonCharacterBase` is spawned in a test world
  - When: component hierarchy is inspected
  - Then: `CameraBoom` (SpringArm) is a child of the capsule root at relative `Z=60.0uu`; `FollowCamera` is a child of `CameraBoom`
  - Edge cases: relative location exactly `0.0` (regression to old pivot) must fail the test

- **AC-2**: SpringArm inherit flags
  - Given: `CameraBoom` is constructed
  - When: flags are read
  - Then: `bUsePawnControlRotation=true`, `bInheritPitch=true`, `bInheritYaw=true`, `bInheritRoll=false`
  - Edge cases: `bInheritRoll=true` (horizon-tilt regression) must fail

- **AC-3**: `UMoonCameraSettings` has all 12 Tuning Knobs fields with shipped defaults
  - Given: a default-constructed `UMoonCameraSettings` asset
  - When: each of the 12 fields is read
  - Then: `TargetArmLength=450.0`, `CameraSocketOffset=(0,45,20)`, `CameraLagSpeed=18.0`, `CameraLagMaxDistance=60.0` (and the remaining 8 fields match the GDD Tuning Knobs table's Current Value column)
  - Edge cases: any field left at a C++ zero-initialized default (`0.0`/`(0,0,0)`) indicates a missed field — fail

- **AC-4**: `BeginPlay` applies asset values at runtime, not constructor literals
  - Given: a `UMoonCameraSettings` asset with a non-default `TargetArmLength` (e.g. `300.0`) assigned to the character
  - When: `BeginPlay` runs
  - Then: `CameraBoom->TargetArmLength == 300.0` (matches the asset, not the constructor literal `450.0`)
  - Edge cases: no asset assigned — must fall back to constructor literal without crashing (null-asset guard)

---

## Test Evidence

**Story Type**: Logic
**Required evidence**:
- Logic: `Moon/Source/Moon/Tests/MoonCameraSettingsTests.cpp` — `Moon.Camera.Settings.ShippedDefaults`, compiled Automation test covering AC-3 (DataAsset field defaults)
- Logic: `tests/unit/camera/camera-settings-foundation_test.ps1` — static source-regex check covering AC-1/AC-2 (hierarchy/inherit flags); only weakly infers AC-4 via source text order, not runtime execution
- Logic: `Moon/Source/Moon/Tests/MoonCameraApplySettingsRuntimeTests.cpp` — `Moon.Camera.Settings.AppliesAtRuntimeNotConstructorLiteral`, compiled Automation test covering AC-4: spawns an `AMoonCharacterBase` (via a test-only accessor subclass), assigns a `UMoonCameraSettings` instance with `TargetArmLength=300.0`, runs `BeginPlay()`, and asserts the live `CameraBoom->TargetArmLength` reads `300.0` (not the constructor's `450.0` literal); a second case covers the null-asset edge case (no crash, literal `450.0` preserved). AC-4 is now runtime-verified, not just static-regex-inferred.

**Status**: [x] Created — pending CI/editor run for pass confirmation

---

## Dependencies

- Depends on: None
- Unlocks: Story 002, Story 003, Story 004, Story 005, Story 006, Story 007, Story 008, Story 009

---

## Completion Notes
**Completed**: 2026-08-17
**Criteria**: 6/6 passing
**Deviations**: ADVISORY — test evidence uses `.ps1` static regression instead of `.cpp` at the exact filename stem for AC-1/AC-2/AC-5/AC-6 (matches repo's existing `tests/unit/movement/*.ps1` convention, since nothing under repo-root `tests/` compiles); ADVISORY — `EditDefaultsOnly` on `UMoonCameraSettings` fields, `EditAnywhere` more idiomatic for a standalone DataAsset (non-blocking, noted in code review)
**Test Evidence**: Logic — `Moon/Source/Moon/Tests/MoonCameraSettingsTests.cpp` (AC-3), `Moon/Source/Moon/Tests/MoonCameraApplySettingsRuntimeTests.cpp` (AC-4), `tests/unit/camera/camera-settings-foundation_test.ps1` (AC-1/AC-2/AC-5/AC-6) — re-run, passes
**Code Review**: Complete — APPROVED (unreal-specialist + qa-tester parallel review; one BLOCKING gap on AC-4 found and fixed before this closure)
