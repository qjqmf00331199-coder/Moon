# Story 005: Collision Guardrails — Debris Ignore + Corner Dithering

> **Epic**: Camera System Foundation Fixes
> **Status**: Ready
> **Layer**: Core
> **Type**: Integration
> **Estimate**: 3h
> **Manifest Version**: 2026-07-27
> **Last Updated**: 2026-08-19

## Context

**GDD**: `design/gdd/camera-system-base.md`
**Requirement**: `TR-cam-005`
*(Requirement text lives in `docs/architecture/tr-registry.yaml` — read fresh at review time)*

**ADR Governing Implementation**: ADR-0005: Camera System (base) — SpringArm + Data-Driven Config
**ADR Decision Summary**: `bDoCollisionTest=true`, `ProbeSize` from `UMoonCameraSettings` (default 12.0), query channel `ECC_Camera`. Destructible Geometry (not yet designed) is pre-obligated by this ADR to classify its debris under a channel the CameraBoom's collision response ignores.

**Engine**: Unreal Engine 5.8 | **Risk**: LOW
**Engine Notes**: Standard SpringArm collision probe (`bDoCollisionTest`, `ProbeSize`, `ProbeChannel`) — pre-cutoff API.

**Control Manifest Rules (this layer)**:
- Guardrail: Camera SpringArm/FOV/shake work is presentation-side and must not gate movement, damage, or cast judgment — source: ADR-0005.

**Cross-epic forward obligation**: Destructible Geometry does not exist as a GDD/epic yet. This story implements the camera side of the contract now (ignore `ECC_Destructible` in the SpringArm's collision response) so that whichever epic designs Destructible Geometry next only has to classify debris under that channel — it does not have to touch camera code.

---

## Acceptance Criteria

*From GDD `design/gdd/camera-system-base.md`, scoped to this story:*

- [ ] SpringArm `bDoCollisionTest=true`, `ProbeSize=12.0uu` (from `UMoonCameraSettings`), query channel `ECC_Camera` (Rule 5)
- [ ] SpringArm collision response ignores a dedicated `ECC_Destructible` trace channel (Rule 5, Edge Case 3) — camera only reacts to `WorldStatic`/`WorldDynamic`
- [ ] **QA-TEST-07**: when physics debris (Geometry Collection fragments) pass between camera and character, `TargetArmLength` stays at its configured value (`450.0uu` default) — no rapid-snap artifact from debris blocking the probe
- [ ] **QA-TEST-10**: when the SpringArm's actual computed length shrinks to `≤80.0uu` (Edge Case 1 — player backed into a corner), the character mesh material applies a dither (Temporal AA) fade toward transparent, and the camera's near-clipping plane is set to `10.0uu` so no interior-mesh clipping is visible

---

## Implementation Notes

*Derived from ADR-0005 Decision 5 and GDD Edge Cases 1 and 3:*

Two independent guardrails in one story because both are "camera must not let geometry ruin visibility" — corner dithering (Edge Case 1) and debris-ignore (Edge Case 3) share the same collision-probe code path even though their fixes are different (material dither vs. channel-ignore).

This story only registers the `ECC_Destructible` ignore rule on the camera side — it does not invent or implement the channel classification itself. That is a forward obligation on the Destructible Geometry epic (not yet designed).

---

## Out of Scope

*Handled by neighbouring stories — do not implement here:*

- Classifying actual Geometry Collection debris under `ECC_Destructible` (Destructible Geometry epic, not yet created)
- Story 001: `UMoonCameraSettings.CameraProbeSize` field itself (this story only reads it)

---

## QA Test Cases

*Written by /qa-plan sprint (2026-08-14). The developer implements against these — do not invent new test cases during implementation.*

- **AC-1**: collision probe config
  - Given: `CameraBoom` is configured from `UMoonCameraSettings`
  - When: collision properties are read
  - Then: `bDoCollisionTest=true`, `ProbeSize=12.0uu`, probe channel `ECC_Camera`
  - Edge cases: `ProbeSize` outside safe range (5.0-25.0uu) flagged during data entry, not a runtime failure

- **AC-2**: `ECC_Destructible` ignored in collision response
  - Given: SpringArm's collision response channel map is configured
  - When: a test actor on the `ECC_Destructible` channel overlaps the probe
  - Then: the SpringArm's collision response for `ECC_Destructible` is `Ignore`
  - Edge cases: a `WorldStatic` actor at the same position must still block (only `ECC_Destructible` is ignored, not all channels)

- **AC-3 (QA-TEST-07)**: debris does not cause rapid-snap
  - Given: multiple physics debris actors (simulating Geometry Collection fragments) pass between camera and character on `ECC_Destructible`
  - When: the debris crosses the probe's line repeatedly over several seconds
  - Then: `TargetArmLength` stays at its configured value (`450.0uu` default) throughout — no sudden reduction from debris blocking
  - Edge cases: dozens of simultaneous debris pieces (worst-case Arena Morphing scenario) must not degrade this guarantee

- **AC-4 (QA-TEST-10)**: corner dithering at ≤80uu computed length
  - Given: player is backed into a corner, SpringArm's actual computed length shrinks to `≤80.0uu`
  - When: the character mesh is rendered
  - Then: character mesh material applies a dither (Temporal AA) fade toward transparent; near-clipping plane is set to `10.0uu`
  - Edge cases: computed length recovering above `80.0uu` (player steps away) must fade the dither back out, not leave the mesh stuck transparent

---

## Test Evidence

**Story Type**: Integration
**Required evidence**:
- Integration: `tests/integration/camera/collision-guardrails_test.cpp` OR documented playtest — must exist and pass, or documented evidence in `production/qa/evidence/`

**Status**: [ ] Not yet created

---

## Dependencies

- Depends on: Story 001 (`UMoonCameraSettings.CameraProbeSize`)
- Unlocks: Destructible Geometry epic's future debris-channel classification (forward obligation, not a blocking dependency)
