# Story 004: Camera Lag Hard Limit

> **Epic**: Camera System Foundation Fixes
> **Status**: Ready
> **Layer**: Core
> **Type**: Logic
> **Estimate**: 2-4 hours
> **Manifest Version**: 2026-07-27
> **Last Updated**: Not started

## Context

**GDD**: `design/gdd/camera-system-base.md`
**Requirement**: `TR-cam-004`

**ADR Governing Implementation**: ADR-0005: Camera System SpringArm
**ADR Decision Summary**: Use native SpringArm positional lag, keep rotation lag disabled for aiming, and load the hard `CameraLagMaxDistance=60.0uu` limit from camera settings so high-speed displacement cannot leave the character off-screen.

**Engine**: Unreal Engine 5.8 | **Risk**: LOW
**Engine Notes**: Validate the SpringArm lag/substepping fields against installed UE5.8 headers and prove behavior in PIE; source-property presence alone does not prove the hard-follow result.

**Control Manifest Rules (this layer)**:
- Required: settings-driven SpringArm configuration.
- Forbidden: runtime hardcoded tuning as the source of truth.
- Guardrail: lag processing remains presentation-only.

---

## Acceptance Criteria

- [ ] Position lag is enabled, rotation lag is disabled, control rotation is inherited for pitch/yaw but not roll.
- [ ] `CameraLagSpeed=18.0` and `CameraLagMaxDistance=60.0uu` are applied from `UMoonCameraSettings` defaults.
- [ ] During dash or a launch of at least `3000uu/s` on Z, lag displacement never exceeds `60.0uu`; at the boundary the camera hard-follows.
- [ ] Lag substepping behaves consistently at representative low and high frame rates without overshoot beyond the hard limit.

---

## Implementation Notes

- Configure `bEnableCameraLag`, `bEnableCameraRotationLag`, inheritance flags, max distance, speed, and verified substepping properties from Story 001 settings.
- Measure SpringArm lag displacement, not raw `TargetArmLength`, when asserting the `60uu` cap.
- Do not add gameplay state changes in response to lag distance.

---

## Out of Scope

- Teleport cache reset belongs to Story 008.
- Wall/debris probe collision belongs to Story 005.
- Dynamic FOV belongs to Story 006.

---

## QA Test Cases

- **AC-1/2**: Lag configuration
  - Given: the production camera settings asset and a spawned player.
  - When: SpringArm runtime properties are inspected.
  - Then: positional lag and approved defaults are active, rotation lag is off, and roll is not inherited.
  - Edge cases: Blueprint defaults do not overwrite the loaded asset.
- **AC-3**: Hard limit
  - Given: position lag is active.
  - When: dash and `>=3000uu/s` vertical launch scenarios run.
  - Then: measured lag displacement remains `<=60.0uu` and hard-follow engages at the limit.
  - Edge cases: repeated launches and direction reversal.
- **AC-4**: Frame-rate stability
  - Given: low and high fixed-frame-rate test runs.
  - When: the same launch is replayed.
  - Then: neither run exceeds `60.0uu` and both converge without visible oscillation.
  - Edge cases: a single long frame.

---

## Test Evidence

**Story Type**: Logic
**Required evidence**:
- `tests/unit/camera/camera_lag_hard_limit_test.*`
- `production/qa/evidence/camera-lag-hard-limit-evidence.md`

**Status**: [ ] Not yet created

---

## Dependencies

- Depends on: Story 001
- Unlocks: Story 008
