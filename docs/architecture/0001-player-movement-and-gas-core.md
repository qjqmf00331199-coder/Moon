# ADR 0001: Player Movement and GAS Core Foundation

**Status:** Proposed
**Date:** 2026-07-18
**Context:** Moon Fragment Hunt (UE 5.8)

## Context

The game relies heavily on tight, responsive combat mechanics (Dopamine Driven Design). The core loop involves building tension via quick spell weaving, just-dodging, and entering Luna Overdrive. To support these mechanics efficiently, we need a robust foundation for Player Movement and the Gameplay Ability System (GAS). The combat systems (Health/Damage Core, Spell Casting, Dash/Evasion) are all heavily intertwined with GAS. 

Currently, the project is a Blueprint-only Unreal Engine 5.8 project, but complex GAS architectures and frame-perfect interactions (e.g., just-dodge i-frames, combo gauge thresholds) strongly benefit from a C++ foundation to avoid Blueprint spaghetti and ensure maximum performance.

## Decision

1. **Convert to a C++ Project**:
   We will initialize a primary `Moon` C++ module. This allows us to define our base Character, AbilitySystemComponent (ASC), and AttributeSets in C++.

2. **Player Character Architecture (`AMoonCharacterBase`)**:
   - The base player character will inherit from `ACharacter` and implement `IAbilitySystemInterface`.
   - It will own the `UAbilitySystemComponent` locally (Avatar = Owner) since the player character does not persist across matches/levels in a way that requires PlayerState ownership (as defined in the Health/Damage Core GDD, player death = instant checkpoint respawn).
   - The character will use **Enhanced Input** natively mapped to GAS abilities for movement and action binding.

3. **Core Attribute Set (`UMoonAttributeSet`)**:
   - We will implement a single central AttributeSet to start, covering MVP needs:
     - `Health`, `MaxHealth`
     - `Mana`, `MaxMana`, `ManaRegenRate`
     - `TensionGauge`, `TensionGaugeMax` (for the combo/overdrive mechanics)
   - These attributes will be data-driven via GE_Init (GameplayEffect) tied to an `entities.yaml` / CurveTable lookup mechanism.

4. **Damage Pipeline**:
   - We will use an Execution Calculation (`UGameplayEffectExecutionCalculation`) for the main damage pipeline (`ApplyDamage`) to handle Defense-bypass flags, i-frame evaluation (`State.Invulnerable`), and Execution states (`State.Executable`). This ensures a single entry point for all damage as mandated by the `health-damage-core.md` GDD.

## Consequences

**Positive:**
- **Performance**: High-frequency ticks and GAS tag evaluations are resolved in C++.
- **Scalability**: Keeps Blueprints clean. Blueprints will only be used for cosmetic data, VFX, and ability data definitions (subclassing `UGameplayAbility`), while the framework remains in C++.
- **Safety**: Enforcement of strict same-frame rules (e.g., Luna Overdrive release vs cast gate) is much easier in C++ than in BP event graphs.

**Negative:**
- Requires compiling via Rider/Visual Studio, creating a slightly slower iteration loop for foundational changes.
- Requires team to adhere strictly to C++ header declarations for new Attributes.

## Next Steps
- Generate Unreal project files (`.sln`) from the `.uproject`.
- Implement `AMoonCharacterBase`, `UMoonAbilitySystemComponent`, and `UMoonAttributeSet`.
- Set up the base Enhanced Input mapping context.
