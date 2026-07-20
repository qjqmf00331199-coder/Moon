# Moon Fragment Hunt — Master Architecture

## Document Status
- Version: 1.0
- Last Updated: 2026-07-18
- Engine: Unreal Engine 5.8
- Review Mode: Solo
- GDDs Covered: player-movement, camera-system-base, health-damage-core, enemy-ai-base, spell-casting-base, dash-evasion, combo-tension-gauge, luna-overdrive, combat-hud
- ADRs Referenced: ADR-0001 (Player Movement and GAS Core Foundation)
- Technical Director Sign-Off: 2026-07-18 — APPROVED WITH CONDITIONS (see Phase 7b below)
- Lead Programmer Feasibility: SKIPPED — Solo mode (per director-gates.md)

## Engine Knowledge Gap Summary

Project pinned to UE 5.8; LLM training data covers up to ~5.3. HIGH RISK domain relevant to this scope: **GAS attribute set initialization** — legacy init functions deprecated in 5.8, replacement pattern undocumented in engine-reference. Already flagged as an Open Question in `health-damage-core.md`, `spell-casting-base.md`, and `luna-overdrive.md`. All GAS-touching modules (Health/Damage Core, Spell Casting, Combo/Tension Gauge, Luna Overdrive) inherit this flag. Additional narrow gap: `SetLooseGameplayTagCount` UE5.8 signature not in engine-reference — required for Luna Overdrive's non-counted tag ownership pattern.

Enhanced Input, CMC, AIController/BT/Perception, SpringArm/Camera, UMG/CommonUI domains are LOW/MEDIUM risk — patterns used across GDDs already align with `current-best-practices.md`.

## System Layer Map

```
┌─ PRESENTATION ────────────────────────────────────────┐
│ Combat HUD                                             │
├─ FEATURE ───────────────────────────────────────────────┤
│ Combo/Tension Gauge · Luna Overdrive                   │
├─ CORE ──────────────────────────────────────────────────┤
│ Camera System (base) · Enemy AI (base) · Spell Casting  │
│ (base) · Dash/Evasion                                   │
├─ FOUNDATION ────────────────────────────────────────────┤
│ Player Movement · Health/Damage Core                    │
├─ PLATFORM ──────────────────────────────────────────────┤
│ UE5.8 (GAS, Enhanced Input, CMC, AIController, UMG)      │
└──────────────────────────────────────────────────────────┘
```

Matches `design/gdd/systems-index.md` dependency map exactly — no circular dependencies (confirmed there and re-verified here).

## Module Ownership

| Module | Layer | Owns | Exposes | Consumes | Engine APIs |
|---|---|---|---|---|---|
| `AMoonCharacterBase` (Movement) | Foundation | Velocity, jump/coyote timers, facing | `AddMovementInput`, Z-launch API | Camera Yaw (read) | `UCharacterMovementComponent`, Enhanced Input |
| `UMoonAttributeSet` + ASC (Health/Damage) | Foundation | Health/Mana/TensionGauge attributes, `State.Invulnerable`/`State.Executable` tags | `ApplyDamage`, `TryExecute`, `OnDeath`, `OnHealthPercentCrossed` | — | GAS `UAbilitySystemComponent`, `UGameplayEffectExecutionCalculation` ⚠️HIGH RISK |
| Camera Module | Core | SpringArm/Camera params, FOV state | `ResetCameraLag()`, execution-blend API | Movement position, Overdrive/Execution events | `USpringArmComponent`, `UCameraComponent` |
| Enemy AI Module | Core | AIController/BT state, archetype MaxHealth | `OnAttackTelegraphed`/`Committed`, `TriggerStagger`/`ClearStagger` | HDC `OnDeath` | `AIController`, `UAIPerceptionComponent`, Behavior Tree |
| Spell Casting Module | Core | Mana attribute, per-element cooldown, `CostBypass.Active` consumer contract | `OnSpellCast`/`OnSpellHit`, cast gate | HDC damage entry point, Movement non-block contract | GAS `UGameplayAbility` (InstancedPerActor) ⚠️HIGH RISK |
| Dash/Evasion Module | Core | Dash charges, `IsJustDodge`判定 | Just-Dodge events | Enemy AI telegraph, Movement velocity API, HDC tags | `LaunchCharacter`/velocity override |
| Combo/Tension Module | Feature | TensionGauge value + decay | `OnOverdriveTriggered` | Spell `OnSpellHit`, HDC tag/damage events | GAS Attribute ⚠️HIGH RISK |
| Luna Overdrive Module | Feature | `CostBypass.Active` grant/clear (sole owner), timer | `OnOverdriveStarted`/`Ended`, `OverdriveTimeRemaining` | Tension trigger, HDC Death | `SetLooseGameplayTagCount` (non-counted) ⚠️API signature unverified |
| Combat HUD Module | Presentation | Widget state mirroring only | — (read-only) | All upstream delegates | UMG + CommonUI |

Dependency flow: `Movement/HDC → Camera/EnemyAI/Spell/Dash → Tension/Overdrive → HUD` — no upstream module ever references a downstream one at compile time (enforced architectural invariant, matches Player Movement Core Rule 7's "no compile-time reference" pattern generalized project-wide).

## Data Flow

**1. Frame update path**
```
Input(Enhanced Input) → Movement(velocity) ─┐
                                             ├→ Camera(read pos/yaw) → Render
Input → Spell Cast Gate → GameplayEffect ──→ HDC.ApplyDamage → Health Attribute
                                             └→ MakeNoise → Enemy AI Perception
```
Synchronous, completes within-frame (instant-cast contract — no ability latency across all spell/dash systems, InstancedPerActor).

**2. Event/signal path**
```
Enemy AI.OnAttackTelegraphed/Committed ──(sub)──→ Dash/Evasion
HDC.OnDeath / OnHealthPercentCrossed ──(sub)──→ Enemy AI, Combo Gauge, Boss Phase (future)
HDC.OnTagAdded(State.Executable) ──(sub)──→ Combo/Tension Gauge
Spell.OnSpellHit ──(sub)──→ Combo/Tension Gauge
Combo/Tension.OnOverdriveTriggered ──(sub)──→ Luna Overdrive
Luna Overdrive.OnOverdriveStarted/Ended ──(sub)──→ Combat HUD, Camera(FOV), Overdrive Visual State (future)
```
All GAS delegates/GameplayTag callbacks — no polling (matches Combat HUD Rule 3's event-driven mandate).

**3. Save/load path**
**GAP — no Persistence system exists.** `health-damage-core.md` Core Rule 6 assumes a checkpoint/respawn mechanism this architecture does not yet define. Tracked as Required New ADR below.

**4. Initialization order**
`HDC (ASC+AttributeSet) → Movement/Camera (parallel, no mutual dependency) → Enemy AI/Spell Casting → Dash/Evasion → Combo Gauge → Luna Overdrive → Combat HUD` (pure consumer, last).

**Thread boundary**: Game thread only. No networking in MVP scope — no cross-thread GAS ability activation.

## API Boundaries

```cpp
// Health/Damage Core — single damage entry point
bool UMoonAttributeSet::TryExecute(AActor* Target);
void ApplyDamage(AActor* Target, float RawDamage, bool bBypassDefense);
DECLARE_MULTICAST_DELEGATE(OnDeath);
DECLARE_MULTICAST_DELEGATE_OneParam(OnHealthPercentCrossed, float Threshold);
// Invariant: RawDamage<0 hard-clamped to 0. State.Invulnerable always blocks 100%, bBypassDefense notwithstanding.

// Player Movement — movement contract
void AddMovementInput(FVector Dir); // guaranteed live during casting/execution/hitstop
void LaunchCharacterZ(float ZVelocity); // consumed by Dash, Arena Morphing (future)
// Invariant: only the Status Effect system (future) may set MovementLocked.

// Spell Casting — cast gate
bool CanCast(EElement Element); // Mana>=Cost AND !OnCooldown, true if CostBypass.Active
void CastSpell(EElement Element); // completes same-frame, per-frame-per-element cap=1
// Invariant: CostBypass.Active may only be granted by Luna Overdrive.

// Luna Overdrive — sole grantor contract
void TriggerOverdrive(); // OnOverdriveTriggered handler, lazy time-compare gate
float GetTimeRemaining(); // OverdriveEndTime - CurrentTime, clamped non-negative
// Invariant: use SetLooseGameplayTagCount(1/0) — never AddLooseGameplayTag counted API.

// Combat HUD — read-only mirror
// Subscribes to the above delegates/attributes only; exposes no gameplay-mutating API (Core Rule 1).
```

⚠️ All GAS types above (`UAbilitySystemComponent`, `UGameplayEffectExecutionCalculation`, `SetLooseGameplayTagCount`) are provisional pending real UE5.8 header cross-check — `ue-gas-specialist` verification required before implementation (already an Open Question in 3 GDDs).

## ADR Audit

| ADR | Engine Compat | Version | GDD Linkage | Conflicts | Valid |
|---|---|---|---|---|---|
| ADR-0001 (Player Movement and GAS Core Foundation) | ⚠️ Partial — GAS init flagged, no dedicated Engine Compatibility section | ✅ UE5.8 stated in header | ❌ No GDD Requirements Addressed section | None vs this session's layer map | ⚠️ **Proposed** — must move to Accepted; stories referencing it are currently auto-blocked per `docs/CLAUDE.md` |

Missing required sections per `docs/CLAUDE.md`: **ADR Dependencies**, **Engine Compatibility** (as a distinct section), **GDD Requirements Addressed**. Recommend a revision pass on ADR-0001 before Accepted status.

### Traceability Coverage

19 baseline requirements (see Technical Requirements Baseline, captured in conversation) vs. existing ADRs:

| Coverage | Count |
|---|---|
| Covered by ADR-0001 (Movement/GAS foundation) | TR-mov-001~005, TR-hp-001~003 (partial/indirect) |
| **GAP — no ADR** | TR-cam-*, TR-ai-*, TR-spell-*, TR-dash-*, TR-tension-*, TR-overdrive-*, TR-hud-* (14 of 19) |

Only the Foundation layer has ADR coverage. Core, Feature, and Presentation layers are entirely uncovered.

## Required ADRs

**Foundation (must have before coding starts):**
- ADR-0001 → revise to Accepted (add missing required sections)
- **New: Save/Persistence Strategy** — resolves the Data Flow Phase 3 gap; `health-damage-core.md` Core Rule 6 depends on it.

**Core:**
- Enemy AI architecture (BT/Perception composition, archetype data)
- Spell Casting GAS ability structure (3-element slots, CostBypass hook implementation)
- Dash/Evasion impulse + Just-Dodge implementation

**Feature:**
- Combo/Tension Gauge Attribute + decay implementation
- Luna Overdrive tag ownership (non-counted loose tag) implementation

**Presentation:**
- Combat HUD UMG/CommonUI widget architecture

## Architecture Principles

1. **Presentation never gates gameplay ticks** — hitstop, execution cameras, and casting animations are pure rendering-layer overlays; movement input processing and damage resolution continue at 100% tick rate underneath them (generalizes Player Movement Core Rule 8/9 and Health/Damage Core Rule 5 project-wide).
2. **Single entry point per concern** — all damage flows through `ApplyDamage`/GameplayEffect (Health/Damage Core Rule 1); all cost/cooldown bypass flows through the single `CostBypass.Active` tag (Spell Casting Rule 10, Luna Overdrive sole-grantor contract). No system reimplements a sibling's entry point.
3. **Upstream never compiles against downstream** — Movement does not reference Spell Casting types; Spell Casting does not reference Combo/Tension Gauge types. Dependencies flow one direction only, matching `systems-index.md`'s dependency map with zero circular references.
4. **Event-driven over polling** — all cross-system communication uses GAS delegates/GameplayTag callbacks, never per-tick state polling (Combat HUD Rule 3 generalized as a project-wide default).
5. **Engine-version risk is explicitly flagged, never silently assumed** — any UE5.8 API without a verified engine-reference entry (GAS attribute init, `SetLooseGameplayTagCount`) carries an Open Question and requires specialist header cross-check before implementation.

## Open Questions

| ID | Summary | Priority | Resolution Path |
|----|---------|----------|-----------------|
| QQ-01 | Persistence/checkpoint system undesigned — Save/load path is an architecture gap | High | New ADR (Foundation) + eventual Persistence GDD |
| QQ-02 | GAS attribute set init pattern for UE5.8 undocumented in engine-reference | High | `ue-gas-specialist` header cross-check at implementation time (inherited from 3 GDDs) |
| QQ-03 | `SetLooseGameplayTagCount` UE5.8 signature unverified | Medium | `ue-gas-specialist` header cross-check before Luna Overdrive implementation |
| QQ-04 | ADR-0001 missing required sections (ADR Dependencies, Engine Compatibility, GDD Requirements Addressed) | High | Revise ADR-0001 before promoting to Accepted |
