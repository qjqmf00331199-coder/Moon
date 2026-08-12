# Story 003: Airborne Substate, Jump Buffer, and Coyote Runtime

> **Epic**: Player Movement Foundation Fixes
> **Status**: Complete
> **Layer**: Foundation
> **Type**: Logic
> **Estimate**: 2-4 hours
> **Manifest Version**: 2026-07-27
> **Last Updated**: 2026-08-12

## Context

**GDD**: `design/gdd/player-movement.md`  
**Requirement**: `TR-mov-003`, `TR-mov-006`, `TR-mov-007`

**ADR Governing Implementation**: ADR-0009: Player Movement Runtime Contract  
**ADR Decision Summary**: Airborne substates are derived from `Velocity.Z` after CMC movement, not custom movement modes. Jump buffer and coyote time are character-owned float timers with inclusive 150ms boundaries. `MovementLocked` is a private reservation for a future Status Effect ADR.

**Engine**: Unreal Engine 5.8 | **Risk**: HIGH  
**Engine Notes**: This story avoids post-cutoff custom movement-mode risk by not introducing custom movement modes.

**Control Manifest Rules (this layer)**:
- Required: derive Ascending/Falling after CMC tick.
- Forbidden: custom movement modes for Ascending/Falling in this scope.
- Guardrail: timer boundaries must be unit-testable by direct value injection.

---

## Acceptance Criteria

- [x] GIVEN Falling state and an external Z impulse makes `Velocity.Z > 0`, WHEN the next post-CMC tick runs, THEN airborne substate is Ascending. — `AMoonCharacterBase::Tick()`, derived after `Super::Tick()`; covered by `airborne_and_grace_windows_test.ps1`.
- [x] GIVEN coyote timer values 149ms, 150ms, 150.5ms, and 151ms, WHEN jump input is evaluated, THEN 149ms and 150ms pass, while 150.5ms and 151ms fail. — `IsWithinGraceWindow()` inclusive `[0.0f, 0.150f]` check against the remaining-timer value; covered by `airborne_and_grace_windows_test.ps1` (elapsed→remaining conversion, exact-150ms-boundary regression test included).
- [x] GIVEN jump buffer timer values 149ms, 150ms, 150.5ms, and 151ms before landing, WHEN landing occurs, THEN 149ms and 150ms trigger the buffered jump, while later values do not. — same `IsWithinGraceWindow()` boundary, consumed in `Landed()`.
- [x] GIVEN `MovementLocked=true`, WHEN movement input occurs, THEN movement is ignored; GIVEN Spell Casting attempts to write the flag, THEN no write path is available. — `Move()` gates on `bMovementLocked`; `SetMovementLocked()` private/uncalled; covered by `movement_lock_contract_test.ps1`.

---

## Implementation Notes

- Place airborne substate derivation after `Super::Tick()` so the current frame's CMC result is visible.
- Implement tests by directly injecting timer values rather than approximating through frame simulation.
- Keep `SetMovementLocked(bool)` private and uncalled until a Status Effect ADR defines a legal writer.
- Avoid `SetMovementModeWithCustomMode()` here; this story does not need it.

---

## Out of Scope

- Dash's deprecated movement-mode call is handled by the Dash/Evasion epic.
- Movement tuning clamps are Story 002.
- Hitstop is Story 004.

---

## QA Test Cases

- **AC-1**: Falling to Ascending re-entry
  - Given: character is falling.
  - When: a test hook sets velocity Z positive before the next movement result is sampled.
  - Then: `GetAirborneSubState()` reports Ascending after the tick.
  - Edge cases: ceiling bump to zero or negative velocity should report Falling.

- **AC-2**: Coyote time boundary
  - Given: coyote timer is directly set to 149ms, 150ms, 150.5ms, and 151ms.
  - When: jump input is evaluated.
  - Then: 149ms/150ms execute; 150.5ms/151ms do not.
  - Edge cases: floating-point comparisons near 150ms.

- **AC-3**: Jump buffer boundary
  - Given: jump buffer timer is directly set to the same boundary values before landing.
  - When: landing is processed.
  - Then: only values within `<=150ms` execute the buffered jump.
  - Edge cases: simultaneous landing and jump input.

- **AC-4**: Movement lock ownership
  - Given: a test-only Status Effect role toggles movement lock.
  - When: movement input occurs.
  - Then: movement is blocked.
  - Edge cases: Spell Casting role cannot call any public setter.

---

## Test Evidence

**Story Type**: Logic  
**Required evidence**:
- `tests/unit/movement/airborne_and_grace_windows_test.*`
- `tests/unit/movement/movement_lock_contract_test.*`

**Status**: [x] Created — both files present, PASS

**Verification (2026-07-27)**:
- `tests/unit/movement/airborne_and_grace_windows_test.ps1` — PASS
- `tests/unit/movement/movement_lock_contract_test.ps1` — PASS
- Regression re-run: `camera_yaw_facing_test.ps1`, `movement_foundation_contract.ps1`, `movement_independence_check.ps1` — all PASS
- UBT full build (`MoonEditor Win64 Development`) — **Succeeded**, 5/5 actions
- PIE runtime verification — **not yet performed** (no Unreal MCP/Editor control tool available this session)

**Bug found and fixed during implementation**: initial boundary check used `Timer > 0.0f`, which silently rejected the exact-150ms-elapsed case (remaining timer value `0.0f`) that AC-2/AC-3 require to pass. Fixed by introducing a distinct `UnarmedTimerSentinel = -1.0f` and changing the check to inclusive `>= 0.0f`. Caught before acceptance by manual trace of the diff, not by the first self-reported test pass — the original test mirrored the buggy formula rather than exercising real elapsed→remaining semantics.

---

## Dependencies

- Depends on: Story 001 (status: Complete, 4/4 AC passing as of 2026-08-12)
- Unlocks: Dash/Evasion air-dash and MovementLocked dash gating work

## Completion Notes
**Completed**: 2026-08-12
**Criteria**: 4/4 passing
**Deviations**: None
**Test Evidence**: Logic — `tests/unit/movement/airborne_and_grace_windows_test.ps1`, `tests/unit/movement/movement_lock_contract_test.ps1`, both re-verified PASS
**Code Review**: Complete (combined Stories 001-005 review, commit `1561ecf`)
