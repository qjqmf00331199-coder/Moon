# Story 004: SpringArm Lag Max-Distance Hard-Follow

> **Epic**: Camera System Foundation Fixes
> **Status**: Ready
> **Layer**: Core
> **Type**: Logic
> **Estimate**: 2h
> **Manifest Version**: 2026-07-27
> **Last Updated**: (set by /dev-story when implementation begins)

## Context

**GDD**: `design/gdd/camera-system-base.md`
**Requirement**: `TR-cam-004`
*(Requirement text lives in `docs/architecture/tr-registry.yaml` — read fresh at review time)*

**ADR Governing Implementation**: ADR-0005: Camera System (base) — SpringArm + Data-Driven Config
**ADR Decision Summary**: `UMoonCameraSettings`-driven SpringArm config; `CameraLagMaxDistance` preserved from current shipped tuning at `60.0uu`.

**Engine**: Unreal Engine 5.8 | **Risk**: LOW
**Engine Notes**: None beyond standard SpringArm lag properties.

**Control Manifest Rules (this layer)**:
- Required: Camera configuration must be data-asset driven through `UMoonCameraSettings` — source: ADR-0005.
- Guardrail: Camera SpringArm/FOV/shake work is presentation-side and must not gate movement, damage, or cast judgment — source: ADR-0005.

**⚠️ Known GDD inconsistency (do not fix here, do not propagate the wrong value)**: `camera-system-base.md` Formula 3 (`:138`) and the Arena Morphing dependency row (`:204`) state `200uu`, while Rule 4 (`:69`), Edge Case 4 (`:183`), the Tuning Knobs table (`:221`, safe range 40-120), and QA-TEST-05's AC (`:256`) all state `60.0uu`. Per `docs/architecture/tr-registry.yaml` TR-cam-004's revision note and ADR-0005, **`60.0uu` is the ratified value** — implement against `60.0uu`. The GDD's own Formula 3 / Arena Morphing row still needs a documentation fix, tracked separately (architecture-review-2026-07-27.md C-3), not part of this story.

---

## Acceptance Criteria

*From GDD `design/gdd/camera-system-base.md`, scoped to this story (QA-TEST-05):*

- [ ] `CameraLagMaxDistance = 60.0uu` (from `UMoonCameraSettings`, safe range 40.0-120.0uu)
- [ ] While `bEnableCameraLag=true` and the character accelerates away from the camera (dash, launch), the world distance between camera and character never exceeds `60.0uu`
- [ ] On reaching the `60.0uu` limit, the SpringArm hard-follows (no further lag delay) rather than continuing to interpolate past the cap — Formula 3's clamp branch
- [ ] Edge Case 4: a `Z`-axis launch of `3000uu/s` or greater does not push the character off-screen (this is the concrete scenario the cap exists to prevent)

---

## Implementation Notes

*Derived from ADR-0005 Migration Plan step 1 (asset defaults) and GDD Formula 3:*

Formula 3's clamp branch: once `Distance = ‖L_current(t) - L_target(t)‖ > MaxDistance`, snap `L_final(t)` to `L_target(t) + (L_current(t) - L_target(t)) / Distance × MaxDistance` — this is a per-frame position clamp, not a one-shot teleport. Confirm this matches (or is subsumed by) the SpringArm's native `CameraLagMaxDistance` property behavior in UE5.8 before writing custom clamp code — the property likely already implements this formula natively.

---

## Out of Scope

*Handled by neighbouring stories — do not implement here:*

- Story 008: `ResetCameraLag()` on teleport (different scenario — instantaneous large displacement, not sustained high-speed pursuit)
- Fixing the GDD's own Formula 3 / Arena Morphing `200uu` inconsistency (separate documentation task)

---

## QA Test Cases

*Written by /qa-plan sprint (2026-08-14). The developer implements against these — do not invent new test cases during implementation.*

- **AC-1**: `CameraLagMaxDistance = 60.0uu` from data asset
  - Given: `UMoonCameraSettings.CameraLagMaxDistance = 60.0`
  - When: `BeginPlay` applies settings to `CameraBoom`
  - Then: `CameraBoom->CameraLagMaxDistance == 60.0`
  - Edge cases: value outside the safe range (40.0-120.0) — no runtime clamp required by the GDD, but flag if the asset default drifts outside range during data entry

- **AC-2 (QA-TEST-05)**: distance never exceeds cap during sustained acceleration
  - Given: `bEnableCameraLag=true`, character accelerates away from camera (dash chain, launch)
  - When: camera-to-character world distance is sampled every tick during the acceleration
  - Then: distance never exceeds `60.0uu` at any sampled tick
  - Edge cases: multiple rapid direction reversals must not produce a transient overshoot past `60.0uu`

- **AC-3**: hard-follow at the cap, not continued lag
  - Given: distance has reached the `60.0uu` cap
  - When: the character continues moving away
  - Then: the SpringArm hard-follows (distance stays pinned at `60.0uu`, not still lagging further behind)
  - Edge cases: none beyond the cap-holding behavior itself

- **AC-4 (Edge Case 4)**: high-speed Z-launch does not go off-screen
  - Given: character receives a `Z`-axis launch of `≥3000uu/s`
  - When: the launch resolves over subsequent ticks
  - Then: character remains within the camera's visible frame (verified via viewport screenshot or on-screen-bounds check in PIE)
  - Edge cases: launch combined with simultaneous horizontal dash input (compound velocity)

---

## Test Evidence

**Story Type**: Logic
**Required evidence**:
- Logic: `tests/unit/camera/lag-max-distance_test.cpp` — must exist and pass

**Status**: [ ] Not yet created

---

## Dependencies

- Depends on: Story 001 (`UMoonCameraSettings.CameraLagMaxDistance` must exist)
- Unlocks: None
