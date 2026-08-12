## QA Plan: Player Movement Foundation Fixes
**Date**: 2026-08-12
**Epic**: production/epics/player-movement-foundation-fixes/
**Story Count**: 5
**Review Mode**: solo (no director gates)

---

### Story Classification Table

| Story | Type | Automated Required | Manual Required | Blocker? |
|-------|------|--------------------|------------------|----------|
| 001 Camera-Yaw Facing and Movement Independence | Logic | Yes — `camera_yaw_facing_test.ps1`, `movement_independence_check.ps1` (PASS) | AC-4 root-motion editor/uasset inspection (deferred) | No |
| 002 Data-Driven Movement Tuning and Clamp Enforcement | Logic (reclassified from Config/Data — real clamp/joint-bound code) | Yes — `tuning_clamp_and_joint_bound_test.ps1` (PASS, 4/4 AC) | None | No |
| 003 Airborne Substate, Jump Buffer, and Coyote Runtime | Logic | Yes — `airborne_and_grace_windows_test.ps1`, `movement_lock_contract_test.ps1` (PASS, 4/4 AC) | None | No |
| 004 Presentation-Only Hitstop Rewrite | Visual/Feel | Static only — `hitstop_no_time_dilation_check.ps1` (PASS) | AC-1/AC-3/AC-4 human PIE playtest (deferred) | No (ADVISORY — Visual/Feel never blocks) |
| 005 Movement Traceability and Static Regression Checks | Integration | Yes — `movement_traceability_test.ps1`, `movement_regression_checks.ps1` (PASS) | AC-4 N≥100 actor benchmark, reference hardware (deferred) | No (Production-milestone scoped) |

**Smoke Check**: PASS — source: `production/qa/smoke-2026-08-12.md` (9/9 automated tests, all manual smoke checks pass).

---

### Automated Test Requirements

All 9 tests already exist and pass (verified in smoke-2026-08-12.md):
- `tests/unit/movement/airborne_and_grace_windows_test.ps1`
- `tests/unit/movement/camera_yaw_facing_test.ps1`
- `tests/unit/movement/movement_lock_contract_test.ps1`
- `tests/unit/movement/tuning_clamp_and_joint_bound_test.ps1`
- `tests/integration/movement/movement_traceability_test.ps1`
- `tests/static/hitstop_no_time_dilation_check.ps1`
- `tests/static/movement_foundation_contract.ps1`
- `tests/static/movement_independence_check.ps1`
- `tests/static/movement_regression_checks.ps1`

No new automated test files required for this cycle.

---

### Manual QA Scope (this sprint)

1. **Story 004 PIE session**: hitstop trigger on landing, movement continuity during freeze, dash-cancel-during-freeze, AnimNotify SFX timing. Capture normal + slow-mo clips for evidence doc reviewer checklist (`production/qa/evidence/presentation-only-hitstop-rewrite-evidence.md`).
2. **Story 001 AC-4 editor check**: inspect root-motion import flag on jump/landing AnimSequence assets. Piggyback on session 1 (content inspection, not full playtest).

Estimated effort: 2 sessions, ~45-60 min combined.

---

### Out of Scope

- **Story 005 AC-4** (N≥100 actor benchmark, 300 frames on reference hardware) — deferred to Production-milestone prep. Evidence-only per story text ("evidence-only until target minimum hardware is final"); does not gate this sprint's Done bar.

---

### Entry Criteria

- [x] Smoke check PASS or PASS WITH WARNINGS report exists at `production/qa/smoke-2026-08-12.md`
- [x] Build stable — no crash on launch (confirmed in smoke report)
- [x] All 5 stories at status Complete/Complete-with-notes (no `production/sprint-status.yaml` exists in this project — verified directly against story files' status fields instead)

---

### Exit Criteria

QA cycle complete when all 5 stories reach one of: PASS, PASS WITH NOTES, or FAIL with a bug filed in `production/qa/bugs/`. Deferred/advisory items (Story 001 AC-4, Story 004 AC-1/3/4, Story 005 AC-4) are tracked as open items, not treated as FAIL, per their story-type gate tier.
