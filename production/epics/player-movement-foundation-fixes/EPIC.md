# Epic: Player Movement Foundation Fixes

> **Layer**: Foundation
> **GDD**: `design/gdd/player-movement.md`
> **Architecture Module**: Player Movement (`AMoonCharacterBase`, CMC-facing runtime contract)
> **Status**: Ready
> **Stories**: Created 2026-07-27

## Overview

Bring the shipped movement foundation back into alignment with the Approved Player Movement contract before playtest and combat-system implementation depend on it. This epic fixes the camera-yaw facing deviation, completes the movement runtime contract from ADR-0009, removes Time Dilation from hitstop, adds missing movement traceability, and preserves movement independence from Spell Casting and presentation systems.

## Governing ADRs

| ADR | Decision Summary | Engine Risk |
|---|---|---|
| ADR-0001: Player Movement and GAS Core Foundation | Establishes `AMoonCharacterBase`, CMC, Enhanced Input, local ASC, and base AttributeSet. | HIGH |
| ADR-0005: Camera System SpringArm | Ratifies camera-yaw facing flags and the camera/movement shared rotation contract. | LOW |
| ADR-0009: Player Movement Runtime Contract | Defines airborne substates, movement lock ownership, timer windows, hitstop replacement, and trace scopes. | HIGH |

## GDD Requirements

| TR-ID | Requirement | ADR Coverage |
|---|---|---|
| TR-mov-001 | Camera-relative CMC input and camera-yaw facing with `bUseControllerRotationYaw=true`, `bOrientRotationToMovement=false`. | ADR-0001, ADR-0005 ✅ |
| TR-mov-002 | Movement remains independent of Spell Casting at compile/static-analysis level. | ADR-0009 ✅ |
| TR-mov-003 | Ascending/Falling derived from `Velocity.Z` sign after CMC tick. | ADR-0009 ✅ |
| TR-mov-004 | Data-driven movement tuning with clamps and AirTime joint bound. | ADR-0001 ✅ |
| TR-mov-005 | External velocity/Z launch path for downstream systems. | ADR-0001 ⚠️ |
| TR-mov-006 | `MovementLocked` write access restricted to Status Effect only. | ADR-0009 ✅ |
| TR-mov-007 | Jump buffer and coyote timers are character-owned, delta-time based, inclusive at 150ms. | ADR-0009 ✅ |
| TR-mov-008 | Hitstop/execution presentation uses no Time Dilation; capture-and-blend only. | ADR-0009 ✅ |
| TR-mov-009 | Movement latency measured through Unreal Insights trace scopes. | ADR-0009 ✅ |
| TR-mov-010 | Locomotion animations are non-root-motion. | ADR-0009 ✅ |

## Stories

| # | Story | Type | Status | ADR |
|---|---|---|---|---|
| 001 | Camera-Yaw Facing and Movement Independence | Logic | Ready | ADR-0001, ADR-0005, ADR-0009 |
| 002 | Data-Driven Movement Tuning and Clamp Enforcement | Config/Data | Ready | ADR-0001 |
| 003 | Airborne Substate, Jump Buffer, and Coyote Runtime | Logic | Ready | ADR-0009 |
| 004 | Presentation-Only Hitstop Rewrite | Visual/Feel | Ready | ADR-0009, ADR-0005 |
| 005 | Movement Traceability and Static Regression Checks | Integration | Ready | ADR-0009 |

## Definition of Done

This epic is complete when:
- All five stories are implemented, reviewed, and closed via `/story-done`.
- Player Movement acceptance criteria relevant to TR-mov-001 through TR-mov-010 are verified.
- Logic and Integration stories have passing test files or static CI scripts in `tests/`.
- Visual/Feel story 004 has evidence in `production/qa/evidence/`.
- No movement/hitstop path uses `CustomTimeDilation` or global Time Dilation.

## Next Step

Start with `story-001-camera-yaw-facing-and-movement-independence.md`; it fixes the known playtest-blocking rotation flag bug before deeper movement or combat validation.
