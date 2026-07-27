# Story 004: Presentation-Only Hitstop Rewrite

> **Epic**: Player Movement Foundation Fixes
> **Status**: Ready
> **Layer**: Foundation
> **Type**: Visual/Feel
> **Estimate**: 2-4 hours
> **Manifest Version**: 2026-07-27
> **Last Updated**:

## Context

**GDD**: `design/gdd/player-movement.md`  
**Requirement**: `TR-mov-008`

**ADR Governing Implementation**: ADR-0009: Player Movement Runtime Contract; ADR-0005: Camera System SpringArm  
**ADR Decision Summary**: Hitstop must not use any Time Dilation. It captures mesh/camera transforms, lets capsule/CMC gameplay continue normally, holds presentation briefly, then blends back to the real transform over 1-2 frames.

**Engine**: Unreal Engine 5.8 | **Risk**: HIGH  
**Engine Notes**: Mesh animation pause and camera compensation must be verified against UE5.8 behavior and ADR-0005 SpringArm lag.

**Control Manifest Rules (this layer)**:
- Required: capture-and-blend presentation freeze only.
- Forbidden: `SetGlobalTimeDilation`, `CustomTimeDilation`, or gameplay tick slowdown.
- Guardrail: visual artifact evidence is required, not just grep.

---

## Acceptance Criteria

- [ ] GIVEN landing hitstop occurs, WHEN movement input occurs in the same frame, THEN velocity updates match a non-hitstop control frame.
- [ ] GIVEN movement source is searched, WHEN static checks run, THEN hitstop paths contain no `CustomTimeDilation` or global Time Dilation calls.
- [ ] GIVEN hitstop occurs during dash-cancel or movement input, WHEN visual evidence is reviewed, THEN pose sliding and snap-back artifacts are not visible enough to fail the GDD threshold.
- [ ] GIVEN AnimNotify landing SFX would occur inside the freeze window, WHEN manual audio check is run, THEN SFX is not delayed or dropped.

---

## Implementation Notes

- Rewrite existing `TriggerHitStop()` instead of layering a second hitstop path on top of it.
- Capture mesh transform at freeze start and force/blend mesh presentation only. Capsule and CMC remain authoritative and continue normal movement.
- If camera compensation touches SpringArm lag or `ResetCameraLag()`, cross-check ADR-0005 before implementing.
- Remove old timer-based `CustomTimeDilation` restore logic once replacement is verified.

---

## Out of Scope

- General camera system data asset work.
- Combat hitstop for non-landing impacts unless it already calls the same shared helper.
- Final sound design polish beyond the advisory AnimNotify slip check.

---

## QA Test Cases

- **Manual check: Velocity continuity**
  - Setup: trigger landing hitstop while holding movement input.
  - Verify: movement velocity updates during the freeze window.
  - Pass condition: gameplay movement matches a no-hitstop control capture.

- **Manual check: Visual artifact**
  - Setup: trigger landing hitstop and dash-cancel/move during the freeze.
  - Verify: captured clip at normal speed and slow playback.
  - Pass condition: no obvious pose sliding or snap-back; evidence doc records result.

- **Manual check: Time Dilation absence**
  - Setup: run static grep and inspect hitstop code path.
  - Verify: no global or actor Time Dilation API is used.
  - Pass condition: static check passes and code review confirms the replacement path.

- **Manual check: AnimNotify slip**
  - Setup: trigger landing SFX/impact notify during hitstop.
  - Verify: sound plays at the intended moment.
  - Pass condition: no missing or delayed landing SFX in the checked clip.

---

## Test Evidence

**Story Type**: Visual/Feel  
**Required evidence**:
- `production/qa/evidence/presentation-only-hitstop-rewrite-evidence.md`
- Static grep/test evidence under `tests/static/`

**Status**: [ ] Not yet created

---

## Dependencies

- Depends on: Story 001
- Unlocks: playtest build validation and Dash/Evasion feel validation
