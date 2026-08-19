# Story 009: Camera Shake Dispatch + Amplitude Cap

> **Epic**: Camera System Foundation Fixes
> **Status**: Ready
> **Layer**: Core
> **Type**: Logic
> **Estimate**: 2.5h
> **Manifest Version**: 2026-07-27
> **Last Updated**: (set by /dev-story when implementation begins)

## Context

**GDD**: `design/gdd/camera-system-base.md`
**Requirement**: `TR-cam-008` (shake-cap half — see Story 007 for the look-suppression half)
*(Requirement text lives in `docs/architecture/tr-registry.yaml` — read fresh at review time)*

**ADR Governing Implementation**: ADR-0005: Camera System (base) — SpringArm + Data-Driven Config
**ADR Decision Summary**: `UCameraShakeBase` subclasses per trigger (damage/Supernova/Just-Dodge), owned by `AMoonPlayerCameraManager`, with a hard cap on concurrent shakes and a same-class-restart-not-stack policy to prevent unbounded amplitude compounding.

**Engine**: Unreal Engine 5.8 | **Risk**: LOW
**Engine Notes**: `UCameraShakeBase` is a stable pre-cutoff API.

**Control Manifest Rules (this layer)**:
- Guardrail: Camera SpringArm/FOV/shake work is presentation-side and must not gate movement, damage, or cast judgment — source: ADR-0005.

**⚠️ No dedicated GDD QA-TEST number**: Camera shake amplitude capping (Rule 9, Edge Case 6) is not one of the 10 numbered QA-TESTs in `camera-system-base.md` — the acceptance criteria below are derived directly from Rule 9 and Edge Case 6's prose. Flag this gap to qa-lead when `/qa-plan` runs for this story.

---

## Acceptance Criteria

*Derived from GDD `design/gdd/camera-system-base.md` Rule 9 and Edge Case 6 (no dedicated QA-TEST # exists):*

- [ ] Three `UCameraShakeBase` subclasses exist for: damage hit (Pitch/Roll shake proportional to damage, amplitude-capped), Supernova explosion (Radial shake with distance-based attenuation), Just-Dodge (small fast lateral shake)
- [ ] `AMoonPlayerCameraManager` owns shake dispatch and enforces a hard cap on concurrent active shakes
- [ ] When a new shake of the same class is triggered while one is already active, the existing instance restarts (does not stack) — prevents unbounded amplitude compounding from rapid repeated triggers (Edge Case 6)
- [ ] Total camera shake amplitude across all concurrent shakes never exceeds the amplitude cap regardless of how many trigger sources fire in the same window

---

## Implementation Notes

*Derived from ADR-0005 Decision 6:*

"Camera shake (Rule 9, Edge Case 6): `UCameraShakeBase` subclasses per trigger (damage/Supernova/Just-Dodge), owned by `AMoonPlayerCameraManager`, with a hard cap on concurrent shakes and same-class-restart-not-stack policy to prevent unbounded amplitude compounding."

This story shares `AMoonPlayerCameraManager` with Story 002 (pitch clamp) — both live in the same class. Trigger wiring (subscribing to Health/Damage Core's damage delegate, a future Supernova event, and Dash/Evasion's Just-Dodge success) may partially depend on those upstream systems already existing; verify their delegate signatures before wiring rather than assuming.

---

## Out of Scope

*Handled by neighbouring stories — do not implement here:*

- Story 002: Pitch clamp (same class, different concern)
- Upstream trigger sources' own implementation (Health/Damage Core, Dash/Evasion Just-Dodge — those systems fire the delegates this story subscribes to)

---

## QA Test Cases

*Written by /qa-plan sprint (2026-08-14). No dedicated GDD QA-TEST # exists for this story — cases authored directly from Rule 9 / Edge Case 6. The developer implements against these — do not invent new test cases during implementation.*

- **AC-1**: three shake subclasses exist and are triggerable
  - Given: damage-hit, Supernova-explosion, and Just-Dodge trigger events
  - When: each fires independently
  - Then: the corresponding `UCameraShakeBase` subclass plays (Pitch/Roll for damage, Radial with distance attenuation for Supernova, small fast lateral for Just-Dodge)
  - Edge cases: a trigger event firing with zero associated magnitude (e.g. 0 damage) must not play a zero-amplitude shake that still counts against the concurrency cap

- **AC-2**: concurrent shake hard cap enforced
  - Given: more simultaneous shake triggers fire than the configured cap
  - When: `AMoonPlayerCameraManager` dispatches them
  - Then: only up to the cap are active at once; excess triggers are dropped or queued, not force-played
  - Edge cases: cap boundary exactly met (N triggers, cap=N) must all play normally

- **AC-3 (Edge Case 6)**: same-class restart, not stack
  - Given: a shake of a given class is already active
  - When: another shake of the same class is triggered before the first finishes
  - Then: the existing instance restarts (resets its own timeline) rather than a second instance stacking on top
  - Edge cases: rapid-fire triggers of the same class (e.g. multiple enemies exploding within one frame) must not compound amplitude beyond the single-instance cap

- **AC-4**: total amplitude never exceeds cap
  - Given: multiple different-class shakes are active concurrently (e.g. damage + Supernova at once)
  - When: their combined camera offset is computed
  - Then: total amplitude across all concurrent shakes stays within the configured amplitude cap
  - Edge cases: worst-case simultaneous trigger of all three classes at once

---

## Test Evidence

**Story Type**: Logic
**Required evidence**:
- Logic: `tests/unit/camera/camera-shake-cap_test.cpp` — must exist and pass

**Status**: [ ] Not yet created

---

## Dependencies

- Depends on: Story 001 (`UMoonCameraSettings`), Story 002 (`AMoonPlayerCameraManager` class must exist)
- Unlocks: None
