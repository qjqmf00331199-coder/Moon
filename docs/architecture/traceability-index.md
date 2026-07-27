# Architecture Traceability Index

- **Last Updated:** 2026-07-27 (v2)
- **Engine:** Unreal Engine 5.8
- **Source review:** [architecture-review-2026-07-27-v2.md](architecture-review-2026-07-27-v2.md) — verdict **CONCERNS**
- **Previous:** [architecture-review-2026-07-27.md](architecture-review-2026-07-27.md) — verdict FAIL (38 / 15 / 23)
- **Registry:** `tr-registry.yaml` v3 (unchanged this pass — no new/superseded/reworded requirements)

## Coverage Summary

| | 2026-07-18 | 2026-07-27 | 2026-07-27 (v2) |
|---|---|---|---|
| Total active requirements | 74 | 76 | **76** |
| ✅ Covered | 9 (12%) | 38 (50%) | **52 (68%)** |
| ⚠️ Partial | 6 (8%) | 15 (20%) | **11 (15%)** |
| ❌ Gap | 59 (80%) | 23 (30%) | **13 (17%)** |

10 gaps closed by ADR-0008 (Health/Damage Core death contract) + ADR-0009 (Player Movement runtime
contract). 4 partials upgraded to covered — 2 by ADR-0008 closing ambiguity ADR-0001 left open
(TR-hp-004/005), 2 by C-1's resolution letting ADR-0004/0006 stop hedging on an assumed producer
(TR-ai-004, TR-overdrive-006).

### By system

| System | ✅ | ⚠️ | ❌ | Governing ADR |
|---|---|---|---|---|
| Health/Damage Core | 9 | 0 | 0 | ADR-0001, ADR-0002, ADR-0008 |
| Player Movement | 8 | 2 | 0 | ADR-0001, ADR-0005, ADR-0009 |
| Camera System (base) | 7 | 2 | 0 | ADR-0005 |
| Luna Overdrive | 7 | 1 | 0 | ADR-0004 |
| Enemy AI (base) | 6 | 2 | 0 | ADR-0006 |
| Spell Casting (base) | 7 | 3 | 0 | ADR-0003, ADR-0004 |
| Dash/Evasion | 5 | 2 | 1 | ADR-0007 |
| Combo/Tension Gauge | 1 | 1 | 5 | ADR-0001 (attribute only), ADR-0004 (lock only) |
| Combat HUD | 0 | 0 | 7 | **none** |

### ADR status snapshot

| ADR | Status | Blocked by |
|---|---|---|
| ADR-0001 Player Movement and GAS Core | Accepted | — |
| ADR-0002 Checkpoint Persistence | Accepted | — (C-1 resolved, `ResetDeathState()` wired) |
| ADR-0003 Spell Casting GAS | Accepted | — |
| ADR-0004 Luna Overdrive Fixed Window | **Accepted** (2026-07-27) | — |
| ADR-0005 Camera System (SpringArm) | Accepted (2026-07-27) | — |
| ADR-0006 Enemy AI Behavior Tree | **Accepted** (2026-07-27) | — |
| ADR-0007 Dash/Evasion Just-Dodge | **Accepted** (2026-07-27) | — |
| ADR-0008 Health/Damage Core Death Contract | **Accepted** (2026-07-27) | — |
| ADR-0009 Player Movement Runtime Contract | **Proposed** | none — content-complete, no unresolved dependency, just needs an Accept pass |

## Full Matrix

| TR-ID | GDD | System | Requirement (short) | ADR | Status |
|-------|-----|--------|---------------------|-----|--------|
| TR-mov-001 | player-movement | Player Movement | CMC + camera-relative input + facing decouple | ADR-0001, ADR-0005 | ✅ |
| TR-mov-002 | player-movement | Player Movement | Module compile-independent of SpellCasting | ADR-0009 | ✅ |
| TR-mov-003 | player-movement | Player Movement | Airborne substate via Velocity.Z sign | ADR-0009 | ✅ |
| TR-mov-004 | player-movement | Player Movement | Data-driven tuning + hard clamps + joint bound | ADR-0001 | ⚠️ |
| TR-mov-005 | player-movement | Player Movement | External velocity/Z-launch injection API | ADR-0007 | ⚠️ |
| TR-mov-006 | player-movement | Player Movement | MovementLocked write restricted to Status Effect | ADR-0009 | ✅ |
| TR-mov-007 | player-movement | Player Movement | Jump buffer / coyote timers (delta-time) | ADR-0009 | ✅ |
| TR-mov-008 | player-movement | Player Movement | Hitstop/execution = no Time Dilation | ADR-0009 | ✅ (V-1 resolved — CustomTimeDilation removal specified) |
| TR-mov-009 | player-movement | Player Movement | Input→velocity latency budget ≤33ms p95 | ADR-0009 | ✅ |
| TR-mov-010 | player-movement | Player Movement | Non-root-motion locomotion anims | ADR-0009 | ✅ |
| TR-hp-001 | health-damage-core | Health/Damage Core | GAS ASC+AttributeSet, single ApplyDamage | ADR-0001 | ✅ |
| TR-hp-002 | health-damage-core | Health/Damage Core | Shield/armor intercept + bBypassDefense | ADR-0001 | ✅ |
| TR-hp-003 | health-damage-core | Health/Damage Core | State.Invulnerable i-frame gating | ADR-0001 | ✅ |
| TR-hp-004 | health-damage-core | Health/Damage Core | Ref-counted overlay tags, auto-clear on death | ADR-0001, ADR-0008 | ✅ |
| TR-hp-005 | health-damage-core | Health/Damage Core | State.Executable + TryExecute API | ADR-0001, ADR-0007, ADR-0008 | ✅ |
| TR-hp-006 | health-damage-core | Health/Damage Core | Immediate death detection | ADR-0008 | ✅ |
| TR-hp-007 | health-damage-core | Health/Damage Core | Event exposure (OnDeath/OnExecuted/…) | ADR-0008 | ✅ |
| TR-hp-008 | health-damage-core | Health/Damage Core | Runtime MaxHealth reclamp (absolute) | ADR-0008 | ✅ |
| TR-hp-009 | health-damage-core | Health/Damage Core | Death = instant checkpoint respawn | ADR-0002 | ✅ |
| TR-cam-001 | camera-system-base | Camera System | SpringArm→Camera hierarchy, controller-driven | ADR-0005 | ✅ |
| TR-cam-002 | camera-system-base | Camera System | IA_Look routing + pitch clamp | ADR-0005 | ✅ |
| TR-cam-003 | camera-system-base | Camera System | Camera-relative basis, independent rotation read | ADR-0005 | ⚠️ |
| TR-cam-004 | camera-system-base | Camera System | Camera lag + 60.0uu max-distance cap | ADR-0005 | ⚠️ C-3 value conflict, still open |
| TR-cam-005 | camera-system-base | Camera System | Collision test + separate destructible channel | ADR-0005 | ✅ |
| TR-cam-006 | camera-system-base | Camera System | Overdrive FOV / execution blend overlay | ADR-0005 | ✅ |
| TR-cam-007 | camera-system-base | Camera System | ResetCameraLag() on teleport/checkpoint | ADR-0005, ADR-0002 | ✅ |
| TR-cam-008 | camera-system-base | Camera System | Look-input suppression + shake caps | ADR-0005 | ✅ |
| TR-cam-009 | camera-system-base | Camera System | All camera params data-asset driven | ADR-0005 | ✅ |
| TR-ai-001 | enemy-ai-base | Enemy AI | AIController+BT, 6-state machine | ADR-0006 | ✅ |
| TR-ai-002 | enemy-ai-base | Enemy AI | AIPerception Sight+Hearing (open MakeNoise) | ADR-0006 | ✅ |
| TR-ai-003 | enemy-ai-base | Enemy AI | Telegraph/Commit delegates, commit-frame damage | ADR-0006 | ✅ |
| TR-ai-004 | enemy-ai-base | Enemy AI | Consume HDC OnDeath as sole Dead transition | ADR-0006, ADR-0008 | ✅ (C-1 resolved) |
| TR-ai-005 | enemy-ai-base | Enemy AI | TriggerStagger/ClearStagger hooks | ADR-0006 | ✅ |
| TR-ai-006 | enemy-ai-base | Enemy AI | Archetype tags + read-only overlays | ADR-0006 | ✅ |
| TR-ai-007 | enemy-ai-base | Enemy AI | NavMesh pursuit + pathfind-fail timeout | ADR-0006 | ✅ |
| TR-ai-008 | enemy-ai-base | Enemy AI | Swarm within 60fps budget | ADR-0006 | ⚠️ explicitly deferred, unmeasured |
| TR-spell-001 | spell-casting-base | Spell Casting | GAS pipeline, InstancedPerActor, same-frame | ADR-0003 | ✅ |
| TR-spell-002 | spell-casting-base | Spell Casting | Never MovementLocked during cast | ADR-0003 | ✅ |
| TR-spell-003 | spell-casting-base | Spell Casting | Per-element cooldowns, shared Mana | ADR-0003 | ✅ |
| TR-spell-004 | spell-casting-base | Spell Casting | Bypass = tag AND time, both gates | ADR-0003, ADR-0004 | ✅ |
| TR-spell-005 | spell-casting-base | Spell Casting | Cast-rate limiter | ADR-0003 | ✅ |
| TR-spell-006 | spell-casting-base | Spell Casting | OnExecuted mana snap + regen + clamp | ADR-0004 | ⚠️ regen pause only; snap/clamp uncovered |
| TR-spell-007 | spell-casting-base | Spell Casting | MakeNoise + route via ApplyDamage | ADR-0003 | ⚠️ |
| TR-spell-008 | spell-casting-base | Spell Casting | Expose cast/hit events for HUD | ADR-0003 | ⚠️ delegates yes, HUD data surface no |
| TR-spell-009 | spell-casting-base | Spell Casting | Deterministic same-frame tag→gate ordering | ADR-0003, ADR-0004 | ✅ |
| TR-spell-010 | spell-casting-base | Spell Casting | Mana regen pauses only during Overdrive Active | ADR-0004 | ✅ |
| TR-dash-001 | dash-evasion | Dash/Evasion | Fractional charge accumulation | ADR-0007 | ✅ |
| TR-dash-002 | dash-evasion | Dash/Evasion | Instant swept position step, Override semantics | ADR-0007 | ⚠️ C-2 GDD conflict, still open |
| TR-dash-003 | dash-evasion | Dash/Evasion | Grant/remove State.Invulnerable i-frames | ADR-0007 | ✅ |
| TR-dash-004 | dash-evasion | Dash/Evasion | Just-Dodge window + spatial test | ADR-0007 | ✅ |
| TR-dash-005 | dash-evasion | Dash/Evasion | Grant State.Executable, cap 1 refund | ADR-0007 | ✅ |
| TR-dash-006 | dash-evasion | Dash/Evasion | Camera-relative dir + shake trigger | ADR-0005 | ⚠️ shake yes, input basis no |
| TR-dash-007 | dash-evasion | Dash/Evasion | Respect MovementLocked, fire mid-cast | ADR-0007 | ✅ |
| TR-dash-008 | dash-evasion | Dash/Evasion | Expose charge/cooldown to HUD | — | ❌ |
| TR-tension-001 | combo-tension-gauge | Combo/Tension | GAS attribute, event-driven only | ADR-0001 | ⚠️ attribute exists, write policy uncovered |
| TR-tension-002 | combo-tension-gauge | Combo/Tension | Read-only subscribe upstream delegates | — | ❌ |
| TR-tension-003 | combo-tension-gauge | Combo/Tension | Decay timer w/ grace, ticks during Invuln | — | ❌ |
| TR-tension-004 | combo-tension-gauge | Combo/Tension | Gain→Penalty→Decay ordering + clamp | — | ❌ |
| TR-tension-005 | combo-tension-gauge | Combo/Tension | OnOverdriveTriggered once-per-frame at Max | — | ❌ ADR-0004's upstream producer, still unspecified |
| TR-tension-007 | combo-tension-gauge | Combo/Tension | Expose read-only value + state to HUD | — | ❌ |
| TR-tension-008 | combo-tension-gauge | Combo/Tension | Full gain lock via Overdrive time-state query | ADR-0004 | ✅ |
| TR-overdrive-001 | luna-overdrive | Luna Overdrive | Subscribe OnOverdriveTriggered, enter Active | ADR-0004 | ✅ |
| TR-overdrive-002 | luna-overdrive | Luna Overdrive | SetLooseGameplayTagCount(1/0) sole grantor | ADR-0004 | ✅ |
| TR-overdrive-003 | luna-overdrive | Luna Overdrive | Timer-variable duration | ADR-0004 | ✅ |
| TR-overdrive-004 | luna-overdrive | Luna Overdrive | Lazy CurrentTime>=EndTime evaluation | ADR-0004 | ✅ |
| TR-overdrive-006 | luna-overdrive | Luna Overdrive | Death cancels timer + clears tag, idempotent | ADR-0004, ADR-0008 | ✅ (C-1 resolved) |
| TR-overdrive-007 | luna-overdrive | Luna Overdrive | Expose Started/Ended(EndReason) + TimeRemaining | ADR-0004 | ✅ |
| TR-overdrive-008 | luna-overdrive | Luna Overdrive | Inactive/Active/Recovery three-state boundary | ADR-0004 | ✅ |
| TR-overdrive-009 | luna-overdrive | Luna Overdrive | Fixed window — re-trigger ignored, no refresh | ADR-0004 | ✅ |
| TR-hud-001 | combat-hud | Combat HUD | UMG+CommonUI read-only, non-focusable | — | ❌ |
| TR-hud-002 | combat-hud | Combat HUD | Bind only to existing upstream interfaces | — | ❌ |
| TR-hud-003 | combat-hud | Combat HUD | Event-driven, no idle tick, 60fps budget | — | ❌ |
| TR-hud-004 | combat-hud | Combat HUD | Coalesce per-frame updates, last-value-wins | — | ❌ |
| TR-hud-005 | combat-hud | Combat HUD | Signals off real values, not interpolated | — | ❌ |
| TR-hud-006 | combat-hud | Combat HUD | Mirror upstream death/overdrive transitions | — | ❌ |
| TR-hud-007 | combat-hud | Combat HUD | Device-detect glyph swap (5.8 unified Input) | — | ❌ |

## Known Gaps

### 🟠 Feature layer — the only layer with gaps left

- **Combo/Tension Gauge** (TR-tension-002/003/004/005/007) → `/architecture-decision Combo/Tension Gauge`
  TR-tension-005 is the emission of the event ADR-0004 consumes — the producer is still unspecified.

### 🟡 Presentation layer

- **Combat HUD** (TR-hud-001..007) → `/architecture-decision Combat HUD` — partly reverse-documentation,
  `WBP_CombatHUD` already exists in-engine; `design/ux/combat-hud.md` now gives it a UX-level head start.
- Dash residual: TR-dash-008 (charge/cooldown HUD surface).

**Foundation layer: no gaps.** Both systems that drove the FAIL verdict (Health/Damage Core, Player
Movement) are fully covered as of ADR-0008 and ADR-0009.

## Superseded Requirements

| TR-ID | Superseded by | Date | Reason |
|---|---|---|---|
| TR-overdrive-005 | TR-overdrive-009 | 2026-07-27 | Re-trigger clause inverted — `luna-overdrive.md` Rule 5 fixed-window rewrite (2026-07-21, re-approved 2026-07-23) makes re-trigger ignored, not refreshing |
| TR-tension-006 | TR-tension-008 | 2026-07-27 | Contract inverted — `combo-tension-gauge.md` Rule 7 (2026-07-23) forbids reading `CostBypass.Active` and removes the gain multiplier entirely |

No requirements deprecated. No IDs renumbered or deleted. No registry changes this pass (ADR-0008/0009
address existing TR-IDs verbatim — no new/superseded/reworded requirements introduced).
