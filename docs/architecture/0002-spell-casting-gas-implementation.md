# ADR 0002: Spell Casting (base) GAS Implementation

## Status
Proposed

## Context
We need to implement the Spell Casting (base) system according to `design/gdd/spell-casting-base.md`. 
The core constraints are:
1. Three fixed spells (Blackhole, Fire, Lightning).
2. Instant-cast (no forced recovery/delays), allowing free movement during casts.
3. Cost (Mana) and Cooldown gating.
4. Support for `CostBypass.Active` (from Luna Overdrive) to completely skip cost and cooldown checks.
5. Per-element per-frame cast rate floor and a global MaxCastsPerSecond cap to prevent macro abuse.

## Decision
1. **Ability Class Structure**: We will create a base `UMoonGameplayAbility_Spell` that inherits from `UGameplayAbility`. This base class will handle the generic cost, cooldown, and bypass logic. The three spells will inherit from this or be Blueprint instantiations of this base class.
2. **Cost and Cooldown Bypass**: Instead of standard `CommitAbility`, we will override `CanActivateAbility` and `CommitAbility` (or just implement custom `CommitExecute`) to check for the presence of the `CostBypass.Active` tag. If present, we will skip applying the Cost and Cooldown `GameplayEffect`s.
3. **Instancing Policy**: We will use `InstancedPerActor` so that abilities can execute instantly and complete within the same frame if necessary, or persist solely for presentation logic (montage playback) without blocking new ability activations (by not applying blocking tags).
4. **Input Binding**: We will map the Enhanced Input actions (`IA_Spell_Blackhole`, etc.) directly to GAS ability activations in `SetupPlayerInputComponent` or via a central input router in `AMoonCharacterBase`.

## Consequences
- **Positive**: Complies strictly with the instant-cast and bypass rules of the GDD.
- **Negative**: Overriding standard GAS cost/cooldown application means we deviate slightly from pure data-driven standard GE application, requiring C++ maintenance of this bypass logic.
