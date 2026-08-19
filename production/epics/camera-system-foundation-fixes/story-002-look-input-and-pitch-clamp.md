# Story 002: Look Input and Pitch Clamp

> **Epic**: Camera System Foundation Fixes
> **Status**: Ready
> **Layer**: Core
> **Type**: Logic
> **Estimate**: 2-4 hours
> **Manifest Version**: 2026-07-27
> **Last Updated**: Not started

## Context

**GDD**: `design/gdd/camera-system-base.md`
**Requirement**: `TR-cam-002`

**ADR Governing Implementation**: ADR-0005: Camera System SpringArm
**ADR Decision Summary**: Enhanced Input routes `IA_Look` to controller yaw/pitch, while a new `AMoonPlayerCameraManager` owns the data-driven view-pitch clamp.

**Engine**: Unreal Engine 5.8 | **Risk**: LOW
**Engine Notes**: Confirm the local PlayerController/GameMode assignment path and `APlayerCameraManager` pitch fields against installed UE5.8 headers before wiring the subclass.

**Control Manifest Rules (this layer)**:
- Required: pitch clamp ownership belongs to `AMoonPlayerCameraManager`.
- Forbidden: ad hoc pitch clamping in CMC or SpringArm code.
- Guardrail: continue using Enhanced Input; do not add legacy axis bindings.

---

## Acceptance Criteria

- [ ] `IA_Look` remains an Axis2D Enhanced Input action whose X/Y values route to controller yaw/pitch input.
- [ ] `AMoonPlayerCameraManager` is assigned for the player and reads `CameraPitchMin=-60` and `CameraPitchMax=30` from `UMoonCameraSettings`.
- [ ] Repeated maximum look input never moves view pitch outside `[-60,+30]` degrees.
- [ ] Pitch-clamp ownership is absent from CharacterMovement and SpringArm update code.

---

## Implementation Notes

- Implement `AMoonPlayerCameraManager : APlayerCameraManager` and apply `ViewPitchMin/ViewPitchMax` from the same settings asset used by the character.
- Wire the manager through the actual project PlayerController/GameMode path; do not rely on an editor-only manual selection that tests cannot reproduce.
- Retain the current `IA_Look` Enhanced Input route in `AMoonCharacterBase` unless a minimal PlayerController extraction is required to make manager assignment explicit.

---

## Out of Scope

- Story 003 verifies camera-relative movement and facing.
- Story 007 owns temporary execution look suppression.
- Story 009 owns camera shake behavior on the manager.

---

## QA Test Cases

- **AC-1**: Enhanced Input routing
  - Given: keyboard/mouse and gamepad input mappings for `IA_Look`.
  - When: positive and negative X/Y values are injected.
  - Then: X changes controller yaw and Y changes controller pitch through Enhanced Input.
  - Edge cases: zero input and simultaneous two-axis input.
- **AC-2/3**: Pitch boundaries
  - Given: a player using `AMoonPlayerCameraManager` with default camera settings.
  - When: look input is held past both vertical limits.
  - Then: pitch stops at `-60` and `+30` degrees without overshoot.
  - Edge cases: large single-frame input and settings values at their safe-range boundaries.
- **AC-4**: Ownership
  - Given: camera and movement source files.
  - When: static checks search for pitch-clamp logic.
  - Then: the clamp is owned by `AMoonPlayerCameraManager`, not CMC or SpringArm update code.
  - Edge cases: comments do not satisfy the ownership check.

---

## Test Evidence

**Story Type**: Logic
**Required evidence**:
- `tests/unit/camera/look_input_and_pitch_clamp_test.*`
- `tests/static/camera/pitch_clamp_ownership_check.ps1`

**Status**: [ ] Not yet created

---

## Dependencies

- Depends on: Story 001
- Unlocks: Story 009
