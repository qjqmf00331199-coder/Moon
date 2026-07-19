# Architecture Traceability Index

- **Last Updated:** 2026-07-18
- **Engine:** Unreal Engine 5.8
- **Source review:** [architecture-review-2026-07-18.md](architecture-review-2026-07-18.md)

## Coverage Summary

- Total requirements: 74
- ✅ Covered: 9 (12%)
- ⚠️ Partial: 6 (8%)
- ❌ Gaps: 59 (80%)

## Full Matrix

| TR-ID | GDD | System | Requirement (short) | ADR | Status |
|-------|-----|--------|---------------------|-----|--------|
| TR-mov-001 | player-movement | Player Movement | CMC + camera-relative input + facing decouple | ADR-0001 | ⚠️ |
| TR-mov-002 | player-movement | Player Movement | Module compile-independent of SpellCasting | — | ❌ |
| TR-mov-003 | player-movement | Player Movement | Airborne substate via Velocity.Z sign | — | ❌ |
| TR-mov-004 | player-movement | Player Movement | Data-driven tuning + hard clamps + joint bound | ADR-0001 | ⚠️ |
| TR-mov-005 | player-movement | Player Movement | External velocity/Z-launch injection API | — | ❌ |
| TR-mov-006 | player-movement | Player Movement | MovementLocked access restricted to Status Effect | — | ❌ |
| TR-mov-007 | player-movement | Player Movement | Jump buffer / coyote timers (delta-time) | — | ❌ |
| TR-mov-008 | player-movement | Player Movement | Hitstop/execution = no Time Dilation | — | ❌ |
| TR-mov-009 | player-movement | Player Movement | Input→velocity latency budget ≤33ms p95 | — | ❌ |
| TR-mov-010 | player-movement | Player Movement | Non-root-motion locomotion anims | — | ❌ |
| TR-hp-001 | health-damage-core | Health/Damage Core | GAS ASC+AttributeSet, single ApplyDamage | ADR-0001 | ✅ |
| TR-hp-002 | health-damage-core | Health/Damage Core | Shield/armor intercept + bBypassDefense | ADR-0001 | ✅ |
| TR-hp-003 | health-damage-core | Health/Damage Core | State.Invulnerable i-frame gating | ADR-0001 | ✅ |
| TR-hp-004 | health-damage-core | Health/Damage Core | Ref-counted overlay tags, auto-clear on death | ADR-0001 | ⚠️ |
| TR-hp-005 | health-damage-core | Health/Damage Core | State.Executable + TryExecute API | ADR-0001 | ⚠️ |
| TR-hp-006 | health-damage-core | Health/Damage Core | Immediate death detection | — | ❌ |
| TR-hp-007 | health-damage-core | Health/Damage Core | Event exposure (OnDeath/OnExecuted/…) | — | ❌ |
| TR-hp-008 | health-damage-core | Health/Damage Core | Runtime MaxHealth reclamp (absolute) | — | ❌ |
| TR-hp-009 | health-damage-core | Health/Damage Core | Death = instant checkpoint respawn | ADR-0002-checkpoint | ✅ |
| TR-cam-001 | camera-system-base | Camera System | SpringArm→Camera hierarchy, controller-driven | — | ❌ |
| TR-cam-002 | camera-system-base | Camera System | IA_Look routing + pitch clamp | — | ❌ |
| TR-cam-003 | camera-system-base | Camera System | Camera-relative movement basis | — | ❌ |
| TR-cam-004 | camera-system-base | Camera System | Camera lag + max-distance cap | — | ❌ |
| TR-cam-005 | camera-system-base | Camera System | Collision test + separate destructible channel | — | ❌ |
| TR-cam-006 | camera-system-base | Camera System | Overdrive FOV / execution blend overlay | — | ❌ |
| TR-cam-007 | camera-system-base | Camera System | ResetCameraLag() on teleport/checkpoint | — | ❌ |
| TR-cam-008 | camera-system-base | Camera System | Look-input suppression + shake caps | — | ❌ |
| TR-cam-009 | camera-system-base | Camera System | All camera params data-asset driven | — | ❌ |
| TR-ai-001 | enemy-ai-base | Enemy AI | AIController+BT, 6-state machine | — | ❌ |
| TR-ai-002 | enemy-ai-base | Enemy AI | AIPerception Sight+Hearing (open MakeNoise) | — | ❌ |
| TR-ai-003 | enemy-ai-base | Enemy AI | Telegraph/Commit delegates, commit-frame damage | — | ❌ |
| TR-ai-004 | enemy-ai-base | Enemy AI | Consume HDC OnDeath as sole Dead transition | — | ❌ |
| TR-ai-005 | enemy-ai-base | Enemy AI | TriggerStagger/ClearStagger hooks | — | ❌ |
| TR-ai-006 | enemy-ai-base | Enemy AI | Archetype tags + read-only overlays | — | ❌ |
| TR-ai-007 | enemy-ai-base | Enemy AI | NavMesh pursuit + pathfind-fail timeout | — | ❌ |
| TR-ai-008 | enemy-ai-base | Enemy AI | Swarm within 60fps budget (unverified) | — | ❌ |
| TR-spell-001 | spell-casting-base | Spell Casting | GAS pipeline, InstancedPerActor, same-frame | ADR-0002-spell | ✅ |
| TR-spell-002 | spell-casting-base | Spell Casting | Never MovementLocked during cast | ADR-0002-spell | ✅ |
| TR-spell-003 | spell-casting-base | Spell Casting | Per-element cooldowns, shared Mana | ADR-0002-spell | ✅ |
| TR-spell-004 | spell-casting-base | Spell Casting | CostBypass.Active both gates | ADR-0002-spell | ✅ |
| TR-spell-005 | spell-casting-base | Spell Casting | Cast-rate limiter | ADR-0002-spell | ✅ |
| TR-spell-006 | spell-casting-base | Spell Casting | OnExecuted mana snap + regen + clamp | — | ❌ |
| TR-spell-007 | spell-casting-base | Spell Casting | MakeNoise + route via ApplyDamage | ADR-0002-spell | ⚠️ |
| TR-spell-008 | spell-casting-base | Spell Casting | Expose cast/hit events for HUD | — | ❌ |
| TR-spell-009 | spell-casting-base | Spell Casting | Deterministic same-frame tag→gate ordering | ADR-0002-spell | ⚠️ |
| TR-dash-001 | dash-evasion | Dash/Evasion | Fractional charge accumulation | — | ❌ |
| TR-dash-002 | dash-evasion | Dash/Evasion | Velocity Override via Movement API | — | ❌ |
| TR-dash-003 | dash-evasion | Dash/Evasion | Grant/remove State.Invulnerable i-frames | — | ❌ |
| TR-dash-004 | dash-evasion | Dash/Evasion | Just-Dodge window + spatial test | — | ❌ |
| TR-dash-005 | dash-evasion | Dash/Evasion | Grant State.Executable, cap 1 refund | — | ❌ |
| TR-dash-006 | dash-evasion | Dash/Evasion | Camera-relative dir + shake trigger | — | ❌ |
| TR-dash-007 | dash-evasion | Dash/Evasion | Respect MovementLocked, fire mid-cast | — | ❌ |
| TR-dash-008 | dash-evasion | Dash/Evasion | Expose charge/cooldown to HUD | — | ❌ |
| TR-tension-001 | combo-tension-gauge | Combo/Tension | GAS attribute, event-driven only | — | ❌ |
| TR-tension-002 | combo-tension-gauge | Combo/Tension | Read-only subscribe upstream delegates | — | ❌ |
| TR-tension-003 | combo-tension-gauge | Combo/Tension | Decay timer w/ grace, ticks during Invuln | — | ❌ |
| TR-tension-004 | combo-tension-gauge | Combo/Tension | Gain→Penalty→Decay ordering + clamp | — | ❌ |
| TR-tension-005 | combo-tension-gauge | Combo/Tension | OnOverdriveTriggered once-per-frame at Max | — | ❌ |
| TR-tension-006 | combo-tension-gauge | Combo/Tension | Read CostBypass.Active to gate multiplier | — | ❌ |
| TR-tension-007 | combo-tension-gauge | Combo/Tension | Expose read-only value + state to HUD | — | ❌ |
| TR-overdrive-001 | luna-overdrive | Luna Overdrive | Subscribe OnOverdriveTriggered, enter Active | — | ❌ |
| TR-overdrive-002 | luna-overdrive | Luna Overdrive | SetLooseGameplayTagCount(1/0) sole grantor | — | ❌ |
| TR-overdrive-003 | luna-overdrive | Luna Overdrive | Timer-variable duration | — | ❌ |
| TR-overdrive-004 | luna-overdrive | Luna Overdrive | Lazy CurrentTime>=EndTime evaluation | — | ❌ |
| TR-overdrive-005 | luna-overdrive | Luna Overdrive | Same-frame race determinism | — | ❌ |
| TR-overdrive-006 | luna-overdrive | Luna Overdrive | Death cancels timer + clears tag, idempotent | — | ❌ |
| TR-overdrive-007 | luna-overdrive | Luna Overdrive | Expose Started/Ended(EndReason) + TimeRemaining | — | ❌ |
| TR-hud-001 | combat-hud | Combat HUD | UMG+CommonUI read-only, non-focusable | — | ❌ |
| TR-hud-002 | combat-hud | Combat HUD | Bind only to existing upstream interfaces | — | ❌ |
| TR-hud-003 | combat-hud | Combat HUD | Event-driven, no idle tick, 60fps budget | — | ❌ |
| TR-hud-004 | combat-hud | Combat HUD | Coalesce per-frame updates, last-value-wins | — | ❌ |
| TR-hud-005 | combat-hud | Combat HUD | Signals off real values, not interpolated | — | ❌ |
| TR-hud-006 | combat-hud | Combat HUD | Mirror upstream death/overdrive transitions | — | ❌ |
| TR-hud-007 | combat-hud | Combat HUD | Device-detect glyph swap (5.8 unified Input) | — | ❌ |

## Known Gaps

### Core layer (highest priority — cause of FAIL verdict)

- **Camera System** (TR-cam-001..009) → `/architecture-decision Camera System`
- **Enemy AI** (TR-ai-001..008) → `/architecture-decision Enemy AI` (swarm perf unverified)
- **Dash/Evasion** (TR-dash-001..008) → `/architecture-decision Dash/Evasion`

### Foundation layer (partial — close remaining gaps)

- Player Movement: TR-mov-002/003/005/006/007/008/009/010 (ADR-0001 is GAS-foundation-only despite title)
- Health/Damage Core: TR-hp-006/007/008 (death detection, event exposure, MaxHealth reclamp — TR-hp-006/007 also block ADR-0002-checkpoint)

### Feature / Presentation layer (lower priority)

- Combo/Tension Gauge (TR-tension-001..007) → `/architecture-decision Combo/Tension Gauge`
- Luna Overdrive (TR-overdrive-001..007) → `/architecture-decision Luna Overdrive` (verify `SetLooseGameplayTagCount` first)
- Combat HUD (TR-hud-001..007) → `/architecture-decision Combat HUD`
- Spell Casting residual: TR-spell-006/008 (mana regen/OnExecuted snap, HUD event exposure)

## Superseded Requirements

None — first registry population (version 2).
