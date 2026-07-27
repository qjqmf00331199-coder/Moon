# ADR-0008: Health/Damage Core — Death Detection & Event Contract

## Status
Accepted

**Accepted:** 2026-07-27 (per `/architecture-review` C-1 recheck — `Depends On` is ADR-0001,
already Accepted, no unresolved blocking dependency; TR-hp-006/007/008 and Rules 9/10 addressed
per its own GDD Requirements Addressed table; ADR-0002's required amendment already applied.
Verification Required items (GAS callback signature drift, `RemoveLooseGameplayTagCount` full-count
semantics) remain open as implementation-time risks, same status as ADR-0005's TR-cam-003/004 —
non-blocking for Acceptance.)

## Date
2026-07-27

## Engine Compatibility

| Field | Value |
|-------|-------|
| **Engine** | Unreal Engine 5.8 |
| **Domain** | Core (GAS — Health/Damage) |
| **Knowledge Risk** | HIGH — extends `UMoonAttributeSet`, whose legacy attribute-init pattern is already flagged as deprecated-in-5.8 by ADR-0001 with no documented replacement. This ADR adds a second GAS surface (`PostGameplayEffectExecute` death branch, interface dispatch, ref-counted tag clear) with the same "post-cutoff, unverified against real headers" exposure. |
| **References Consulted** | `docs/engine-reference/unreal/VERSION.md`, `breaking-changes.md`, `deprecated-apis.md`, `plugins/gameplay-ability-system.md` (`PostGameplayEffectExecute`, `GetGameplayAttributeValueChangeDelegate`, loose-tag patterns) |
| **Post-Cutoff APIs Used** | None new beyond ADR-0001's already-flagged `UAttributeSet` init surface — this ADR extends the existing class, it does not introduce a new post-cutoff API family. |
| **Verification Required** | (1) Confirm `PostGameplayEffectExecute` and `PreAttributeChange` signatures are unchanged in UE5.8 (not independently confirmed by this project's engine-reference library — same gap ADR-0001 already carries). (2) Confirm `UAbilitySystemComponent`'s loose-tag ref-count API (`AddLooseGameplayTag`/`RemoveLooseGameplayTag` with count) behaves per `health-damage-core.md` Edge Case row 5 ("두 소비 시스템이 동시에 부여/해제") — needed for the `State.Dead` blanket-clear step below. |

## ADR Dependencies

| Field | Value |
|-------|-------|
| **Depends On** | ADR-0001 (Accepted) — `UMoonAttributeSet`, `UMoonAbilitySystemComponent`, `AMoonCharacterBase` must exist; this ADR extends them, does not replace them. |
| **Enables** | ADR-0002 (Checkpoint Persistence, Accepted — restore path needs the `bIsDead` reset this ADR defines), ADR-0004 (Luna Overdrive, Proposed — `PlayerDeath` end-reason needs a real `OnDeath` to subscribe to), ADR-0006 (Enemy AI, Proposed — entire Dead-state branch needs a real `OnDeath` to subscribe to) |
| **Blocks** | None currently — this ADR closes the C-1 blocking conflict from `architecture-review-2026-07-27.md` and is a precondition for ADR-0004/0006 being Accepted. |
| **Ordering Note** | Per `docs/CLAUDE.md`'s ADR status lifecycle, ADR-0004 and ADR-0006 cannot be Accepted while they cite an undefined `OnDeath`. This ADR should be Accepted, and its GDD-visible surface stable, before those two are re-reviewed for Acceptance. ADR-0002 is already Accepted against an informal `AMoonCharacterBase::OnDeath()` call-site assumption — Decision 2 below is a compatible refinement of that assumption, not a breaking change, but ADR-0002's Migration Plan needs one addendum step (Decision 6). |

## Context

### Problem Statement

`architecture-review-2026-07-27.md` (finding **C-1**, recurring since 2026-07-18 and now three ADRs wide) found that `OnDeath` is consumed by ADR-0002 §3, ADR-0004 §Risks, and ADR-0006 §7, but defined by no ADR. ADR-0001's Decision text covers the damage `ExecutionCalculation` (defense-bypass, i-frame gating) only — it never defines a `Health<=0` transition, a death event, or an execution event. `health-damage-core.md` Rules 5/7/9/10 and ACs 4/7/8/9/10 already define the contract unambiguously at the GDD level (immediate, presentation-decoupled death; exactly-once `OnDeath`; `TryExecute`/`OnExecuted`; `OnHealthPercentCrossed`). **The GDD is fine — the architecture never picked it up.** This is the highest-priority blocking item in the review and gates `/create-epics`.

### Current State

`UMoonAttributeSet` already implements `PreAttributeChange` and `PostGameplayEffectExecute` with basic Health/Mana/TensionGauge clamping (per this project's Track B implementation session). Neither hook currently detects a `Health<=0` transition, guards against re-firing, exposes any delegate, nor clears overlay tags on death. `AMoonCharacterBase` has a `TryExecute()` stub referenced by ADR-0001's requirements table, but `OnExecuted` is not wired. `AMoonEnemyCharacterBase` (ADR-0006) and the checkpoint restore path (ADR-0002) both assume a producer that does not yet exist.

### Constraints

- Death detection must be immediate and presentation-decoupled (Rule 5) — no tick-based or delayed detection, matching `player-movement.md` Rule 9's hitstop separation philosophy (judgment and presentation are different pipelines).
- Must fire `OnDeath` **exactly once** even under same-frame multi-hit (Edge Case row 2, AC4) — needs an explicit guard, not an implicit one.
- Must serve two distinct, non-related consumer classes from one producer mechanism: `AMoonCharacterBase` (player — checkpoint restore) and `AMoonEnemyCharacterBase` (enemy — BT Dead-state). ADR-0006 already committed these as separate `ACharacter` hierarchies with no shared intermediate base class.
- `TryExecute()` (Rule 9) must reuse the same death-detection path rather than duplicating it, while still emitting a distinct `OnExecuted` (mana-refund subscribers care specifically about execution, not any death).
- Runtime `MaxHealth` changes (TR-hp-008) must re-clamp `Health` without ever triggering Death from that clamp alone (Edge Case row 7) — this must be true by construction, not by a conditional that could regress.

### Requirements

- TR-hp-006 — immediate, presentation-decoupled `Health<=0` → Death, same frame.
- TR-hp-007 — expose `OnDeath`, `OnExecuted`, `OnHealthPercentCrossed`, Health change delegates.
- TR-hp-008 — runtime `MaxHealth` re-clamp, absolute preservation, downward clamp must not itself kill.
- Rule 9 — `TryExecute(Target)` API, `State.Executable`-gated, defense-bypass, exactly-0 Health, `OnExecuted` on success.
- Rule 10 — `OnHealthPercentCrossed(threshold)`, example thresholds 50%/25%.

## Decision

1. **Death detection lives in `UMoonAttributeSet::PostGameplayEffectExecute`.** This is the GAS-idiomatic seam for post-modification detection and is where the existing clamp logic already runs (ADR-0001). On the Health attribute's post-execute callback: clamp `Health` to `[0, MaxHealth]`; if the clamped value is `<= 0` **and** `bIsDead == false`, set `bIsDead = true` and resolve `Data.Target.AbilitySystemComponent->GetAvatarActor()`.

2. **New interface `IMoonHealthEventInterface`** (`UINTERFACE`), implemented by both `AMoonCharacterBase` and `AMoonEnemyCharacterBase`. Step 1 calls `IMoonHealthEventInterface::Execute_HandleDeath(Avatar)`. Each concrete class's `HandleDeath_Implementation()`:
   - Broadcasts that class's own `FOnMoonDeath OnDeath` multicast delegate — this is what ADR-0002 calls (`AMoonCharacterBase::OnDeath()`-shaped) and what ADR-0006 subscribes to ("Character subscribes to Health/Damage Core's `OnDeath`"). Both existing assumptions read correctly once `OnDeath` is a per-class delegate backed by this shared mechanism.
   - Grants a new `State.Dead` GameplayTag and removes all overlay tags this system owns (`State.Invulnerable`, `State.Executable`) via a full-count `RemoveLooseGameplayTagCount`, not a single decrement — satisfies the "사망 시 오버레이 태그 자동 소멸" edge case regardless of how many systems still hold a reference count on those tags.
   - Does **not** itself route to checkpoint restore, ragdoll, or any other class-specific behavior — that stays owned by the subscriber (ADR-0002, ADR-0006), matching Rule 7's "이 시스템은 사망 판정까지만 소유."

3. **`State.Dead` gates further damage the same way `State.Invulnerable` already does** — the base damage GameplayEffect's `ApplicationRequirement` (already checking `State.Invulnerable` per ADR-0001) additionally checks `!HasTag(State.Dead)`. This reuses Rule 8's existing i-frame mechanism (block at `ApplicationRequirement`, not a Health-floor trick) instead of inventing a second "is this thing dead" check elsewhere.

4. **`OnExecuted` is broadcast by `TryExecute()`, not by the AttributeSet.** `AMoonCharacterBase::TryExecute(AActor* Target)`: confirms `Target`'s ASC has `State.Executable`; if present, applies an Instant GameplayEffect with `bBypassDefense=true` whose modifier **Overrides** Health to exactly `0` (not an Additive `-9999`-style subtract) — guarantees "정확히 0" regardless of current Health or shield state (AC7's "실드 존재 여부와 무관하게 성공"). This GE flows through the same Decision 1/2/3 path, so `OnDeath`/`State.Dead`/tag-clear all fire identically. After confirming the GE applied, `TryExecute` broadcasts its own `FOnMoonExecuted OnExecuted(AActor* Target)` — kept separate from `OnDeath` because mana-refund subscribers care specifically about "was this death an execution," not "did the target ever die." Target without `State.Executable`: `TryExecute` returns `false` immediately, no GE, no event (AC8).

5. **`MaxHealth` re-clamp (TR-hp-008) is a different code path, not a conditional inside Decision 1.** `PreAttributeChange` (which fires for *every* attribute write, GE-driven or not) handles `MaxHealth` changes: when `Attribute == GetMaxHealthAttribute()`, reclamp `Health = min(Health, NewValue)` directly, in `PreAttributeChange`, never touching `PostGameplayEffectExecute`'s death branch. This makes "MaxHealth clamp never triggers Death" true by construction — the clamp physically cannot reach the death-detection code — rather than a flag-checked skip that a future edit could silently break.

6. **`bIsDead` reset — required amendment to ADR-0002's Migration Plan.** `UMoonAttributeSet::ResetDeathState()` (new `BlueprintCallable` method: clears `bIsDead`, removes `State.Dead`) must be called by `UMoonCheckpointSubsystem::RestoreCheckpoint` before it re-applies the Health-restore GameplayEffect. Without this, a respawned character's `AttributeSet` still has `bIsDead == true` and the exactly-once guard in Decision 1 permanently suppresses all future `OnDeath` for that instance. This is additive to ADR-0002 (one new call in its existing restore sequence), not a contradiction of its `forbidden_patterns: direct_attributeset_write_on_restore` entry — restore still only ever mutates Health via GameplayEffect application.

### Architecture Diagram

```
UMoonAttributeSet
 ├─ bIsDead : bool (guard, default false)
 ├─ HealthPercentThresholds : TArray<float> (EditDefaultsOnly, default {0.5, 0.25})
 ├─ PreAttributeChange(MaxHealth) ──────────► Health = min(Health, NewMaxHealth)   [never touches death branch]
 ├─ PostGameplayEffectExecute(Health) ──────► clamp [0,MaxHealth]
 │                                             ├─ crosses HealthPercentThresholds? → OnHealthPercentCrossed(threshold) per crossed step
 │                                             └─ NewHealth<=0 && !bIsDead? → bIsDead=true → IMoonHealthEventInterface::HandleDeath(Avatar)
 └─ ResetDeathState() ──────────────────────► bIsDead=false, remove State.Dead   [called by ADR-0002 restore path]

IMoonHealthEventInterface (UInterface)
 └─ HandleDeath()

AMoonCharacterBase : implements IMoonHealthEventInterface        AMoonEnemyCharacterBase : implements IMoonHealthEventInterface
 ├─ FOnMoonDeath OnDeath (broadcast in HandleDeath_Implementation) ├─ FOnMoonDeath OnDeath (broadcast in HandleDeath_Implementation)
 ├─ HandleDeath_Implementation(): OnDeath.Broadcast(),             ├─ HandleDeath_Implementation(): OnDeath.Broadcast(),
 │    grant State.Dead, RemoveLooseGameplayTagCount(overlays)      │    grant State.Dead, RemoveLooseGameplayTagCount(overlays)
 ├─ TryExecute(AActor* Target) -> bool                              └─ (subscribed to by ADR-0006's death handler — StopLogic/ragdoll)
 │    └─ on success: FOnMoonExecuted OnExecuted(Target)
 └─ (subscribed to by ADR-0002's checkpoint restore)

Damage GE ApplicationRequirement: !HasTag(State.Invulnerable) && !HasTag(State.Dead)   [reuses ADR-0001's existing gate mechanism]
```

### Key Interfaces

```
IMoonHealthEventInterface (UInterface)
    void HandleDeath()  // called exactly once by UMoonAttributeSet::PostGameplayEffectExecute

UMoonAttributeSet
    bool bIsDead = false
    TArray<float> HealthPercentThresholds = {0.5, 0.25}
    void ResetDeathState()  // BlueprintCallable — clears bIsDead + State.Dead; required call in ADR-0002 restore path
    FOnMoonHealthPercentCrossed OnHealthPercentCrossed  // (float Threshold)

AMoonCharacterBase / AMoonEnemyCharacterBase  (both implement IMoonHealthEventInterface)
    FOnMoonDeath OnDeath  // multicast, broadcast from HandleDeath_Implementation

AMoonCharacterBase
    bool TryExecute(AActor* Target)
    FOnMoonExecuted OnExecuted  // (AActor* Target) — broadcast by TryExecute after confirmed death

GameplayTag: State.Dead  // NEW — damage GE ApplicationRequirement gate, alongside existing State.Invulnerable
```

## Alternatives Considered

### Alternative 1: Detect death inside `UMoonDamageExecCalc` (the ExecutionCalculation)

- **Description**: Have the damage `ExecutionCalculation` itself check the post-modifier Health value and fire the death event.
- **Pros**: Co-located with the existing shield/`bBypassDefense` logic from ADR-0001.
- **Cons**: `ExecutionCalculation` runs *before* the attribute's own clamp is applied — it would need to duplicate `PostGameplayEffectExecute`'s clamp math to know the true final value, and `TryExecute`'s direct Health-Override path (Decision 4) wouldn't naturally reuse it either, making it a worse single point of truth than the AttributeSet.
- **Rejection Reason**: `PostGameplayEffectExecute` is the correct GAS-idiomatic seam for "after this attribute actually changed" regardless of which GameplayEffect caused it — the ExecCalc is about *how much* damage applies, not what happens after it lands.

### Alternative 2: Tick-based/polling Health check

- **Description**: A `HealthComponent`-style wrapper that checks `Health <= 0` once per Tick instead of hooking into GAS's synchronous callback.
- **Pros**: Simpler mental model, no GAS callback plumbing.
- **Cons**: Directly violates Rule 5 ("사망 판정은 즉시, 애매함 없음" — no delayed judgment) and contradicts the same-frame philosophy already established for hitstop (`player-movement.md` Rule 9).
- **Rejection Reason**: Explicitly forbidden by the GDD's own design intent; not a real contender.

### Alternative 3: Shared `AMoonHealthCharacterBase` intermediate class instead of an interface

- **Description**: Introduce a common base class above both `AMoonCharacterBase` and `AMoonEnemyCharacterBase` that owns `OnDeath` directly.
- **Pros**: Slightly less boilerplate than an interface (no `Execute_`/`_Implementation` indirection).
- **Cons**: ADR-0006 already committed `AMoonEnemyCharacterBase` as an independent `ACharacter` subclass with no shared intermediate base. Retrofitting a common base now touches both already-Proposed/partially-implemented hierarchies — larger, riskier refactor than adding an interface both classes can each independently implement without restructuring their inheritance.
- **Rejection Reason**: Cost/risk not justified for two consumer classes; revisit only if a third Health-owning actor type appears (e.g. destructible-with-health) and the interface pattern starts feeling repetitive.

## Consequences

### Positive

- Closes C-1: gives ADR-0002, ADR-0004, and ADR-0006 a real producer to cite instead of an assumed one. Unblocks all three toward Acceptance.
- Single `PostGameplayEffectExecute` branch handles Death, `OnHealthPercentCrossed`, and (via `TryExecute`'s reuse) `OnExecuted`'s death half — one code path to verify against ACs 4/5/7/8/9/10, not three.
- `State.Dead` reuses the exact same `ApplicationRequirement` gating mechanism as the existing `State.Invulnerable` i-frame gate (ADR-0001) — no new damage-blocking mechanism introduced.
- `bIsDead`/`PreAttributeChange`-vs-`PostGameplayEffectExecute` split makes the "MaxHealth clamp never kills" contract (TR-hp-008) true by construction rather than by a checked flag.

### Negative

- Adds a new interface (`IMoonHealthEventInterface`) and a new tag (`State.Dead`) as additional surface area on top of an already-accreting `AMoonCharacterBase` (flagged separately as C-4 in the 2026-07-27 review — this ADR adds to that accretion, not resolves it).
- `ResetDeathState()` is a required, easy-to-forget integration point for any future respawn/revive path (not just ADR-0002's checkpoint restore) — every future consumer of GAS-based revival must remember to call it.

### Risks

- **GAS callback signature drift (HIGH engine risk, unverified)**: `PostGameplayEffectExecute`/`PreAttributeChange` signatures are assumed unchanged from pre-5.8 GAS — not independently confirmed against real 5.8 headers by this project's engine-reference library. Mitigation: verify at implementation time (same `ue-gas-specialist` cross-check ADR-0001 already flags as required before its own attribute-init pattern ships); if either signature changed, this ADR's Decision 1/5 split needs re-validation, not necessarily a different architecture.
- **`RemoveLooseGameplayTagCount` full-count semantics unverified**: `health-damage-core.md` Edge Case row 5 requires ref-counted tags so one system's cleanup doesn't stomp another's grant; this ADR's blanket clear-on-death assumes a "remove all references at once" call exists and behaves as expected. Mitigation: same verification pass as above.
- **`ResetDeathState()` is a manual integration point, not enforced by the type system**: if a future respawn/revive path forgets to call it, that instance's `OnDeath` silently never fires again. Mitigation: documented here and as a required Migration Plan step for ADR-0002; no automatic enforcement exists — accepted as a known sharp edge rather than over-engineering a callback registry for a single current caller.

## GDD Requirements Addressed

| GDD Document | Requirement | How This ADR Satisfies It |
|---|---|---|
| health-damage-core.md | TR-hp-006 (immediate, presentation-decoupled `Health<=0`→Death, same frame) | Decision 1 — synchronous `PostGameplayEffectExecute` branch, no tick/delay |
| health-damage-core.md | TR-hp-007 (`OnDeath`/`OnExecuted`/`OnHealthPercentCrossed`/Health-change delegates) | Decision 2 (`OnDeath` via interface dispatch), Decision 4 (`OnExecuted`), Decision 1 (`OnHealthPercentCrossed`); Health-change delegate already exists per ADR-0001 (`GetGameplayAttributeValueChangeDelegate`) |
| health-damage-core.md | TR-hp-008 (runtime `MaxHealth` re-clamp, absolute preservation, clamp must not kill) | Decision 5 — separate `PreAttributeChange` code path, structurally cannot reach the death branch |
| health-damage-core.md | Rule 9 (`TryExecute`, defense-bypass, exact 0, `OnExecuted`) | Decision 4 — Override-to-0 GE + dedicated `OnExecuted` broadcast |
| health-damage-core.md | Rule 10 (`OnHealthPercentCrossed`, example 50%/25%) | Decision 1 — `HealthPercentThresholds` default `{0.5, 0.25}` |
| health-damage-core.md | Edge Case row 2 (dup-hit same frame → `OnDeath` exactly once) | `bIsDead` guard (Decision 1) |
| health-damage-core.md | Edge Case row 5 (ref-counted overlay tags, no premature clear) | Full-count `RemoveLooseGameplayTagCount` (Decision 2), not a single decrement |
| health-damage-core.md | Edge Case row 7 (`MaxHealth` runtime change, clamp ≠ Death) | Decision 5 |

## Performance Implications

- **CPU**: One additional branch (guard check + threshold-array iteration, max 2 elements by default) per `PostGameplayEffectExecute` call on the Health attribute. Negligible — same order of cost as the clamp logic already present.
- **Memory**: One `bool`, one small `TArray<float>` (2 entries default), one new GameplayTag, one new UInterface — negligible.
- **Load Time**: None.
- **Network**: Out of scope (no multiplayer requirement in any GDD to date, consistent with ADR-0001/0002).

## Migration Plan

1. Add `bIsDead`, `HealthPercentThresholds`, `ResetDeathState()` to `UMoonAttributeSet`; extend `PostGameplayEffectExecute` (Decision 1) and `PreAttributeChange` (Decision 5).
2. Add `IMoonHealthEventInterface` (UInterface + `HandleDeath`); implement on `AMoonCharacterBase` and `AMoonEnemyCharacterBase` (`FOnMoonDeath OnDeath` + tag-clear + `State.Dead` grant per Decision 2).
3. Register `State.Dead` GameplayTag; add `!HasTag(State.Dead)` to the base damage GE's `ApplicationRequirement` alongside the existing `State.Invulnerable` check.
4. Implement `TryExecute`'s Override-to-0 GE and `OnExecuted` broadcast on `AMoonCharacterBase` (Decision 4).
5. **Amend ADR-0002**: add a call to `UMoonAttributeSet::ResetDeathState()` in `UMoonCheckpointSubsystem::RestoreCheckpoint`, immediately before the existing Health-restore GameplayEffect application. No other change to ADR-0002's Decision or Migration Plan.
6. Re-run `/architecture-review` in a fresh session to confirm TR-hp-006/007/008 flip to ✅ and C-1 clears.

**Rollback plan**: All additions are additive (new fields/interface/tag on existing classes) — revert by removing the interface implementation and the two new hooks; no existing field or method signature changes.

## Validation Criteria

- `health-damage-core.md` ACs 4, 5, 7, 8, 9, 10 pass in PIE (or automated GAS test harness once `tests/unit/` has real content).
- Same-frame multi-hit on an already-`bIsDead` target produces exactly one `OnDeath` broadcast (AC4 / Edge Case row 2).
- `TryExecute` on a target without `State.Executable` returns `false` with zero side effects (AC8).
- A `MaxHealth` downward change that would otherwise cross 0 does not broadcast `OnDeath` (Edge Case row 7 / TR-hp-008).
- Checkpoint restore (ADR-0002) followed by lethal damage produces a fresh `OnDeath` — confirms `ResetDeathState()` integration.
- Re-run `/architecture-review` — TR-hp-006/007/008 ✅, C-1 resolved.

## Related Decisions

- ADR-0001 (Player Movement and GAS Core) — this ADR extends `UMoonAttributeSet`/`AMoonCharacterBase` established there; does not change any of ADR-0001's own decisions.
- ADR-0002 (Checkpoint Persistence, Accepted) — requires the one-step amendment in Migration Plan item 5 (`ResetDeathState()` call).
- ADR-0004 (Luna Overdrive Fixed Window, Proposed) — its `PlayerDeath` end-reason can now subscribe to a real `OnDeath`.
- ADR-0006 (Enemy AI Behavior Tree, Proposed) — its entire Dead-state branch can now subscribe to a real `OnDeath`.
- `design/gdd/health-damage-core.md` — primary GDD source (Rules 5, 7, 8, 9, 10; Edge Case rows 2, 3, 4, 5, 7; ACs 4, 5, 6, 7, 8, 9, 10).
- `docs/architecture/architecture-review-2026-07-27.md` — finding C-1, the blocking conflict this ADR resolves.
