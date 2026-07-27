# Story 002: Data-Driven Movement Tuning and Clamp Enforcement

> **Epic**: Player Movement Foundation Fixes
> **Status**: Ready
> **Layer**: Foundation
> **Type**: Config/Data
> **Estimate**: 2-4 hours
> **Manifest Version**: 2026-07-27
> **Last Updated**:

## Context

**GDD**: `design/gdd/player-movement.md`  
**Requirement**: `TR-mov-004`, `TR-mov-005`

**ADR Governing Implementation**: ADR-0001: Player Movement and GAS Core Foundation  
**ADR Decision Summary**: Movement tuning values must be data-driven and clamped. External systems need a safe Movement-owned path for Z impulse / velocity injection without owning movement internals.

**Engine**: Unreal Engine 5.8 | **Risk**: HIGH  
**Engine Notes**: CMC tuning properties are stable, but any new external velocity path must be checked against UE5.8 headers and project engine notes.

**Control Manifest Rules (this layer)**:
- Required: gameplay values with GDD tuning knobs must be data-driven.
- Forbidden: hardcoded movement tuning literals as runtime source of truth.
- Guardrail: AirTime joint bound must be enforced, not only documented.

---

## Acceptance Criteria

- [ ] GIVEN movement data has invalid zero/negative `MaxWalkSpeed`, `JumpZVelocity`, `GravityScale`, `MaxAcceleration`, `BrakingDecelerationWalking`, or `GroundFriction`, WHEN loaded or written at runtime, THEN values are clamped and warning logs are emitted.
- [ ] GIVEN `JumpZVelocity` and `GravityScale` produce AirTime outside 0.5s-3.0s, WHEN validation runs, THEN the combination is rejected or reverted to the previous valid combination.
- [ ] GIVEN movement source files, WHEN static checks run, THEN GDD tuning numbers are not hardcoded as runtime source of truth.
- [ ] GIVEN a downstream system needs Z impulse, WHEN using the Movement-owned API, THEN it can inject the impulse without directly owning movement state.

---

## Implementation Notes

- Preserve existing feel defaults unless the data source already defines different approved values.
- Validate individual clamps before the AirTime joint bound.
- If a full movement settings DataAsset does not exist, introduce the smallest project-consistent data/config surface needed by this story.
- Do not implement Dash air-dash behavior here; expose or confirm the Movement-side safe hook only.

---

## Out of Scope

- Dash-specific `AirDashZImpulse` tuning belongs to the Dash/Evasion epic.
- Camera settings belong to the Camera System epic.
- Status Effect access control belongs to a future Status Effect ADR.

---

## QA Test Cases

- **AC-1**: Clamp invalid scalar values
  - Given: invalid values at load time and runtime write time.
  - When: validation runs.
  - Then: values clamp to the GDD hard minimums and log warnings.
  - Edge cases: `GravityScale=0`, `GravityScale<0`, `GroundFriction=0`.

- **AC-2**: AirTime joint bound
  - Given: `GravityScale=0.1` and `JumpZVelocity=800`, then `GravityScale=1.3` and `JumpZVelocity=100`.
  - When: AirTime is computed.
  - Then: out-of-bound combinations are rejected or reverted.
  - Edge cases: exactly 0.5s and exactly 3.0s should follow the chosen inclusive/exclusive policy documented in code.

- **AC-3**: No hardcoded tuning
  - Given: movement `.h` and `.cpp` files.
  - When: static checks search for protected tuning literal patterns.
  - Then: runtime tuning comes from data/config properties.
  - Edge cases: test constants and safe fallback defaults may be allowlisted with comments.

---

## Test Evidence

**Story Type**: Config/Data  
**Required evidence**:
- Config/Data: smoke check pass (`production/qa/smoke-movement-tuning.md`)
- Static check or unit test under `tests/`

**Status**: [ ] Not yet created

---

## Dependencies

- Depends on: Story 001 should be complete or in review
- Unlocks: Story 003 and Dash/Evasion air-dash work
