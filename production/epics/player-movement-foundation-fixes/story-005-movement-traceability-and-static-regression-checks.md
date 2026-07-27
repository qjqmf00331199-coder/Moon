# Story 005: Movement Traceability and Static Regression Checks

> **Epic**: Player Movement Foundation Fixes
> **Status**: Ready
> **Layer**: Foundation
> **Type**: Integration
> **Estimate**: 2-4 hours
> **Manifest Version**: 2026-07-27
> **Last Updated**:

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

- [ ] GIVEN movement input is processed, WHEN Insights tracing is enabled, THEN `MovementInputTrace.InputTriggered` is emitted at the Enhanced Input Triggered callback start.
- [ ] GIVEN CMC has updated velocity toward the requested movement, WHEN tracing is enabled, THEN `MovementInputTrace.VelocityUpdated` is emitted at the first post-CMC frame.
- [ ] GIVEN static movement checks run, THEN Spell Casting references, montage input-lock patterns, Time Dilation calls, and root-motion locomotion violations are reported as failures.
- [ ] GIVEN N>=100 actor provisional benchmark is run on the development reference machine, THEN results are recorded as baseline evidence before Production gate.

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

**Status**: [ ] Not yet created

---

## Dependencies

- Depends on: Stories 001, 003, and 004
- Unlocks: Production gate movement evidence and repeatable playtest instrumentation
