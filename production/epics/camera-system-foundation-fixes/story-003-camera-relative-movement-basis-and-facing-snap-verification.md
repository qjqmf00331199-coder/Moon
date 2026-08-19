# Story 003: Camera-Relative Movement Basis + Facing Snap Verification

> **Epic**: Camera System Foundation Fixes
> **Status**: Ready
> **Layer**: Core
> **Type**: Logic
> **Estimate**: 2h
> **Manifest Version**: 2026-07-27
> **Last Updated**: (set by /dev-story when implementation begins)

## Context

**GDD**: `design/gdd/camera-system-base.md`
**Requirement**: `TR-cam-003`
*(Requirement text lives in `docs/architecture/tr-registry.yaml` — read fresh at review time)*

**ADR Governing Implementation**: ADR-0005: Camera System (base) — SpringArm + Data-Driven Config
**ADR Decision Summary**: Camera-relative movement input (Rule 3, Formula 1) is already implemented and unaffected by this ADR. This ADR does, however, ratify Rule 6's rotation contract (`bUseControllerRotationYaw=true`, `bOrientRotationToMovement=false`) as canonical and requires a PIE strafe-aim acceptance check, since a rotation-flag deviation was found and tracked as a separate bug fix (believed resolved by the Player Movement epic's Story 001 — verify, do not assume).

**Engine**: Unreal Engine 5.8 | **Risk**: LOW
**Engine Notes**: None beyond standard `FRotationMatrix`/`AddMovementInput` usage.

**Control Manifest Rules (this layer)**:
- Required: Player movement must use camera-relative input, diagonal magnitude normalization, and camera-yaw facing with `bUseControllerRotationYaw=true`, `bOrientRotationToMovement=false`, `bUseControllerRotationPitch=false`, `bUseControllerRotationRoll=false` — source: ADR-0001, ADR-0005.
- Guardrail: Camera SpringArm/FOV/shake work is presentation-side and must not gate movement, damage, or cast judgment — source: ADR-0005.

**⚠️ Note**: TR-cam-003 has ⚠️ Partial ADR coverage per the epic's GDD Requirements table — not because the formula is wrong, but because this story's verification check is what closes that gap.

---

## Acceptance Criteria

*From GDD `design/gdd/camera-system-base.md`, scoped to this story:*

- [ ] **QA-TEST-01**: at controller Yaw = 0°, 45°, 90°, 180°, forward input (W / stick-up 100%) moves the character along the camera's Forward unit vector, within ±1.0° tolerance
- [ ] **QA-TEST-02**: diagonal input (W+D or diagonal stick 100%) produces a movement input vector clamped/normalized to magnitude `1.0` (no √2 diagonal speed boost)
- [ ] **QA-TEST-03**: on a fast mouse/stick snap changing controller Yaw, character mesh facing matches the new controller Yaw within the same tick (0 frames of lerp delay) — this is the explicit ADR-0005 Validation Criteria check tied to the Rule 6 ratification
- [ ] Movement and Camera each independently read `PlayerController` rotation — no execution-order dependency, no circular reference between the two systems

---

## Implementation Notes

*Derived from ADR-0005 Validation Criteria and GDD Formula 1:*

Per ADR-0005: "New explicit check tied to the Rule 6 fix: strafe in all 4 diagonal directions while continuously look-input rotating; character facing must snap to camera yaw every tick with zero lag, confirming aim stays screen-locked."

Before writing new code, verify whether `MoonCharacterBase.cpp`'s rotation flags already match the ratified contract (`bUseControllerRotationYaw=true`, `bOrientRotationToMovement=false`) — the ADR flagged these as reversed at authoring time (2026-07-23), and the Player Movement epic's Story 001 ("Camera Yaw Facing and Movement Independence") may have already fixed this. Confirm via source read, not assumption, before touching this code.

---

## Out of Scope

*Handled by neighbouring stories — do not implement here:*

- Story 001: `UMoonCameraSettings` (this story does not read tuning knobs — Rule 3's math has no tunable parameters)
- Story 004: `CameraLagMaxDistance` off-screen prevention (different formula, different QA test)

---

## QA Test Cases

*Written by /qa-plan sprint (2026-08-14). The developer implements against these — do not invent new test cases during implementation.*

- **AC-1 (QA-TEST-01)**: forward input follows camera Forward vector
  - Given: controller Yaw is set to `0°`, `45°`, `90°`, `180°` (one case per value)
  - When: 100% forward input (W / stick-up) is applied
  - Then: character moves along the camera's Yaw-only Forward unit vector, within ±1.0° angular tolerance
  - Edge cases: Yaw wraparound at `359°→0°` must not flip the computed Forward vector direction

- **AC-2 (QA-TEST-02)**: diagonal input magnitude clamp
  - Given: W+D (or diagonal stick 100%) input is applied
  - When: the raw combined input vector is measured before injection into `AddMovementInput`
  - Then: magnitude is clamped/normalized to exactly `1.0` (not `√2 ≈ 1.414`)
  - Edge cases: single-axis 100% input (W only) must remain magnitude `1.0`, not be reduced by the clamp

- **AC-3 (QA-TEST-03)**: instant facing snap, zero lag
  - Given: character is stationary or moving, controller Yaw changes abruptly (fast mouse snap or stick flick)
  - When: the next tick renders
  - Then: character mesh facing equals controller Yaw within that same tick — 0 frames of interpolation delay
  - Edge cases: this must hold true while strafing in all 4 diagonal directions simultaneously with continuous look-input rotation (ADR-0005 Validation Criteria) — verify via PIE manual check, not unit test alone

- **AC-4**: no circular reference / execution-order dependency
  - Given: both Movement and Camera systems read `PlayerController` rotation independently
  - When: tick order between the two systems is reversed (test harness forces alternate order)
  - Then: output (movement direction, camera facing) is identical regardless of tick order
  - Edge cases: none beyond order-reversal itself — this is the whole point of the check

---

## Test Evidence

**Story Type**: Logic
**Required evidence**:
- Logic: `tests/unit/camera/camera-relative-input_test.cpp` — must exist and pass
- Manual/PIE: strafe-aim 4-diagonal-direction check per ADR-0005 Validation Criteria — record result in `production/qa/evidence/`

**Status**: [ ] Not yet created

---

## Dependencies

- Depends on: Story 001 (data-asset foundation must exist first, per epic ordering — though this story's own math has no tunable inputs)
- Unlocks: None
