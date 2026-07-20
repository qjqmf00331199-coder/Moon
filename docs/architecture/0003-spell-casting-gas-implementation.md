# ADR 0003: Spell Casting (base) GAS Implementation

**Status:** Accepted  
**Date:** 2026-07-18  
**Accepted:** 2026-07-20 (Blackhole spell slice compiled and placed in PIE test rig; base class pattern verified in-engine)  
**Context:** Moon Fragment Hunt (UE 5.8)

---

## Engine Compatibility

| Field | Value |
|-------|-------|
| **Engine** | Unreal Engine 5.8 |
| **Domain** | Gameplay — GAS Ability, Enhanced Input |
| **Knowledge Risk** | HIGH — `UGameplayAbility` `ActivateAbility`/`CommitAbility` override contract, `InstancedPerActor` behavior, and input-binding patterns all touch GAS internals that had 5.3–5.8 evolution. |
| **References Consulted** | `docs/engine-reference/unreal/VERSION.md`, `deprecated-apis.md` (grepped for GameplayAbility/CommitAbility — no new deprecations in 5.8 range for this path), `current-best-practices.md` |
| **Post-Cutoff APIs Used** | `UGameplayAbility::CanActivateAbility`, `CommitAbility` — stable pre-5.0 APIs. `InstancedPerActor` instancing policy — stable. |
| **Verification Required** | (1) Confirm `InstancedPerActor` with same-frame activation does not introduce latency issues in UE 5.8's ability task scheduling. (2) Verify `SetupPlayerInputComponent` Enhanced Input → GAS ability activation binding pattern is not superseded by a new 5.8 input routing convention (check `current-best-practices.md` section on Enhanced Input + GAS). |

---

## ADR Dependencies

| Field | Value |
|-------|-------|
| **Depends On** | ADR-0001 (Player Movement and GAS Core) — needs `UMoonAbilitySystemComponent`, `UMoonAttributeSet` (Mana attributes), and the Enhanced Input context established there |
| **Required By** | Luna Overdrive implementation (CostBypass.Active tag contract), Combo/Tension Gauge (OnSpellHit delegate), Combat HUD (cooldown/mana state display) |
| **Enables** | The core spell-weave combat loop; all higher systems that react to spell events |
| **Blocks** | Luna Overdrive implementation cannot start without the CostBypass.Active bypass gate established here |

---

## Context

We need to implement the Spell Casting (base) system according to `design/gdd/spell-casting-base.md`. The core constraints are:

1. Three fixed spells (Blackhole, Fire, Lightning) — each with its own cost and cooldown.
2. Instant-cast (no forced recovery/delays), allowing free movement during casts.
3. Cost (Mana) and Cooldown gating, both bypassable via `CostBypass.Active`.
4. Support for `CostBypass.Active` (granted by Luna Overdrive) to completely skip cost and cooldown checks.
5. Per-frame cast-rate floor (per element) and a global `MaxCastsPerSecond` cap (20/s, registered in `entities.yaml`) to prevent macro/turbo abuse even when all gates are bypassed.

---

## Decision

1. **Ability Class Structure**:  
   A base `UMoonGameplayAbility_Spell` inherits from `UGameplayAbility`. This base class handles the generic cost, cooldown, and bypass logic. The three spells are either Blueprint instantiations of this base or thin C++ subclasses (data-driven spell stats via `UMoonSpellData` DataAsset).

2. **Cost and Cooldown Bypass**:  
   Override `CanActivateAbility` to check for `CostBypass.Active` tag presence on the caster's ASC. If present, skip the standard GAS cost and cooldown GE evaluation. Override `CommitAbility` (or implement in `CommitExecute`) to conditionally skip applying the Cost and Cooldown `GameplayEffect`s. This avoids reinventing the entire GAS gate — only the bypass path deviates from standard GAS behavior.

3. **Instancing Policy**: `InstancedPerActor`  
   Abilities complete within the same frame for the mechanical outcome (damage, mana deduction, cooldown start) or persist solely for presentation (montage playback) without blocking new ability activations by not applying blocking gameplay tags.

4. **Input Binding**:  
   Map Enhanced Input actions (`IA_Spell_Blackhole`, `IA_Spell_Fire`, `IA_Spell_Lightning`) directly to GAS ability activations in `SetupPlayerInputComponent` or via a central input router in `AMoonCharacterBase`.

5. **Cast Rate Limiter**:  
   A lightweight per-actor per-element frame counter and a rolling 1-second window cast counter guard the `MaxCastsPerSecond` cap. Checked in `CanActivateAbility` before the bypass gate — the rate limiter applies even when `CostBypass.Active` is set (per `spell-casting-base.md` Core Rule 11, `luna-overdrive.md` BLOCKING cross-dependency resolution).

6. **Event Exposure**:  
   `OnSpellCast(Element)` and `OnSpellHit(Element, Target)` delegates exposed from `AMoonCharacterBase` or a thin spell event router component, for Combo/Tension Gauge and Combat HUD consumption.

---

## Alternatives Considered

### Alternative A: Pure data-driven GAS without C++ override
- **Pros**: Fully data-driven — cost/cooldown handled by standard GAS GE application with no C++ bypass logic.
- **Cons**: `CostBypass.Active` requires a conditional GE that modifies cost to 0 when a tag is present. This is achievable with GE magnitude calculation classes, but results in multiple GEs needing to be kept in sync with the bypass state. Cleaner in C++ with an explicit bypass check.
- **Rejection Reason**: The dual-bypass (cost AND cooldown) is simpler and more maintainable as an explicit C++ check than two separate conditional magnitude GEs.

### Alternative B: Separate ability classes per spell, no shared base
- **Pros**: Maximum Blueprint flexibility — each spell is independently tweakable.
- **Cons**: Duplicates bypass logic, cast-rate-limiter, and event exposure across three classes. Any future changes to the bypass mechanic (e.g., adding Luna Overdrive Phase 2 effects) require touching all three.
- **Rejection Reason**: DRY violation; shared base is strictly better for maintainability.

---

## Consequences

### Positive
- Complies strictly with the instant-cast and bypass rules of the GDD.
- Base class is the single place to modify the bypass contract (e.g., when Luna Overdrive evolves).
- Cast-rate limiter lives in one place — cannot be bypassed by switching to a different spell mid-frame.

### Negative
- Overriding standard GAS cost/cooldown application means we deviate slightly from pure data-driven standard GE application, requiring C++ maintenance of this bypass logic.
- Future changes to GAS's `CanActivateAbility` contract in UE engine updates require review of this override.

### Risks
- **Same-frame ordering with CostBypass**: Luna Overdrive grants `CostBypass.Active` and this ability reads it in the same frame. Order: Luna Overdrive's tag grant must land before spell activation check. Mitigation: Luna Overdrive processes via `OnOverdriveTriggered` which fires at end of Combo/Tension frame — all three events (trigger, tag grant, cast gate) should be naturally ordered, but PIE verification required.

---

## GDD Requirements Addressed

| GDD System | TR-ID | Requirement | How This ADR Addresses It |
|------------|-------|-------------|---------------------------|
| spell-casting-base.md | TR-spell-001 | GAS pipeline, InstancedPerActor, same-frame | `UMoonGameplayAbility_Spell` base with `InstancedPerActor` |
| spell-casting-base.md | TR-spell-002 | Never MovementLocked, 100% movement during cast | No blocking tags applied; CMC not paused |
| spell-casting-base.md | TR-spell-003 | Per-element cooldowns, shared Mana | Per-element cooldown GE tag; shared `Mana` attribute from ADR-0001 |
| spell-casting-base.md | TR-spell-004 | CostBypass.Active checked in both gates | Checked in `CanActivateAbility` (cost gate) and `CommitAbility` (cooldown gate) |
| spell-casting-base.md | TR-spell-005 | Cast-rate limiter (per-frame + MaxCastsPerSecond) | Per-actor rolling counter in base class `CanActivateAbility` |
| spell-casting-base.md | TR-spell-007 | MakeNoise per impact + route via ApplyDamage | Impact event triggers `MakeNoise` after `ApplyDamage` call (partial — exact routing TBD) |
| spell-casting-base.md | TR-spell-009 | Deterministic same-frame tag→gate ordering | Bypass tag read before gate evaluation, ordering enforced by call-site structure |

---

## Implementation Status

As of 2026-07-20:
- ✅ `UMoonGameplayAbility_Spell` base class — skeleton implemented
- ✅ `GA_Blackhole` (Blackhole spell ability) — compiled, placed in test rig `L_CombatTest`
- ⚠️ `GA_Fire`, `GA_Lightning` — not yet implemented (Blackhole slice first)
- ⚠️ Cast-rate limiter — scaffolded, PIE verification pending
- ❌ `UMoonSpellData` DataAsset — not yet created (stats currently hardcoded as placeholder)
- ❌ `OnSpellCast`/`OnSpellHit` event exposure — not yet wired to HUD/Tension consumers

---

## Related Decisions
- ADR-0001 (Player Movement and GAS Core) — foundation dependency
- ADR-0002 (Checkpoint Persistence) — sibling Foundation ADR
- `design/gdd/spell-casting-base.md` — primary GDD source
- `design/gdd/luna-overdrive.md` — CostBypass.Active consumer contract
- `design/gdd/combo-tension-gauge.md` — OnSpellHit consumer
