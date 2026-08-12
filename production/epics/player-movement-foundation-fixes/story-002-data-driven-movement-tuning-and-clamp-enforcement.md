# Story 002: Data-Driven Movement Tuning and Clamp Enforcement

> **Epic**: Player Movement Foundation Fixes
> **Status**: Complete
> **Layer**: Foundation
> **Type**: Logic
> **Estimate**: 2-4 hours
> **Manifest Version**: 2026-07-27
> **Last Updated**: 2026-08-12
> **Type-Note**: Reclassified Config/Data → Logic on 2026-07-27 — acceptance criteria require real clamp-enforcement code, an AirTime joint-bound validator, and a new Movement-owned Z-impulse injection API, none of which are data-file edits. ADR-0001's actual Decision text for TR-mov-004 only says tuning is "exposed as UPROPERTY with Safe Range comments" — no clamp/joint-bound/API design is specified there, so this story also carries new-design risk beyond normal implementation.

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

- [x] GIVEN movement data has invalid zero/negative `MaxWalkSpeed`, `JumpZVelocity`, `GravityScale`, `MaxAcceleration`, `BrakingDecelerationWalking`, or `GroundFriction`, WHEN loaded or written at runtime, THEN values are clamped and warning logs are emitted. — `ValidateAndClampMovementTuning()`, called from `BeginPlay()`; exact GDD minimums (100/100/0.1/1000/1000/1); covered by `tuning_clamp_and_joint_bound_test.ps1`.
- [x] GIVEN `JumpZVelocity` and `GravityScale` produce AirTime outside 0.5s-3.0s, WHEN validation runs, THEN the combination is rejected or reverted to the previous valid combination. — revert-to-last-valid-pair policy (user decision); `ComputeAirTime()` matches GDD formula exactly (verified non-squaring); covered by the GDD's own boundary examples in the test.
- [x] GIVEN movement source files, WHEN static checks run, THEN GDD tuning numbers are not hardcoded as runtime source of truth. — clamp minimums are named `static constexpr` constants, not bare literals; covered by static grep check.
- [x] GIVEN a downstream system needs Z impulse, WHEN using the Movement-owned API, THEN it can inject the impulse without directly owning movement state. — `InjectZImpulse(float ZVelocity)`, public/`BlueprintCallable`, no caller wired up yet (correctly out of scope); covered by the test.

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

**Status**: [x] Created — both present

**Verification (2026-07-27)**:
- `tests/unit/movement/tuning_clamp_and_joint_bound_test.ps1` — PASS (includes 3 regression tests for the GravityScale fix below)
- `production/qa/smoke-movement-tuning.md` — PASS
- Regression re-run: `camera_yaw_facing_test.ps1`, `movement_foundation_contract.ps1`, `movement_independence_check.ps1`, `airborne_and_grace_windows_test.ps1`, `movement_lock_contract_test.ps1`, `hitstop_no_time_dilation_check.ps1` — all PASS
- UBT full build (`MoonEditor Win64 Development`) — **Succeeded**, 5/5 actions

**Type reclassified Config/Data → Logic (2026-07-27)**: see header `Type-Note`. Required real clamp/joint-bound/API code, not a data edit.

**Bug found and fixed during implementation review (user-approved scope extension)**: `Tick()`'s pre-existing (Story 003/004-era) asymmetric jump-feel code overwrote `GravityScale` every frame instead of multiplying the validated base value (its own comment said "multiplies," the code assigned) — silently defeating the AirTime joint-bound guarantee for the entire falling phase. Fixed with a new `BaseGravityScale` member + multiply in `Tick()` + clamping `FallingGravityScaleMultiplier` too. Default-tuning numeric output is unchanged. See `production/qa/smoke-movement-tuning.md` for full detail.

---

## Dependencies

- Depends on: Story 001 (status: In Progress — PIE verification still pending; user accepted this risk to proceed with Story 002/003/004)
- Unlocks: Story 003 and Dash/Evasion air-dash work

## Completion Notes
**Completed**: 2026-08-12
**Criteria**: 4/4 passing
**Deviations**: ADVISORY — TR-mov-005's MaxWalkSpeed-override half is intentionally out of scope (Dash/Evasion epic); ADVISORY — `Tick()` refactored (split into UpdateJumpTimers/UpdateResourceRegen/UpdateJumpFeelGravity/UpdateJumpAnimState) and debug-log noise removed during `/code-review`, spanning code from Stories 002-005 (commit `1561ecf`)
**Test Evidence**: Logic — `tests/unit/movement/tuning_clamp_and_joint_bound_test.ps1` PASS (re-verified after Tick() refactor)
**Code Review**: Complete (solo mode, `/code-review` run 2026-08-12, verdict APPROVED after fixes)
