# Story 003: Camera-Relative Movement Regression Contract

> **Epic**: Camera System Foundation Fixes
> **Status**: Ready
> **Layer**: Core
> **Type**: Integration
> **Estimate**: 2-4 hours
> **Manifest Version**: 2026-07-27
> **Last Updated**: Not started

## Context

**GDD**: `design/gdd/camera-system-base.md`
**Requirement**: `TR-cam-003`

**ADR Governing Implementation**: ADR-0005: Camera System SpringArm; ADR-0009: Player Movement Runtime Contract
**ADR Decision Summary**: Camera and Movement independently read controller rotation. Movement derives yaw-only forward/right vectors, clamps combined input magnitude, and keeps character facing snapped to controller yaw without creating a Camera-to-Movement execution-order dependency.

**Engine**: Unreal Engine 5.8 | **Risk**: LOW
**Engine Notes**: The behavior is already implemented and covered by the completed Player Movement epic; this story adds the Camera-epic integration contract and must not duplicate or fork movement ownership.

**Control Manifest Rules (this layer)**:
- Required: camera-relative movement and controller-yaw facing.
- Forbidden: camera code gating movement or directly owning movement state.
- Guardrail: preserve the existing movement trace/static checks.

---

## Acceptance Criteria

- [ ] At controller yaw `0`, `45`, `90`, and `180` degrees, full forward input moves within `±1.0` degree of the yaw-only camera forward vector.
- [ ] Full diagonal input is normalized/clamped to magnitude `1.0` and does not produce `sqrt(2)` speed.
- [ ] Rapid look rotation while idle or strafing updates character facing to controller yaw in the same tick with zero interpolation frames.
- [ ] Camera and Movement share only the PlayerController rotation contract; neither module requires the other's tick output to calculate its result.

---

## Implementation Notes

- Reuse the Player Movement Story 001 test harness and production code. Add only Camera-epic integration coverage that is missing.
- Keep `bUseControllerRotationYaw=true`, pitch/roll false, and `bOrientRotationToMovement=false`.
- Do not create a second movement-vector implementation in a camera class.

---

## Out of Scope

- Camera settings migration belongs to Story 001.
- Pitch bounds belong to Story 002.
- Dash direction behavior belongs to the Dash/Evasion epic.

---

## QA Test Cases

- **AC-1**: Yaw-relative forward movement
  - Given: a spawned player at controller yaw `0`, `45`, `90`, and `180` degrees.
  - When: full forward input is applied for one tick.
  - Then: the movement input direction matches the yaw-only forward vector within `±1.0` degree.
  - Edge cases: pitch and roll are non-zero but excluded from movement basis.
- **AC-2**: Diagonal clamp
  - Given: simultaneous full forward and right input.
  - When: the combined movement input is captured.
  - Then: its magnitude is at most `1.0`.
  - Edge cases: analog values just below and above unit magnitude.
- **AC-3/4**: Facing and independence
  - Given: idle, strafe, and diagonal movement states.
  - When: controller yaw changes sharply.
  - Then: facing matches on the same tick and no camera tick output is required by Movement.
  - Edge cases: missing controller is handled without a crash.

---

## Test Evidence

**Story Type**: Integration
**Required evidence**:
- `tests/integration/camera/camera_relative_movement_test.*`
- Existing movement evidence may be referenced, but the Camera-epic integration assertion must execute.

**Status**: [ ] Not yet created

---

## Dependencies

- Depends on: Player Movement Story 001 (Complete), Story 001
- Unlocks: None
