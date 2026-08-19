# Story 005: Camera Collision, Debris, and Close Occlusion

> **Epic**: Camera System Foundation Fixes
> **Status**: Ready
> **Layer**: Core
> **Type**: Integration
> **Estimate**: 2-4 hours
> **Manifest Version**: 2026-07-27
> **Last Updated**: Not started

## Context

**GDD**: `design/gdd/camera-system-base.md`
**Requirement**: `TR-cam-005`

**ADR Governing Implementation**: ADR-0005: Camera System SpringArm
**ADR Decision Summary**: SpringArm collision uses `ECC_Camera` and a settings-driven `12uu` probe, future destructible debris must not block the camera, and close camera-to-character occlusion is handled as presentation without changing gameplay collision.

**Engine**: Unreal Engine 5.8 | **Risk**: LOW
**Engine Notes**: The Destructible Geometry GDD/channel is not yet designed. Implement the camera-side response contract without inventing ownership of a project collision channel that does not exist.

**Control Manifest Rules (this layer)**:
- Required: settings-driven probe configuration and camera-only presentation handling.
- Forbidden: hardcoded runtime camera tuning or gameplay collision changes to solve visibility.
- Guardrail: collision/dither work must not gate movement or damage.

---

## Acceptance Criteria

- [ ] SpringArm collision testing is enabled on `ECC_Camera` with `ProbeSize=12.0uu` loaded from settings, and static/world geometry prevents wall penetration.
- [ ] Debris classified by the Destructible Geometry contract does not shorten the `450.0uu` arm or cause rapid snap/jitter when crossing the probe.
- [ ] When actual camera-to-character distance falls to `80.0uu` or less, the player mesh uses a dithered fade so internal polygons do not obscure the view.
- [ ] When the camera leaves the close-occlusion threshold, the original mesh materials/visibility are restored without persistent state.

---

## Implementation Notes

- Configure native SpringArm collision with `bDoCollisionTest=true`, `ProbeSize` from Story 001 settings, and `ECC_Camera`.
- Add a camera-side ignore hook/response contract for future destructible debris. If no registered debris channel exists, keep the story integration explicit and test with a controlled test object rather than silently inventing a production channel.
- Implement dither using a reversible material parameter/overlay path. Do not mutate source materials permanently and do not change capsule collision.
- Treat the GDD near-clipping-plane note as project/editor configuration evidence if no safe per-camera runtime API exists.

---

## Out of Scope

- Authoring the Destructible Geometry GDD or choosing its final channel.
- Camera shake from explosions belongs to Story 009.
- Camera lag behavior belongs to Story 004.

---

## QA Test Cases

- **AC-1**: World collision
  - Given: a player backing toward a solid wall.
  - When: the SpringArm probe contacts the wall.
  - Then: the camera retracts without rendering through the wall, using a `12.0uu` probe.
  - Edge cases: narrow corners and fast lateral movement.
- **AC-2**: Debris ignore
  - Given: controlled debris objects passing between player and camera.
  - When: they cross the probe repeatedly.
  - Then: arm length remains at its unobstructed target and no rapid snap/jitter occurs.
  - Edge cases: many simultaneous fragments; solid world geometry must still block.
- **AC-3/4**: Close occlusion fade
  - Given: actual camera-to-character distance crosses `80.0uu` in both directions.
  - When: the player is forced against a wall and then moves clear.
  - Then: the mesh fades before internal polygons obscure the view and restores afterward.
  - Edge cases: repeated threshold crossing and respawn during fade.

---

## Test Evidence

**Story Type**: Integration
**Required evidence**:
- `tests/integration/camera/camera_collision_and_debris_test.*`
- `production/qa/evidence/camera-collision-debris-and-close-occlusion-evidence.md`

**Status**: [ ] Not yet created

---

## Dependencies

- Depends on: Story 001
- Unlocks: None
