# Story 001: Camera Settings and Component Hierarchy

> **Epic**: Camera System Foundation Fixes
> **Status**: Ready
> **Layer**: Core
> **Type**: Config/Data
> **Estimate**: 2-4 hours
> **Manifest Version**: 2026-07-27
> **Last Updated**: Not started

## Context

**GDD**: `design/gdd/camera-system-base.md`
**Requirement**: `TR-cam-001`, `TR-cam-009`

**ADR Governing Implementation**: ADR-0005: Camera System SpringArm
**ADR Decision Summary**: Keep the character-owned `USpringArmComponent -> UCameraComponent` hierarchy and make `UMoonCameraSettings` the runtime source of truth for every camera tuning knob. Constructor literals remain editor-preview fallbacks only.

**Engine**: Unreal Engine 5.8 | **Risk**: LOW
**Engine Notes**: Verify `bUseCameraLagSubstepping` and `CameraLagMaxTimeStep` against the installed UE5.8 headers during implementation; the ADR marks these properties as not yet formally verified.

**Control Manifest Rules (this layer)**:
- Required: camera configuration is data-asset driven through `UMoonCameraSettings`.
- Forbidden: constructor literals as the runtime source of truth.
- Guardrail: camera presentation must not gate movement or gameplay judgment.

---

## Acceptance Criteria

*From GDD `design/gdd/camera-system-base.md`, scoped to this story:*

- [ ] The player camera hierarchy is Capsule Root -> `USpringArmComponent` -> `UCameraComponent`, with the boom pivot at torso height (`Z = 60.0uu`).
- [ ] A `UMoonCameraSettings` data asset exposes all camera tuning knobs named by the GDD, including arm length, socket offset, pitch bounds, lag values, FOV values, execution arm length, and probe size.
- [ ] At runtime, the character applies the assigned settings asset in `BeginPlay`; the current tuned defaults (`TargetArmLength=450`, `SocketOffset=(0,45,20)`, `CameraLagSpeed=18`, `CameraLagMaxDistance=60`) are preserved.
- [ ] A missing or invalid settings asset produces an explicit diagnostic and uses documented safe fallbacks without crashing.

---

## Implementation Notes

- Add `UMoonCameraSettings : UDataAsset` in the Camera domain and give every exposed value the GDD safe-range metadata where Unreal property metadata can express it.
- Keep `CameraBoom` and `FollowCamera` owned by `AMoonCharacterBase`; do not migrate to GameplayCameras.
- Apply the data asset once during runtime initialization. Constructor values may support CDO/editor preview but must not silently override a valid asset.
- Preserve existing tuned values; do not reset the shoulder view to Formula 5's stale historical `350uu/(0,0,0)` recovery values. GDD QA-TEST-09 and ADR-0005 establish `450uu/(0,45,20)` as the runtime base view.

---

## Out of Scope

- Story 002 owns look routing and pitch clamp.
- Story 004 owns lag-limit behavior.
- Stories 006 and 007 own dynamic FOV/execution blends.

---

## QA Test Cases

- **AC-1**: Component hierarchy
  - Given: the `AMoonCharacterBase` class default object or a spawned player.
  - When: camera component attachments and boom relative location are inspected.
  - Then: Capsule Root owns the boom, the camera attaches to the boom socket, and the pivot is at `Z=60.0uu`.
  - Edge cases: Blueprint subclasses must not detach or replace the required hierarchy.
- **AC-2/3**: Data-driven defaults
  - Given: an assigned `UMoonCameraSettings` asset with non-fallback test values.
  - When: the player begins play.
  - Then: runtime camera properties match the asset and the production asset preserves the approved default shoulder view.
  - Edge cases: every GDD tuning field is present; values at safe-range boundaries load correctly.
- **AC-4**: Missing asset fallback
  - Given: no camera settings asset is assigned.
  - When: the player begins play.
  - Then: the camera remains usable with safe defaults and emits one actionable diagnostic.
  - Edge cases: no per-frame warning spam and no null dereference.

---

## Test Evidence

**Story Type**: Config/Data
**Required evidence**:
- `tests/static/camera/camera_settings_contract_check.ps1`
- `production/qa/evidence/camera-settings-and-component-hierarchy-evidence.md`

**Status**: [ ] Not yet created

---

## Dependencies

- Depends on: Player Movement Story 001 (Complete)
- Unlocks: Stories 002, 004, 005, 006, 007, 008, 009
