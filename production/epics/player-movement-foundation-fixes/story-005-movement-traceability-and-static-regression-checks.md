# Story 005: Movement Traceability and Static Regression Checks

> **Epic**: Player Movement Foundation Fixes
> **Status**: Complete
> **Layer**: Foundation
> **Type**: Integration
> **Estimate**: 2-4 hours
> **Manifest Version**: 2026-07-27
> **Last Updated**: 2026-08-12

## Context

**GDD**: `design/gdd/player-movement.md`  
**Requirement**: `TR-mov-002`, `TR-mov-009`, `TR-mov-010`

**ADR Governing Implementation**: ADR-0009: Player Movement Runtime Contract  
**ADR Decision Summary**: Add Unreal Insights trace scopes at input and velocity-update points, and ensure static checks keep Movement independent of Spell Casting, non-root-motion, and free of forbidden input-lock or Time Dilation patterns.

**Engine**: Unreal Engine 5.8 | **Risk**: HIGH  
**Engine Notes**: `TRACE_CPUPROFILER_EVENT_SCOPE` behavior and Enhanced Input trigger timing require implementation-time verification against local UE5.8 headers/source.

**Control Manifest Rules (this layer)**:
- Required: movement trace scopes for p95 input-response measurement.
- Forbidden: Spell Casting references, root-motion locomotion, montage input lock, and Time Dilation in movement presentation.
- Guardrail: checks should run as part of local validation before story closure.

---

## Acceptance Criteria

- [x] (source-level, not live-captured) GIVEN movement input is processed, WHEN Insights tracing is enabled, THEN `MovementInputTrace.InputTriggered` is emitted at the Enhanced Input Triggered callback start. — `TRACE_CPUPROFILER_EVENT_SCOPE(MovementInputTrace.InputTriggered)` as the first statement in `Move()`, before the `bMovementLocked` gate; macro syntax verified against the real UE5.8 header. Live Insights capture DEFERRED (no Editor session this pass).
- [x] (source-level, not live-captured) GIVEN CMC has updated velocity toward the requested movement, WHEN tracing is enabled, THEN `MovementInputTrace.VelocityUpdated` is emitted at the first post-CMC frame. — same macro, in `Tick()` immediately after `Super::Tick()`. Live Insights capture DEFERRED.
- [x] GIVEN static movement checks run, THEN Spell Casting references, montage input-lock patterns, Time Dilation calls, and root-motion locomotion violations are reported as failures. — `tests/static/movement_regression_checks.ps1`; each detector self-tested against a known-bad fixture before trusting its "clean" verdict on real source (including a caught-and-fixed false-positive risk on benign "montage/slot system" prose).
- [ ] DEFERRED (requires Editor/PIE session with a reference machine, not available this session) — GIVEN N>=100 actor provisional benchmark is run on the development reference machine, THEN results are recorded as baseline evidence before Production gate. Pickup recipe recorded in the evidence doc — no numbers fabricated.

---

## Implementation Notes

- Keep trace scope names exactly as ADR-0009 defines them so QA evidence and future reports can search consistently.
- Static checks may be scripts, automation tests, or a repo-consistent hybrid, but they must be runnable without the Unreal Editor when feasible.
- If UE5.8 header verification changes trace macro usage, update this story evidence and ADR notes before closing.

---

## Out of Scope

- Production hardware pass/fail threshold finalization.
- Feature-layer Combo/Tension tick ordering.
- Camera or HUD tracing.

---

## QA Test Cases

- **AC-1**: Input trace scope
  - Given: tracing is enabled in a Development build.
  - When: movement input fires.
  - Then: `MovementInputTrace.InputTriggered` appears at the callback start.
  - Edge cases: held input, released/restarted input.

- **AC-2**: Velocity update trace scope
  - Given: movement input changes target velocity.
  - When: the CMC updates velocity.
  - Then: `MovementInputTrace.VelocityUpdated` appears on the first updated frame.
  - Edge cases: zero input, diagonal input, blocked movement.

- **AC-3**: Static regression suite
  - Given: known forbidden strings are injected in a temporary fixture or checked against fixtures.
  - When: the static suite runs.
  - Then: each forbidden pattern is detected and real source remains clean.
  - Edge cases: comments, tests, generated files.

- **AC-4**: Provisional movement benchmark evidence
  - Given: N>=100 actors on the development reference machine.
  - When: 300 frames are captured.
  - Then: movement tick timing is recorded as baseline evidence.
  - Edge cases: benchmark is evidence-only until target minimum hardware is final.

---

## Test Evidence

**Story Type**: Integration  
**Required evidence**:
- `tests/integration/movement/movement_traceability_test.*` or equivalent automation evidence
- `tests/static/movement_regression_checks.*`
- `production/qa/evidence/movement-traceability-and-static-regression-checks-evidence.md`

**Status**: [x] Created — all three present

**Verification (2026-07-27)**:
- `tests/integration/movement/movement_traceability_test.ps1` — PASS (source-level; explicitly documented as not a live Insights capture)
- `tests/static/movement_regression_checks.ps1` — PASS (consolidates/delegates to `movement_foundation_contract.ps1`, adds new whole-file Time Dilation, montage input-lock, and root-motion checks, all self-tested against known-bad fixtures first)
- `production/qa/evidence/movement-traceability-and-static-regression-checks-evidence.md` — created, documents macro-syntax verification (real UE5.8 header read, cited Epic precedent), placement rationale, and deferred items with pickup checklists
- Regression re-run: `camera_yaw_facing_test.ps1`, `movement_foundation_contract.ps1`, `movement_independence_check.ps1`, `airborne_and_grace_windows_test.ps1`, `movement_lock_contract_test.ps1`, `hitstop_no_time_dilation_check.ps1`, `tuning_clamp_and_joint_bound_test.ps1` — all PASS
- UBT full build (`MoonEditor Win64 Development`) — **Succeeded**, 4/4 actions

**Trace macro syntax resolved (not left open)**: bare-token `TRACE_CPUPROFILER_EVENT_SCOPE(MovementInputTrace.X)` confirmed correct against the real installed UE5.8 header (`ProfilingDebugging/CpuProfilerTrace.h`) — using it with a quoted string literal instead (which ADR-0009's own diagram text shows) would double-stringify and silently break the scope name; flagged as an ADR documentation discrepancy to correct separately, not touched here (out of this story's write boundary).

**Deferred (explicitly, not silently skipped)**: AC-4's live benchmark and the actual `.uasset`-level `bEnableRootMotion` import-setting verification (TR-mov-010) both require an Editor/PIE session not available this session. Pickup recipes recorded in the evidence doc.

---

## Dependencies

- Depends on: Stories 001, 003, and 004 (Story 001 status: In Progress — PIE verification still pending; user accepted this risk earlier in this session and it carries forward)
- Unlocks: Production gate movement evidence and repeatable playtest instrumentation

## Completion Notes
**Completed**: 2026-08-12
**Criteria**: 3/4 passing (AC-4 DEFERRED — needs reference-machine PIE session, not available this session)
**Deviations**: ADVISORY — ADR-0009 diagram shows quoted-string trace macro form; code correctly uses bare-token form per real UE5.8 header. Already documented in story body, tracked as separate ADR-doc fix, not this story's scope.
**Test Evidence**: Integration — `tests/integration/movement/movement_traceability_test.ps1` + `tests/static/movement_regression_checks.ps1`, both live-verified PASS this session; evidence doc at `production/qa/evidence/movement-traceability-and-static-regression-checks-evidence.md`
**Code Review**: Skipped — solo mode; movement code already covered by code review in commit 1561ecf
