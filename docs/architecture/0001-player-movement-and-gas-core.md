# ADR 0001: Player Movement and GAS Core Foundation

**Status:** Accepted  
**Date:** 2026-07-18  
**Accepted:** 2026-07-20 (implementation verified in C++ skeleton — `AMoonCharacterBase`, `UMoonAbilitySystemComponent`, `UMoonAttributeSet` compiled and PIE-tested)  
**Context:** Moon Fragment Hunt (UE 5.8)

---

## Engine Compatibility

| Field | Value |
|-------|-------|
| **Engine** | Unreal Engine 5.8 |
| **Domain** | Foundation — GAS Core, Character Movement |
| **Knowledge Risk** | HIGH — project pinned post-LLM-cutoff. GAS `AttributeSet` initialization APIs and `UGameplayEffectExecutionCalculation` interface had documented changes in 5.3–5.8 range. See `docs/engine-reference/unreal/deprecated-apis.md`. |
| **References Consulted** | `docs/engine-reference/unreal/VERSION.md`, `breaking-changes.md`, `deprecated-apis.md` (grepped for AttributeSet/ExecutionCalc/EnhancedInput — GAS init deprecation flagged under "5.8 Additions"), `current-best-practices.md` |
| **Post-Cutoff APIs Used** | Enhanced Input (`UEnhancedInputComponent`, `UInputMappingContext`) — stable since 5.1. GAS `UGameplayEffectExecutionCalculation` — stable pre-5.0; no known breaking changes in 5.8. |
| **Verification Required** | (1) Confirm `UMoonAttributeSet` init does NOT use the legacy `InitFromMetaDataTable` path deprecated in 5.8 — use `GetLifetimeReplicatedProps`-style or `InitializeComponent`-equivalent pattern per `deprecated-apis.md`. (2) Verify `IAbilitySystemInterface` contract has not changed for locally-owned ASC in 5.8. |

---

## ADR Dependencies

| Field | Value |
|-------|-------|
| **Depends On** | None — Foundation ADR, no upstream architectural decisions |
| **Required By** | ADR-0002 (Checkpoint Persistence — needs `UMoonAttributeSet` + `AMoonCharacterBase`), ADR-0003 (Spell Casting GAS — needs `UMoonAbilitySystemComponent` + base attribute set) |
| **Enables** | All gameplay systems (Health/Damage Core, Spell Casting, Dash/Evasion, Combo/Tension, Luna Overdrive all depend on GAS foundation established here) |
| **Blocks** | Nothing currently blocked |

---

## Context

The game relies heavily on tight, responsive combat mechanics (Dopamine Driven Design). The core loop involves building tension via quick spell weaving, just-dodging, and entering Luna Overdrive. To support these mechanics efficiently, we need a robust foundation for Player Movement and the Gameplay Ability System (GAS). The combat systems (Health/Damage Core, Spell Casting, Dash/Evasion) are all heavily intertwined with GAS.

Currently, the project is a Blueprint-only Unreal Engine 5.8 project, but complex GAS architectures and frame-perfect interactions (e.g., just-dodge i-frames, combo gauge thresholds) strongly benefit from a C++ foundation to avoid Blueprint spaghetti and ensure maximum performance.

---

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

---

## Alternatives Considered

### Alternative A: Blueprint-only GAS
- **Pros**: Faster initial iteration, no compile step for foundational changes.
- **Cons**: Blueprint GAS is notoriously difficult to debug for frame-precise timing (just-dodge window, same-frame ordering contracts). Performance overhead for high-frequency events (tension gain per spell hit, decay ticks) is significantly higher.
- **Rejection Reason**: The game's core design requires frame-perfect precision — a Blueprint-only foundation creates technical debt that would need to be resolved before any combat tuning work begins.

### Alternative B: PlayerState-owned ASC
- **Pros**: Standard multiplayer-friendly pattern; ASC survives character respawn.
- **Cons**: This project is single-player; the player character death = checkpoint respawn (not a "rejoin lobby" scenario). PlayerState-owned ASC requires replication setup that is unnecessary overhead for a single-player game.
- **Rejection Reason**: Unnecessary complexity for single-player scope; locally-owned ASC is the established single-player GAS convention and matches all current GDDs' assumptions.

---

## Consequences

### Positive
- **Performance**: High-frequency ticks and GAS tag evaluations are resolved in C++.
- **Scalability**: Keeps Blueprints clean. Blueprints will only be used for cosmetic data, VFX, and ability data definitions (subclassing `UGameplayAbility`), while the framework remains in C++.
- **Safety**: Enforcement of strict same-frame rules (e.g., Luna Overdrive release vs cast gate) is much easier in C++ than in BP event graphs.

### Negative
- Requires compiling via Rider/Visual Studio, creating a slightly slower iteration loop for foundational changes.
- Requires team to adhere strictly to C++ header declarations for new Attributes.

### Risks
- **GAS init deprecation (HIGH ENGINE RISK)**: Legacy `UAttributeSet` init functions are deprecated in UE 5.8. If current code uses deprecated paths, attribute initialization may silently fail or log warnings. **Mitigation**: Verify init pattern against `deprecated-apis.md` before first full test.

---

## GDD Requirements Addressed

| GDD System | TR-ID | Requirement | How This ADR Addresses It |
|------------|-------|-------------|---------------------------|
| player-movement.md | TR-mov-001 | CMC + camera-relative input + facing decouple | `AMoonCharacterBase` uses CMC + Enhanced Input; facing decoupled from camera yaw |
| player-movement.md | TR-mov-004 | Data-driven tuning + hard clamps | Tuning knobs exposed as UPROPERTY with Safe Range comments; CurveTable binding for per-attribute values |
| health-damage-core.md | TR-hp-001 | GAS ASC + AttributeSet, single ApplyDamage entry | `UMoonAbilitySystemComponent` + `UMoonAttributeSet` + `UMoonDamageExecCalc` establish single entry point |
| health-damage-core.md | TR-hp-002 | Shield/armor intercept + bBypassDefense | `UMoonDamageExecCalc` evaluates `bBypassDefense` flag before applying damage |
| health-damage-core.md | TR-hp-003 | State.Invulnerable i-frame gating | ExecCalc reads `State.Invulnerable` tag presence, returns 0 damage if set |
| health-damage-core.md | TR-hp-004 | Ref-counted overlay tags, auto-clear on death | ASC's built-in ref-counted gameplay tag system; death handler calls blanket tag clear |
| health-damage-core.md | TR-hp-005 | State.Executable + TryExecute API | `State.Executable` tag managed by GAS; `TryExecute()` implemented in `AMoonCharacterBase` |
| health-damage-core.md | TR-hp-009 | Death = instant checkpoint respawn | Death path calls `UMoonCheckpointSubsystem::RestoreCheckpoint` (see ADR-0002) |

---

## Implementation Status

As of 2026-07-18 (updated 2026-07-20):
- ✅ `AMoonCharacterBase` — implemented, compiles, PIE-tested
- ✅ `UMoonAbilitySystemComponent` — implemented
- ✅ `UMoonAttributeSet` — implemented (Health, MaxHealth, Mana, MaxMana, ManaRegenRate, TensionGauge, TensionGaugeMax)
- ✅ Enhanced Input mapping context — set up
- ⚠️ `UMoonDamageExecCalc` — skeleton exists (Blackhole spell GAS slice test rig in place); full damage pipeline verification pending PIE confirmation
- ❌ `UMoonCheckpointSubsystem` — not yet implemented (see ADR-0002)

---

## Related Decisions
- ADR-0002 (Checkpoint Persistence) — depends on this ADR
- ADR-0003 (Spell Casting GAS Implementation) — depends on this ADR
- `design/gdd/player-movement.md` — primary GDD source
- `design/gdd/health-damage-core.md` — primary GDD source
