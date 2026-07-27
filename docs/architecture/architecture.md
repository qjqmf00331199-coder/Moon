# Moon Fragment Hunt — Master Architecture

## Document Status
- Version: 1.1
- Last Updated: 2026-07-27
- Engine: Unreal Engine 5.8
- Review Mode: Solo architecture authoring; latest independent architecture review was full delta
- GDDs Covered: player-movement, camera-system-base, health-damage-core, enemy-ai-base, spell-casting-base, dash-evasion, combo-tension-gauge, luna-overdrive, combat-hud
- ADRs Referenced: ADR-0001 through ADR-0011
- Architecture Review: 2026-07-27 v4 — PASS, 65 covered / 11 partial / 0 gaps of 76 active requirements
- Technical Director Sign-Off: 2026-07-27 — APPROVED WITH CONDITIONS
- Lead Programmer Feasibility: SKIPPED — Solo mode

## Engine Knowledge Gap Summary

The project is pinned to Unreal Engine 5.8, which is beyond the model training cutoff and is treated as HIGH risk for engine API accuracy. All implementation stories must cross-check `docs/engine-reference/unreal/` before relying on remembered Unreal behavior.

The architecture is complete for MVP coverage. Some implementation-time verification items remain, but the ADR-0010/0011 story-readiness blockers were verified on 2026-07-27:

- ADR-0010/0011 verification report: `ue58-api-verification-adr-0010-0011-2026-07-27.md`.
- `FOnAttributeChangeData::GEModData` semantics, ASC cooldown tag/time queries, CommonUI input-method/glyph APIs, and `TG_PostUpdateWork` ordering for current planned call sites are verified against local UE5.8 headers/source.
- GAS attribute initialization, `PostGameplayEffectExecute`, `PreAttributeChange`, and loose GameplayTag count clearing remain separate UE5.8 verification items before Health/Damage Core or Luna Overdrive stories using those exact APIs are marked Ready.
- `UAbilitySystemComponent::SetLooseGameplayTagCount` usage for Luna Overdrive remains a UE5.8 verification item.
- Future latent `AbilityTask` completions or Blueprint ticks at `TG_PostUpdateWork`/`TG_LastDemotable` must not call `AddTension*` unless explicitly ordered before `TensionResolveTickFunction`.
- Deprecated APIs remain banned per `docs/engine-reference/unreal/deprecated-apis.md`, especially legacy `UCharacterMovementComponent::SetMovementMode()` overloads and legacy GAS attribute initialization functions.

Enhanced Input, Character Movement Component basics, SpringArm/Camera, UMG `UUserWidget`, AIController/Behavior Tree, and AIPerception are acceptable with local engine-reference checks. Mass Framework, Iris, advanced CommonUI input routing, and GAS internals are not to be guessed from memory.

## System Layer Map

```
┌─ PRESENTATION ───────────────────────────────────────────────┐
│ Combat HUD                                                    │
├─ FEATURE ────────────────────────────────────────────────────┤
│ Combo/Tension Gauge · Luna Overdrive                         │
├─ CORE ───────────────────────────────────────────────────────┤
│ Camera System · Enemy AI · Spell Casting · Dash/Evasion      │
├─ FOUNDATION ─────────────────────────────────────────────────┤
│ Player Movement · Health/Damage Core · Checkpoint Runtime    │
├─ PLATFORM ───────────────────────────────────────────────────┤
│ Unreal Engine 5.8: GAS, Enhanced Input, CMC, AI, UMG/CommonUI │
└──────────────────────────────────────────────────────────────┘
```

Dependency flow is one-way: `Foundation -> Core -> Feature -> Presentation`. Presentation never mutates gameplay state. Feature systems may consume Core/Foundation events and attributes, but they do not own upstream state. Core systems may read Foundation contracts, but Foundation must not compile against Core, Feature, or Presentation systems.

## Module Ownership

| Module | Layer | Owns | Exposes | Consumes | Governing ADR |
|---|---|---|---|---|---|
| Player Movement (`AMoonCharacterBase`, CMC-facing runtime contract) | Foundation | camera-relative movement input, airborne derived state, jump buffer/coyote timers, movement-lock read contract, presentation-only hitstop policy | `AddMovementInput`, movement-lock read, external velocity/Z launch path, non-root-motion locomotion contract | Camera yaw read, future Status Effect write authority for movement lock | ADR-0001, ADR-0009 |
| Health/Damage Core (`UMoonAttributeSet`, ASC, health event interface) | Foundation | Health, MaxHealth, Mana, TensionGauge attributes, damage entry point, `State.Invulnerable`, `State.Executable`, `State.Dead`, death detection | damage GameplayEffects, `TryExecute`, `OnDeath`, `OnExecuted`, `OnHealthPercentCrossed`, Health attribute delegates, `ResetDeathState()` | Checkpoint restore call, execution target tags | ADR-0001, ADR-0008 |
| Runtime Checkpoint (`UMoonCheckpointSubsystem`) | Foundation | in-memory checkpoint snapshot, restore sequencing | `CaptureCheckpoint`, `RestoreCheckpoint`, `HasActiveCheckpoint` | Health/Damage Core `ResetDeathState()`, Health restore GE | ADR-0002 |
| Camera System | Core | SpringArm/camera hierarchy, camera tuning data, pitch clamp, camera lag reset, Overdrive/execution presentation blend | `UMoonCameraSettings`, `ResetCameraLag()`, camera presentation hooks | Movement position/yaw, checkpoint teleport, Overdrive/execution events | ADR-0005 |
| Enemy AI | Core | enemy archetype tuning, AIController + shared BT/Blackboard, perception, telegraph/commit state, dead-state response | `OnAttackTelegraphed`, `OnAttackCommitted`, `IsTelegraphingAttack`, `GetAttackCommittedTime`, `MeleeAttackRange`, `TriggerStagger` | Health/Damage Core `OnDeath`, Dash just-dodge query | ADR-0006 |
| Spell Casting | Core | per-element GAS abilities, shared Mana cost/cooldown gate, cast-rate limit, `CostBypass.Active` consumption, cooldown query surface | spell activation, `OnSpellCast`, `OnSpellHit`, cooldown tag/remaining/duration accessors | Movement non-block contract, HDC damage entry point, Luna Overdrive bypass state, Combo/Tension gain calls | ADR-0003, ADR-0010 |
| Dash/Evasion | Core | instant swept dash step, dash charges, i-frame window, Just-Dodge spatial query and `State.Executable` grant | dash charge attribute, Just-Dodge success, dash HUD surface | Movement API, HDC tags, Enemy AI telegraph query, Combo/Tension gain call | ADR-0007, ADR-0010 |
| Luna Overdrive | Feature | `Inactive`/`Active`/`Recovery` state, fixed 10s window, 1.5s recovery, `CostBypass.Active` sole ownership, mana-regen pause | `TriggerOverdrive`, `ForceEndOverdrive`, `IsOverdriveActive`, `IsTensionGainLocked`, started/ended events, remaining-time query | Combo/Tension max trigger, HDC `OnDeath`, Spell Casting bypass checks, Combat HUD state read | ADR-0004 |
| Combo/Tension Gauge | Feature | Tension gain/penalty/decay ordering, damage-penalty pending flag, `TG_PostUpdateWork` resolution, death reset of gauge | direct gain/penalty methods on character, read-only GAS attribute value and Building/Decaying state | Spell hit, Dash Just-Dodge, Health attribute-change delegate, Luna Overdrive lock, HDC death | ADR-0001, ADR-0004, ADR-0011 |
| Combat HUD | Presentation | read-only widget binding, per-widget update policy, cooldown overlays, glyph swapping, death/overdrive visual reset | Blueprint visual hooks only; no gameplay mutation | GAS attributes, spell cooldown query, dash charges, TensionGauge, Overdrive events, HDC death | ADR-0010 |

## Data Flow

**Frame Update Path**

```
Enhanced Input
  ├─ Movement input -> AMoonCharacterBase / CMC -> position, velocity, facing
  ├─ Spell input -> GAS ability gate -> cost/cooldown/damage GE -> AttributeSet
  └─ Dash input -> instant swept position step -> i-frame tag / Just-Dodge query

Attribute and state changes
  ├─ HDC PostGameplayEffectExecute -> Health/Mana/Tension/Death delegates
  ├─ Combo/Tension late tick -> pending penalty then decay
  ├─ Overdrive time boundary -> tag/state/events
  └─ HUD event bindings -> visual mirror only
```

Gameplay judgment remains synchronous and game-thread local. Presentation effects such as hitstop, camera blends, HUD interpolation, and glyph swaps may lag visually but must not delay movement input, ability activation, damage application, death detection, or execution success.

**Event and Signal Path**

```
Enemy AI telegraph/commit ──► Dash/Evasion Just-Dodge query
Dash Just-Dodge success ────► Combo/Tension AddTensionFromJustDodge()
Spell OnSpellHit ──────────► Combo/Tension AddTensionFromSpellHit()
Health attribute decrease ─► Combo/Tension pending damage penalty
HDC death branch ──────────► Character/Enemy OnDeath
Character OnDeath ─────────► Checkpoint restore, Overdrive force-end, HUD reset
Enemy OnDeath ─────────────► AI StopLogic / corpse handling
Tension reaches max ───────► Luna Overdrive TriggerOverdrive()
Overdrive start/end ───────► Spell bypass gate, HUD, camera presentation
GAS attribute delegates ───► HUD read-only widgets
Cooldown tag changes ──────► HUD cooldown overlay sweep
```

Cross-system communication uses direct calls when caller and owner already share the same character instance at the triggering moment, and delegates/tag callbacks when the consumer is decoupled or presentation-only. No polling is permitted for gameplay-significant events such as Death or Overdrive entry.

**Save / Restore Path**

MVP persistence is runtime-only checkpoint memory, not disk save. `UMoonCheckpointSubsystem` owns capture/restore. Restore must call `UMoonAttributeSet::ResetDeathState()` before the Health restore GameplayEffect so the `bIsDead` guard and `State.Dead` tag are cleared before the restored pawn can die again. Direct `UMoonAttributeSet` value writes during restore are forbidden; Health restore still goes through GameplayEffect application.

**Initialization Order**

```
ASC + AttributeSet
  -> Checkpoint subsystem availability
  -> Movement / Camera setup
  -> Spell / Dash abilities
  -> Enemy AI perception and BT
  -> Combo/Tension subscriptions and late tick
  -> Luna Overdrive state/events
  -> Combat HUD BindToPlayer after real upstream delegates are available
```

HUD widgets that depend on first real upstream values must stay collapsed until their first delegate update. Showing a zero/default value before initialization is considered worse than showing nothing.

## API Boundaries

```cpp
// Player Movement
void AddMovementInput(FVector Direction);
bool IsMovementLocked() const;
// Movement lock write remains private/reserved for the future Status Effect ADR.
// Hitstop/execution freeze is mesh/camera presentation only; no Time Dilation.

// Health/Damage Core
bool ApplyDamage(AActor* Target, float RawDamage, bool bBypassDefense);
bool TryExecute(AActor* Target);
DECLARE_MULTICAST_DELEGATE(FOnMoonDeath);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnMoonExecuted, AActor* Target);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnMoonHealthPercentCrossed, float Threshold);
void UMoonAttributeSet::ResetDeathState();
// Damage is blocked by State.Invulnerable and State.Dead via ApplicationRequirement.

// Checkpoint Runtime
void CaptureCheckpoint(APawn* Player);
void RestoreCheckpoint(APawn* Player);
bool HasActiveCheckpoint() const;
// Restore must reset death state before applying the Health restore GE.

// Camera System
UMoonCameraSettings* CameraSettings;
void ResetCameraLag();
// Camera tuning is data-asset driven. Constructor literals for tuning are forbidden.

// Enemy AI
DECLARE_MULTICAST_DELEGATE(FOnAttackTelegraphed);
DECLARE_MULTICAST_DELEGATE(FOnAttackCommitted);
bool IsTelegraphingAttack() const;
float GetAttackCommittedTime() const;
float GetMeleeAttackRange() const;
void TriggerStagger();
void ClearStagger();

// Spell Casting
bool CanCast(EMoonSpellElement Element) const;
void CastSpell(EMoonSpellElement Element);
FGameplayTag GetElementCooldownTag(EMoonSpellElement Element) const;
float GetElementCooldownRemaining(EMoonSpellElement Element) const;
float GetElementCooldownDuration(EMoonSpellElement Element) const;
// CostBypass.Active is consumed here but owned only by Luna Overdrive.

// Dash/Evasion
void ActivateDash();
bool CheckJustDodge();
// Dash is an instant swept position step, not a velocity override.

// Luna Overdrive
void TriggerOverdrive();
void ForceEndOverdrive(EMoonOverdriveEndReason Reason);
bool IsOverdriveActive() const;
bool IsTensionGainLocked() const;
float GetOverdriveTimeRemaining() const;

// Combo/Tension
void AddTension(float Amount);
void AddTensionFromSpellHit(float ManaCost);
void AddTensionFromJustDodge();
void ApplyTensionDamagePenalty(); // defers to TensionResolveTickFunction
// Penalty then Decay resolves in TG_PostUpdateWork after same-frame Gain.

// Combat HUD
void BindToPlayer(APawn* Player);
UFUNCTION(BlueprintImplementableEvent) void OnCooldownStateChanged(
    EMoonSpellElement Element,
    bool bOnCooldown,
    float FractionRemaining);
// HUD exposes no gameplay-mutating API.
```

## ADR Audit

| ADR | Status | Domain | Engine Compatibility | GDD Linkage | Conflicts |
|---|---|---|---|---|---|
| ADR-0001 Player Movement and GAS Core | Accepted | Foundation | Partial; GAS init remains UE5.8 risk | Yes | None blocking |
| ADR-0002 Runtime Checkpoint Persistence | Accepted | Foundation | OK; restore sequence amended for death reset | Yes | None |
| ADR-0003 Spell Casting GAS Implementation | Accepted | Core | OK for ADR-0010 cooldown HUD query; broader GAS init risks separate | Yes | None |
| ADR-0004 Luna Overdrive Fixed Window | Accepted | Feature | Partial; loose tag count verification required | Yes | None |
| ADR-0005 Camera System SpringArm | Accepted | Core | OK with known camera GDD value cleanup | Yes | C-3 non-blocking GDD contradiction |
| ADR-0006 Enemy AI Behavior Tree | Accepted | Core | Partial; AI hearing mapping and perf remain implementation checks | Yes | None blocking |
| ADR-0007 Dash/Evasion Just-Dodge | Accepted | Core | OK after avoiding deprecated velocity model; implementation must avoid legacy movement overloads | Yes | C-2 non-blocking GDD contradiction |
| ADR-0008 Health/Damage Core Death Contract | Accepted | Foundation | Partial; GAS callbacks/tag clear verification required | Yes | Resolves C-1 |
| ADR-0009 Player Movement Runtime Contract | Accepted | Foundation | OK; removes Time Dilation presentation violation by decision | Yes | Resolves V-1 architecturally |
| ADR-0010 Combat HUD Widget Architecture | Accepted | Presentation | OK; CommonUI glyph/input-method API verified 2026-07-27 | Yes | None |
| ADR-0011 Combo/Tension Gauge | Accepted | Feature | OK for current call sites; `GEModData` and `TG_PostUpdateWork` verified 2026-07-27 | Yes | None |

### Traceability Coverage

Latest authoritative review: `docs/architecture/architecture-review-2026-07-27-v4.md`.

| System | Covered | Partial | Gaps | Governing ADR |
|---|---:|---:|---:|---|
| Health/Damage Core | 9 | 0 | 0 | ADR-0001, ADR-0002, ADR-0008 |
| Player Movement | 8 | 2 | 0 | ADR-0001, ADR-0005, ADR-0009 |
| Camera System | 7 | 2 | 0 | ADR-0005 |
| Luna Overdrive | 7 | 1 | 0 | ADR-0004 |
| Enemy AI | 6 | 2 | 0 | ADR-0006 |
| Spell Casting | 7 | 3 | 0 | ADR-0003, ADR-0004 |
| Dash/Evasion | 6 | 2 | 0 | ADR-0007, ADR-0010 |
| Combo/Tension Gauge | 6 | 1 | 0 | ADR-0001, ADR-0004, ADR-0011 |
| Combat HUD | 7 | 0 | 0 | ADR-0010 |

Total active requirements: 76. Covered: 65. Partial: 11. Gaps: 0.

## Required ADRs

No additional ADR is required for MVP architecture coverage as of the 2026-07-27 v4 review.

Follow-up ADRs may still be needed before future systems are built, especially Status Effect, Core Extraction Execution, Boss Phase, Save/Persistence beyond runtime checkpoints, and any multiplayer/replication architecture. These are outside current MVP architecture coverage and should not block `/create-epics` for the approved MVP systems.

## Architecture Principles

1. Presentation never gates gameplay judgment. Hitstop, camera blends, execution flair, and HUD interpolation must not slow or delay movement input, cast gates, damage, death, execution, or tag state.
2. Each shared state has one owner. Health/Damage Core owns Health/Death tags, Luna Overdrive owns `CostBypass.Active`, Enemy AI owns enemy archetype tuning, Camera owns camera tuning, and Checkpoint owns checkpoint snapshots.
3. Use the narrowest communication shape that preserves ownership. Same-character gameplay writes may be direct method calls; decoupled consumers use delegates, GAS attribute callbacks, or GameplayTag callbacks.
4. Data-driven tuning is mandatory for gameplay values. Constructor literals are forbidden for camera tuning and should be avoided anywhere a GDD lists a tuning knob.
5. Engine-version risk is named at the decision boundary. UE5.8 API assumptions remain verification items until checked against local engine headers or project engine-reference docs.
6. ADRs can ratify correct shipped spike code, but only when they also name deviations and migration steps. Spike behavior is not production architecture until an ADR accepts or amends it.

## Open Questions

| ID | Summary | Priority | Resolution Path |
|----|---------|----------|-----------------|
| QQ-01 | UE5.8 GAS callback signatures, attribute initialization, and loose tag count clearing remain implementation-time risks; `GEModData` was verified for ADR-0011 on 2026-07-27 | High | Verify remaining exact APIs against installed UE5.8 headers before HDC / Luna Overdrive stories are Ready |
| QQ-02 | CommonUI input-method changed signal and glyph asset source of truth after UE5.8 Enhanced Input/Common Input unification | Resolved | Verified 2026-07-27 in `ue58-api-verification-adr-0010-0011-2026-07-27.md` |
| QQ-03 | Combo/Tension `TG_PostUpdateWork` ordering precondition for current gain call sites | Resolved | Verified 2026-07-27; preserve invariant for future latent AbilityTasks / late Blueprint ticks |
| QQ-04 | Same-frame multiple damage-penalty events collapse to one bool-triggered penalty; designer confirmation still needed before Production tuning | Medium | Game-design review during Combo/Tension implementation/tuning |
| QQ-05 | `AMoonCharacterBase` responsibility accretion across Movement, Overdrive, Tension, Health events, and HUD binding remains non-blocking but growing | Medium | Reassess component decomposition before Production or when a third tick-owning concern is added |
| QQ-06 | GDD cleanup flags C-2/C-3 and missing tuning knobs remain non-blocking for architecture PASS | Resolved | Cleaned up 2026-07-27 in `dash-evasion.md`, `camera-system-base.md`, and `enemy-ai-base.md` |
