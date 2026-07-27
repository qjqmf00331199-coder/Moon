# Epic: Dash/Evasion Foundation Fixes

> **Layer**: Core
> **GDD**: `design/gdd/dash-evasion.md`
> **Architecture Module**: Dash/Evasion
> **Status**: Ready
> **Stories**: Not yet created - run `/create-stories dash-evasion-foundation-fixes`

## Overview

Complete the dash foundation needed for combat playtests: preserve the shipped instant displacement dash, add the missing air-dash Z impulse, replace deprecated movement-mode calls, implement Just-Dodge detection, and expose charge/cooldown state for HUD consumers without letting HUD own gameplay.

## Governing ADRs

| ADR | Decision Summary | Engine Risk |
|---|---|---|
| ADR-0007: Dash/Evasion Just-Dodge | Ratifies instant swept position step, defines air-dash Z impulse, Just-Dodge overlap query, and one-refund multi-target reward. | LOW-MEDIUM |
| ADR-0010: Combat HUD Widget Architecture | Governs dash charge/cooldown surface consumed by HUD. | MEDIUM |

## GDD Requirements

| TR-ID | Requirement | ADR Coverage |
|---|---|---|
| TR-dash-001 | Tick-accumulated fractional charge recharge. | ADR-0007 ✅ |
| TR-dash-002 | Instant swept position step, not velocity override. | ADR-0007 ✅ |
| TR-dash-003 | `State.Invulnerable` during dash i-frames. | ADR-0007 ✅ |
| TR-dash-004 | Just-Dodge timing and spatial query against telegraphed attacks. | ADR-0007 ✅ |
| TR-dash-005 | Grant `State.Executable` to all qualifying enemies, refund at most one charge. | ADR-0007 ✅ |
| TR-dash-006 | Camera-relative dash direction and camera-shake feedback. | ADR-0007, ADR-0005 ✅ |
| TR-dash-007 | Respect `MovementLocked`; dash does not interrupt casts. | ADR-0007 ✅ |
| TR-dash-008 | Expose dash charge/cooldown surface to HUD. | ADR-0010 ✅ |

## Definition of Done

This epic is complete when Dash/Evasion AC1 through AC8 pass, `CheckJustDodge()` is no longer a stub, the deprecated `SetMovementMode()` overload is gone from Dash code, and multi-enemy Just-Dodge refunds exactly one charge.

## Next Step

Run `/create-stories dash-evasion-foundation-fixes` after Player Movement story 001 and Camera-facing validation are complete.
