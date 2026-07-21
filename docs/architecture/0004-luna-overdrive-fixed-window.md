# ADR 0004: Luna Overdrive Fixed Window and Recovery Boundary

**Status:** Proposed
**Date:** 2026-07-21
**Context:** Moon Fragment Hunt (UE 5.8)

---

## Engine Compatibility

| Field | Value |
|-------|-------|
| **Engine** | Unreal Engine 5.8 |
| **Domain** | Gameplay — GAS tags, character-owned state, UMG event consumption |
| **Knowledge Risk** | HIGH — the pinned engine post-dates the model cutoff; GAS tag semantics require local verification |
| **References Consulted** | `docs/engine-reference/unreal/VERSION.md`, `breaking-changes.md`, `deprecated-apis.md`, installed-project GDDs and ADR-0003 |
| **Post-Cutoff APIs Used** | None newly introduced; `UAbilitySystemComponent::SetLooseGameplayTagCount` must still be verified against the installed UE5.8 header before release |
| **Verification Required** | Compile against UE5.8; PIE-check tag count 1→0, exact 10-second expiry, no active/recovery tension gain, and 16:9/21:9 HUD state changes |

## ADR Dependencies

| Field | Value |
|-------|-------|
| **Depends On** | ADR-0001 (character-owned ASC/attributes), ADR-0003 (`CostBypass.Active` spell gate) |
| **Enables** | Luna Overdrive implementation, Combat HUD overdrive timer/state, signature-combat-chain spike |
| **Blocks** | Production Luna Overdrive story must not be marked Ready until this ADR is accepted and its GDD revisions pass independent review |
| **Ordering Note** | Revise Luna Overdrive, Combo/Tension, and Spell Casting GDD contracts in the same change as this ADR |

## Context

### Problem Statement

The approved design permits tension gain and duration refresh while Overdrive is active. With Blackhole granting 28 tension per target at the 0.4 multiplier, a single multi-target cast or four single-target hits can refresh the state. Passive mana regeneration also fills the resource during the free-cast window. The result has no reliable end and no material post-climax drop.

### Constraints

- Preserve the 10-second, cost-free, cooldown-free fantasy.
- `CostBypass.Active` remains single-owner and uses idempotent set/clear semantics.
- Gameplay values remain editable defaults rather than call-site literals.
- HUD remains a read-only consumer.
- The implementation must not depend on timer/input callback ordering.

### Requirements

- Active duration is fixed and cannot be extended or refreshed.
- Tension gain is rejected during Active and for a short recovery interval after expiry.
- Passive mana regeneration pauses during Active; casts still cost zero.
- A cast evaluated at or after the end time uses normal cost/cooldown rules even if tag removal occurs later in the frame.

## Decision

`AMoonCharacterBase` owns a three-state Overdrive boundary: `Inactive`, `Active`, and `Recovery`.

1. Reaching max tension calls `TriggerOverdrive()`. From `Inactive`, it records `OverdriveEndTime = CurrentTime + 10.0`, sets `CostBypass.Active` to count 1, and emits the start event once.
2. `TriggerOverdrive()` received during `Active` or `Recovery` is ignored. It never refreshes or stacks.
3. `AddTension()` returns without changing the gauge or grace timer while `Active` or `Recovery`.
4. Character mana regeneration does not tick while `Active`. Existing mana is preserved; free casts do not consume it.
5. At `CurrentTime >= OverdriveEndTime`, the character clears the tag, enters `Recovery`, records `RecoveryEndTime = CurrentTime + 1.5`, and emits the end event once.
6. Spell activation and commit both ask the character whether the bypass is temporally active. The gameplay tag alone is insufficient, preventing a free cast on the expiry frame.
7. Recovery blocks tension gain only. Normal mana regeneration, movement, dash, and paid spell casting resume immediately.

### Architecture Diagram

```text
Tension reaches Max
        |
        v
Inactive -- TriggerOverdrive --> Active (10.0 s fixed)
                                      |  tag=1, free casts
                                      |  mana regen paused
                                      |  tension/retrigger ignored
                                      v
                              Recovery (1.5 s)
                                      |  tag=0, paid casts
                                      |  mana regen resumes
                                      |  tension ignored
                                      v
                                  Inactive
```

### Key Interfaces

```cpp
void TriggerOverdrive();
void ForceEndOverdrive(EMoonOverdriveEndReason Reason);
bool IsOverdriveActive() const;
bool IsTensionGainLocked() const;
float GetOverdriveTimeRemaining() const;
```

The character exposes Blueprint-assignable started/ended events. Combat HUD and visual-state systems subscribe but cannot mutate gameplay state.

## Alternatives Considered

### Alternative A: Keep unlimited refresh with a 0.4 gain multiplier
- **Pros**: Preserves the current approved GDD and allows long catharsis chains.
- **Cons**: Multi-target Blackhole makes the duration effectively unbounded; the multiplier is a steep switch, not a useful tuning curve.
- **Rejection Reason**: It cannot guarantee the pillar's required post-climax fall.

### Alternative B: Allow execution-only extension with a total cap
- **Pros**: Rewards skilled routing and creates a controllable mastery layer.
- **Cons**: Requires the still-undesigned Execution system and adds extension-budget UI/state before the basic loop is proven.
- **Rejection Reason**: Retain as a post-spike option, not the safe MVP.

### Alternative C: Drain mana or apply cooldown debt on exit
- **Pros**: Produces a very strong mechanical crash.
- **Cons**: Adds hidden debt and makes the climax feel punitive; it also conflicts with the rule that bypassed casts never create cooldown effects.
- **Rejection Reason**: Pausing regeneration and preserving entry mana creates the drop without inventing retroactive costs.

## Consequences

### Positive
- Overdrive always ends after ten seconds.
- The player exits with the mana they had on entry, making resource pressure visible again.
- The recovery interval protects the emotional downbeat without blocking movement or casting.
- Temporal checks make expiry deterministic even if tag clearing happens later in the frame.

### Negative
- Active play no longer builds the next Overdrive.
- Long mastery-driven extensions are deferred.
- Three existing GDD contracts and traceability entries require revision.

### Risks
- A Blueprint may still grant `CostBypass.Active` directly. Mitigation: PIE inspect tag ownership and remove legacy graph logic.
- Death handling is not fully implemented in the current Health/Damage slice. Mitigation: expose `ForceEndOverdrive(PlayerDeath)` and connect it when the canonical death delegate exists.
- Tick-based state transitions could be missed if the actor stops ticking. Mitigation: character tick is already mandatory for movement/resource logic; tests call the pure time predicates directly.

## GDD Requirements Addressed

| GDD System | Requirement | How This ADR Addresses It |
|------------|-------------|--------------------------|
| `luna-overdrive.md` | Ten-second free-cast climax followed by a sharp fall | Fixed Active window, paused mana regeneration, and Recovery state |
| `combo-tension-gauge.md` | Prevent permanent Overdrive loops | Reject all tension gain during Active and Recovery |
| `spell-casting-base.md` | Deterministic bypass gate and passive mana rules | Require tag plus temporal Active state; pause regen only during Active |
| `combat-hud.md` | Read-only Overdrive state and timer | Started/ended events and remaining-time query |

## Performance Implications

- **CPU**: Constant-time comparisons in the existing character tick and spell gate; no additional world queries.
- **Memory**: Two timestamps, one enum/state byte, and two multicast delegates per player character.
- **Load Time**: None.
- **Network**: Single-player MVP; replication is out of scope.

## Migration Plan

1. Revise the three GDD contracts and acceptance criteria.
2. Add the character-owned state and events.
3. Change spell bypass checks from tag-only to character temporal state plus tag.
4. Bind HUD state/timer events.
5. Remove or neutralize any Blueprint graph that directly owns the tag.
6. Compile, run automation tests, then PIE-verify the transition and both target aspect ratios.

## Validation Criteria

- Trigger at `t=0`: Active, tag count 1, remaining time 10.0 seconds.
- Any trigger or tension event during Active: no time extension and gauge stays zero.
- Mana is unchanged by passive regeneration during Active.
- At `t=10.0`: tag count 0; same-frame casts use normal cost/cooldown.
- From `t=10.0` to `t=11.5`: tension events are ignored but mana regeneration and paid casts work.
- At `t=11.5`: tension gain resumes.
- HUD remains legible and within safe zones at 1920×1080 and 2560×1080.

## Related Decisions

- [ADR-0001](0001-player-movement-and-gas-core.md)
- [ADR-0003](0003-spell-casting-gas-implementation.md)
- [`luna-overdrive.md`](../../design/gdd/luna-overdrive.md)
- [`combo-tension-gauge.md`](../../design/gdd/combo-tension-gauge.md)
- [`spell-casting-base.md`](../../design/gdd/spell-casting-base.md)
