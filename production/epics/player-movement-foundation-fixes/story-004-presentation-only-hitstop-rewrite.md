# Story 004: Presentation-Only Hitstop Rewrite

> **Epic**: Player Movement Foundation Fixes
> **Status**: Complete
> **Layer**: Foundation
> **Type**: Visual/Feel
> **Estimate**: 2-4 hours
> **Manifest Version**: 2026-07-27
> **Last Updated**: 2026-08-12

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

- [x] GIVEN landing hitstop occurs, WHEN movement input occurs in the same frame, THEN velocity updates match a non-hitstop control frame. — Static check confirms `UpdateHitStopPresentation()` never writes `Velocity`/`SetActorLocation`/`SetActorTransform`. Human PIE playtest 2026-08-12: PASS.
- [x] GIVEN movement source is searched, WHEN static checks run, THEN hitstop paths contain no `CustomTimeDilation` or global Time Dilation calls. — `tests/static/hitstop_no_time_dilation_check.ps1` PASS.
- [x] GIVEN hitstop occurs during dash-cancel or movement input, WHEN visual evidence is reviewed, THEN pose sliding and snap-back artifacts are not visible enough to fail the GDD threshold. — Human PIE playtest 2026-08-12: PASS. See `production/qa/evidence/presentation-only-hitstop-rewrite-evidence.md`.
- [x] GIVEN AnimNotify landing SFX would occur inside the freeze window, WHEN manual audio check is run, THEN SFX is not delayed or dropped. — Advisory per story text; human PIE playtest 2026-08-12: PASS. See evidence doc's AC-4 section for the partial-mitigation note (Landed() reorder).

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

**Status**: [x] Created — both present

**Verification (2026-07-27)**:
- `tests/static/hitstop_no_time_dilation_check.ps1` — PASS (zero `CustomTimeDilation`/`SetGlobalTimeDilation` refs; positive capture-and-blend mechanism confirmed)
- `production/qa/evidence/presentation-only-hitstop-rewrite-evidence.md` — created, documents mechanism, re-trigger-drift fix, ADR-0005 cross-check (no camera change needed)
- Regression re-run: `movement_foundation_contract.ps1`, `movement_independence_check.ps1`, `airborne_and_grace_windows_test.ps1`, `movement_lock_contract_test.ps1`, `camera_yaw_facing_test.ps1` — all PASS
- UBT full build (`MoonEditor Win64 Development`) — **Succeeded** after one retry (first attempt failed on a machine-side low-memory PCH error, `C1076`/paging file too small, not a code error — 7.1GB free of 32GB at the time; retry succeeded)

**Verification (2026-08-12)**:
- PIE runtime verification — performed. `UnrealEditor.exe` launched this session, user played the build directly and reported AC-1/AC-3/AC-4 all PASS per the reviewer checklist in the evidence doc. See evidence doc's "Human PIE playtest result" section.

**Verified during implementation review**: traced the capture-and-blend logic (freeze/blend-out phase machine, re-trigger drift fix, natural-transform composition via `CapturedMeshRelativeTransform * AttachParent->GetComponentTransform()`) by reading the actual diff, not just trusting the self-reported PASS — logic checks out.

---

## Dependencies

- Depends on: Story 001 (status: Complete, 4/4 AC passing as of 2026-08-12)
- Unlocks: playtest build validation and Dash/Evasion feel validation

## Completion Notes
**Completed**: 2026-08-12 (AC-1/3/4 closed 2026-08-12 via human PIE playtest, same day as static close)
**Criteria**: 4/4 passing
**Deviations**: None blocking. Advisory: 40ms vs 55ms GDD tuning-table mismatch (pre-existing, out of scope); Blueprint `TriggerHitStop` reference grep timed out on full `Moon/Content` sweep, low risk not exhaustively confirmed
**Test Evidence**: `tests/static/hitstop_no_time_dilation_check.ps1` (re-run live, PASS) + `production/qa/evidence/presentation-only-hitstop-rewrite-evidence.md` (now includes 2026-08-12 human PIE playtest result: AC-1/AC-3/AC-4 all PASS)
**Code Review**: Skipped — Solo mode
