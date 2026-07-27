# ADR-0011: Combo/Tension Gauge — Gain/Penalty/Decay Ordering and Wiring Contract

## Status
Accepted

**Accepted:** 2026-07-27 (fresh `/architecture-review` delta pass in
`architecture-review-2026-07-27-v4.md` — closes TR-tension-002/003/004/005/007,
no unresolved blocking dependency; all `Depends On` ADRs are already Accepted.
UE5.8 `TG_PostUpdateWork` and `FOnAttributeChangeData::GEModData` assumptions
verified 2026-07-27 in `ue58-api-verification-adr-0010-0011-2026-07-27.md`.)

## Date
2026-07-27

## Engine Compatibility

| Field | Value |
|-------|-------|
| **Engine** | Unreal Engine 5.8 |
| **Domain** | Core / Gameplay (GAS `AttributeSet` + Actor `Tick`-based accumulation) |
| **Knowledge Risk** | LOW-MEDIUM — the API surface is verified against local UE5.8 headers; future latent AbilityTask or Blueprint tick work can still violate the intended ordering if it schedules Tension gain at or after `TG_PostUpdateWork`. |
| **References Consulted** | `docs/engine-reference/unreal/VERSION.md`; existing project code (`MoonCharacterBase.cpp`/`.h`, `MoonAttributeSet.cpp`/`.h`); local UE5.8 headers/source under `C:\Program Files\Epic Games\UE_5.8`; `ue58-api-verification-adr-0010-0011-2026-07-27.md` |
| **Post-Cutoff APIs Used** | `TG_PostUpdateWork` and `FOnAttributeChangeData::GEModData`, both verified against local UE5.8 headers/source on 2026-07-27. |
| **Verification Required** | Complete for current story-readiness gating. `GEModData` is null on direct/replication dirty broadcasts and populated from GameplayEffect modifier callback data on GE-originated numerical attribute updates. `TG_PostUpdateWork` exists and runs after `TG_PostPhysics` and before `TG_LastDemotable`; current planned spell-hit / Just-Dodge gain call sites execute before it. Future latent `AbilityTask` completions or Blueprint ticks at `TG_PostUpdateWork`/`TG_LastDemotable` must not call `AddTension*` unless ordered before `TensionResolveTickFunction` by prerequisite. |

## ADR Dependencies

| Field | Value |
|-------|-------|
| **Depends On** | ADR-0001 (Accepted, `TensionGauge`/`TensionGaugeMax` attributes + clamp), ADR-0004 (Accepted, `TriggerOverdrive()`/`IsTensionGainLocked()` — this ADR's Decision 1 explicitly builds on, not around, ADR-0004's already-ratified fusion of "Tension reaches Max" into a direct `TriggerOverdrive()` call), ADR-0003 (Accepted, Spell Casting — `AddTensionFromSpellHit` call sites), ADR-0007 (Accepted, Dash/Evasion — Just-Dodge success, not yet wired to Tension), ADR-0008 (Accepted, Health/Damage Core — `OnDeath` + Health attribute-change delegate, both reused here) |
| **Enables** | Closes the last Feature-layer coverage gap group from `architecture-review-2026-07-27-v3.md` (TR-tension-002/003/004/005/007) — after this ADR and ADR-0010, the project has full architecture coverage. |
| **Blocks** | None. |
| **Ordering Note** | Independent of ADR-0010 (Combat HUD) — that ADR only *reads* `TensionGauge`/`TensionGaugeMax` (already placed by ADR-0001), nothing here changes that read contract. |

## Context

### Problem Statement

`combo-tension-gauge.md` (Approved) fully specifies the gain/decay/penalty/lock rules for the Tension gauge, but no ADR governs it — `architecture-review-2026-07-27-v3.md` scored it at 1✅/1⚠️/5❌ of 7 requirements, tied with Combat HUD as the last remaining gap before full project coverage.

Unlike a from-scratch design, a substantial implementation already ships directly on `AMoonCharacterBase` — `AddTension()`, `AddTensionFromSpellHit()`, decay inside `Tick()`, and (via ADR-0004, already Accepted) `TriggerOverdrive()` as the ratified mechanism for "Tension reaches Max." Auditing that code against `combo-tension-gauge.md` found:

- **Correct and to be ratified**: gain/clamp formula (`AddTension`), decay formula and its deliberate *absence* of an `State.Invulnerable` special-case (Rule 3 explicitly requires decay to keep running through invulnerability — the shipped code's silence on that tag is correct, not an oversight), and the Overdrive-trigger fusion already Accepted by ADR-0004.
- **Deviation, not yet a problem in practice but latent**: the GDD specifies Spell Casting/Dash/Health-Damage-Core *subscribe* Tension to their events ("구독"); shipped code instead has those systems call directly into `AMoonCharacterBase`'s `AddTensionFromSpellHit`/`AddTensionFromJustDodge`/`ApplyTensionDamagePenalty` methods.
- **Gap — same-frame ordering is not actually guaranteed**: `AddTensionFromJustDodge()` (would be called by Dash/Evasion) and `ApplyTensionDamagePenalty()` (would be called by Health/Damage Core) are two independent call sites with no ordering relationship to each other — the GDD's Edge Case (AC9) requires a fixed Gain→Penalty→Decay resolution regardless of which system's code happens to run first in a given frame.
- **Gap — two of the three trigger paths are entirely unwired**: `AddTensionFromJustDodge()` and `ApplyTensionDamagePenalty()` are declared, `BlueprintCallable`, and never called from anywhere in the codebase. Dash/Evasion's Just-Dodge success path and Health/Damage Core's player-damage path don't invoke them.
- **Gap — no Death reset of the attribute itself**: nothing zeroes `TensionGauge` on player Death. (ADR-0010's Combat HUD `OnDeath` handler only resets the *display* — if the underlying attribute isn't also reset, the next real `TensionGauge` change would push the stale pre-death value right back to the HUD.)

### Constraints

- `TensionGauge` is never directly writable by player input — purely event-reactive (Rule 1).
- Decay must keep running through `State.Invulnerable` — no exception (Rule 3).
- Damage Penalty triggers only on effective damage received (`>0`), independent of and in addition to Decay (Rule 4).
- Same-frame resolution order is fixed: Gain → Penalty → Decay, regardless of real-world trigger order (Edge Cases, AC9).
- Tension gain is fully locked (not partially multiplied) during Luna Overdrive Active or Recovery, gated on Luna Overdrive's own time state — already correctly implemented via `OverdriveState.IsTensionGainLocked()` (ADR-0004).
- Player Death forces `TensionGauge` to `0` immediately, no carry-over across respawn (Edge Cases, AC8).
- No new upstream interfaces where an existing one already covers the need (project-wide principle, same as TR-hud-002 for Combat HUD).

### Requirements

- TR-tension-002 — Read-only reactive consumption of `OnSpellHit`/`OnTagAdded(State.Executable)`/`OnDamageApplied`/Death, without modifying those systems.
- TR-tension-003 — Decay timer with grace-period gating, continues through `State.Invulnerable`.
- TR-tension-004 — Deterministic same-frame ordering Gain → Penalty → Decay, clamp `[0, Max]`, no carry-over.
- TR-tension-005 — `OnOverdriveTriggered` exactly once at Max, reset to 0 same frame, immediate re-trigger permitted.
- TR-tension-007 — Expose read-only gauge value + Building/Decaying state downstream.

## Decision

1. **TR-tension-005 is already closed by ADR-0004 — formally traced, not re-decided.** ADR-0004 Decision 1 ("Reaching max tension calls `TriggerOverdrive()`") already satisfies this requirement: the GDD's `OnOverdriveTriggered` event is functionally realized as a direct, same-class method call rather than a broadcast delegate, and `AddTension()`'s existing `if (NewTension >= MaxTension) { SetTensionGauge(0.0f); TriggerOverdrive(); }` block delivers exactly-once-per-Max-crossing, same-frame-reset semantics. The deprecated `OnOverdriveTriggered()` `BlueprintImplementableEvent` stub (marked `DeprecatedFunction` in code) is dead and should be removed, not resurrected — reviving it would reintroduce the exact two-mechanism redundancy ADR-0004 already resolved.

2. **TR-tension-002 — direct method calls are ratified, not converted to pub/sub.** `AddTensionFromSpellHit`/`AddTensionFromJustDodge`/`ApplyTensionDamagePenalty` staying `BlueprintCallable` methods on `AMoonCharacterBase` (called directly by Spell Casting abilities, and — after Decision 4's wiring — by Dash/Evasion and Health/Damage Core) is correct and is not converted to an event-subscription architecture. Rationale: `TensionGauge` already lives on the same Character/AttributeSet that Overdrive state lives on (ADR-0001/0004 precedent), and every calling system (Spell abilities, Dash ability, HDC's damage pipeline) already holds a direct reference to the player's `AMoonCharacterBase` at the moment the triggering action resolves — inventing a broadcast-and-subscribe layer for a write into the *same actor's own* attribute would add indirection with no decoupling benefit (unlike Combat HUD, a genuinely separate, decoupled consumer, where event subscription is the right call — ADR-0010 Decision 3's TR-hud-002 reasoning does not transfer here). This is not a new upstream interface being invented; TR-tension-002's "without modifying those systems" clause is satisfied because none of the three call sites require any change to Spell Casting's, Dash's, or HDC's own decision logic — they just gain one additional line calling an already-`BlueprintCallable`, already-existing method.

3. **TR-tension-004 — same-frame Gain→Penalty→Decay ordering is enforced by resolving Penalty+Decay in a dedicated, late-tick-group function, not by leaving them in `AMoonCharacterBase`'s normal `Tick()`.** Because `AddTensionFromJustDodge()` (Dash) and `ApplyTensionDamagePenalty()` (Health/Damage Core) are independent call sites with no inherent relative-ordering guarantee within a frame, a naive "defer Penalty into the Character's regular `Tick()`" fix is not actually sufficient by itself: if a Gain-triggering call (e.g. a Just-Dodge ability activation) happens to fire *after* that `Tick()` call in the same frame — which the Character's own default tick group does not structurally rule out — a Penalty flagged earlier that frame would already have resolved before the late Gain, producing Penalty-then-Gain instead of the mandated Gain-then-Penalty (materially different results: (60×0.8)+20=68 vs. mandated (60+20)×0.8=64). This ADR closes that hole structurally rather than accepting the race:
   - `ApplyTensionDamagePenalty()` sets a `bPendingDamagePenaltyThisFrame = true` flag (idempotent — multiple same-frame calls are a no-op after the first, since the penalty is proportional, not additive, and Edge Cases don't require stacking multiple hits' penalties within one frame differently from one).
   - Gain stays exactly as-is: `AddTension()`/`AddTensionFromSpellHit()`/`AddTensionFromJustDodge()` remain immediate/synchronous, unchanged. This is deliberate, not an oversight — deferring Gain into a summed/queued value would break Rule 6's "same-frame duplicate-trigger prevention" edge case: each `AddTension()` call must independently check the Overdrive-trigger-on-Max condition against *its own* post-gain value (a mid-frame Overdrive trigger resets the gauge to 0 and locks further gain immediately, so a second same-frame Gain call correctly becomes a no-op via `IsTensionGainLocked()` rather than summing with the first) — collapsing multiple Gain events into one deferred sum would evaluate the Overdrive-trigger boundary once instead of per-event, changing this behavior.
   - The Penalty-application-then-Decay pair is moved out of the Character's default-tick-group `Tick()` into a dedicated `FTickFunction` (`TensionResolveTickFunction`) explicitly registered at `TG_PostUpdateWork` — the latest standard Unreal tick group, running after `TG_PrePhysics`/`DuringPhysics`/`PostPhysics` and therefore after normal input processing, ability activation, and AnimNotify firing for that frame. Any Gain that fires during this frame's ordinary gameplay processing (Just-Dodge activation, spell-hit AnimNotify) executes before `TG_PostUpdateWork` runs, so the Penalty-then-Decay resolution deterministically sees the frame's final Gain state before applying the proportional penalty and then decay — closing the ordering hole rather than merely hoping it doesn't occur.
   - This function must read `CurrentTension` *after* applying the pending penalty, not before — the existing shipped `Tick()` block currently caches `CurrentTension` once at the top before the decay grace-period check; that cached read must move to occur after the new penalty-application step, or Decay would silently operate on a pre-penalty value (see Migration Plan item 2).
   - Worst-case added latency versus the pre-ADR (never-called) `ApplyTensionDamagePenalty()` is bounded to within the same frame (a later tick group, not a later frame) — no perceptible display lag, and Combat HUD's `DisplayedTension` lerp (ADR-0010) already visually smooths any sub-frame-group timing difference.
   - **Verified implementation precondition (2026-07-27)**: the current planned Gain-triggering call sites execute before `TG_PostUpdateWork`, and UE5.8 runs `TG_PostUpdateWork` after `TG_PrePhysics`/`DuringPhysics`/`PostPhysics`. Future latent `AbilityTask` completions or Blueprint events scheduled at `TG_PostUpdateWork`/`TG_LastDemotable` must not call `AddTension*` unless they add an explicit prerequisite ordering before `TensionResolveTickFunction`.

4. **Wire the two unconnected trigger paths — additive calls into already-existing methods, no new interfaces.** Dash/Evasion's Just-Dodge success handler (`AMoonGameplayAbility_Dash`'s existing just-dodge-confirmed branch, per ADR-0007) gets one new line: `MoonCharacter->AddTensionFromJustDodge()`. Health/Damage Core's player-damage path gets one new line calling `ApplyTensionDamagePenalty()` — specifically, from the same Health attribute-change delegate Combat HUD (ADR-0010 W1) already subscribes to, filtered for decreases (Decision 5).

5. **Damage Penalty trigger reuses the existing Health attribute-change delegate, gated on GameplayEffect origin — no new `OnDamageApplied` interface.** `combo-tension-gauge.md` names `OnDamageApplied` as an interface Health/Damage Core has not yet exposed ("상류 문서가 '추후' 노출 예정으로 명시한 인터페이스" — the GDD itself flags this as provisional). Rather than adding a new delegate to ADR-0008's already-Accepted surface, this ADR satisfies the requirement with the interface that already exists: `UMoonAbilitySystemComponent`'s generic `GetGameplayAttributeValueChangeDelegate(GetHealthAttribute())` (the same one Combat HUD's `HandleHealthChanged` already binds). `AMoonCharacterBase` subscribes its own handler to this delegate; it calls `ApplyTensionDamagePenalty()` (arming the Decision 3 pending-flag) only when **both** `Data.NewValue < Data.OldValue` (a decrease) **and** `Data.GEModData != nullptr` (the change originated from a GameplayEffect execution).
   The second condition is required, not optional: ADR-0008 Decision 5 handles `TR-hp-008` (runtime `MaxHealth` re-clamp) by writing `Health = min(Health, NewMaxHealth)` directly inside `PreAttributeChange(MaxHealth)` — a genuine Health-attribute decrease with no damage GE involved, which would otherwise spuriously trip this ADR's damage-penalty filter on every `MaxHealth` shrink. `FOnAttributeChangeData::GEModData` is non-null when the change came from a `GameplayEffect` numerical attribute update and null for direct/replication dirty-broadcast paths, verified against local UE5.8 `GameplayEffectTypes.h` and `GameplayEffect.cpp` on 2026-07-27 — so gating on both conditions together excludes the reclamp while still catching real damage hits.
   No filtering for `State.Invulnerable` is needed here: HDC's damage `GameplayEffect` `ApplicationRequirement` already blocks the Health change entirely while `State.Invulnerable` is held (ADR-0001/0008), so an invulnerable hit never reaches this delegate at all — the gate naturally prevents it, matching this project's established "let the upstream block do the work, don't re-check downstream" pattern (same reasoning ADR-0009 used for Ascending/Falling re-entry). A same-frame Death (Health hits exactly 0) still fires this delegate once before `OnDeath` — Decision 6's Death-reset runs afterward and zeroes the gauge regardless, so no ordering conflict with Decision 3's Gain→Penalty→Decay sequence (Death is not one of that sequence's three steps; it's an unconditional override that always wins, same precedent as ADR-0004's "player Death takes eager priority").

6. **Death resets `TensionGauge` to exactly 0 — added inline to `HandleDeath_Implementation()`, not via a self-subscription to its own broadcast.** `AMoonCharacterBase::HandleDeath_Implementation()` (ADR-0008's existing `IMoonHealthEventInterface` implementation, which already broadcasts `OnDeath` and grants `State.Dead`) gets one additional inline step: `AttributeSet->SetTensionGauge(0.0f)`, plus clearing `bPendingDamagePenaltyThisFrame`/`LastTensionGainTime`. This is deliberately *not* implemented as `AMoonCharacterBase` subscribing to its own `OnDeath` delegate — that would be a round-trip through a broadcast for a same-class, same-function write with no decoupling benefit, exactly the indirection Decision 2 already argues against for same-actor state. This is separate from and in addition to `MoonCombatHUDWidget`'s own `OnDeath` *subscription* (ADR-0010 Decision 6, which only resets the HUD's *display* — that one is correctly a subscription, since the HUD is a genuinely separate consumer). Doing the attribute reset inline here closes the gap where the HUD's display-only reset would otherwise be silently overwritten by the next real (stale, pre-death) `TensionGauge` value.

### Architecture Diagram

```
AMoonCharacterBase
 ├─ AddTension(Amount)                  — immediate/synchronous (unchanged, deliberately not deferred — Decision 3)
 │    └─ if NewTension >= Max: SetTensionGauge(0), TriggerOverdrive()   [ADR-0004, TR-tension-005 — already Accepted]
 ├─ AddTensionFromSpellHit(ManaCost)     — calls AddTension (unchanged, direct-call ratified — Decision 2)
 ├─ AddTensionFromJustDodge()            — calls AddTension
 │    └─ NEW caller: AMoonGameplayAbility_Dash's just-dodge-confirmed branch   [Decision 4]
 ├─ HandleHealthAttributeChanged(Data)   — NEW: subscribes GetGameplayAttributeValueChangeDelegate(Health)
 │    └─ if Data.GEModData != nullptr && Data.NewValue < Data.OldValue: ApplyTensionDamagePenalty()   [Decision 4/5]
 ├─ ApplyTensionDamagePenalty()          — CHANGED: sets bPendingDamagePenaltyThisFrame=true (was: immediate mutation)   [Decision 3]
 ├─ HandleDeath_Implementation()          — EXISTING (ADR-0008), gains one inline addition:
 │    └─ + SetTensionGauge(0.0f), clear bPendingDamagePenaltyThisFrame/LastTensionGainTime   [Decision 6 — inline, not a self-subscription]
 └─ TensionResolveTickFunction : FTickFunction, registered at TG_PostUpdateWork   [NEW — Decision 3, replaces the old in-Tick() Tension block]
      ├─ (a) if bPendingDamagePenaltyThisFrame: TensionGauge *= (1 - DamagePenaltyPercent); clear flag
      └─ (b) Decay — existing grace/rate logic, moved here from the old Tick(); CurrentTension read occurs AFTER (a), not cached before it

DELETED: OnOverdriveTriggered() BlueprintImplementableEvent stub (dead, DeprecatedFunction — Decision 1)
```

### Key Interfaces

```
AMoonCharacterBase (existing, unchanged signatures)
    void AddTension(float Amount)
    void AddTensionFromSpellHit(float ManaCost)
    void AddTensionFromJustDodge()
    void ApplyTensionDamagePenalty()    // behavior changed (Decision 3): sets a pending flag instead of mutating immediately —
                                        // doc-comment must state this is now deferred-to-tick-group, not synchronous

AMoonCharacterBase (new private members)
    bool bPendingDamagePenaltyThisFrame = false                              // NEW — Decision 3
    void HandleHealthAttributeChanged(const FOnAttributeChangeData& Data)    // NEW — Decision 5, subscribed in InitializeAttributes/BeginPlay
    struct FTensionResolveTickFunction : public FTickFunction { ... }        // NEW — Decision 3, TickGroup = TG_PostUpdateWork

REMOVED
    UFUNCTION(BlueprintImplementableEvent) void OnOverdriveTriggered()   // dead code, Decision 1
```

## Alternatives Considered

### Alternative 1 (TR-tension-005): Resurrect `OnOverdriveTriggered`, have Luna Overdrive subscribe to it

- **Description**: Un-deprecate the existing stub, broadcast it from `AddTension()`'s Max-reached branch, and have `TriggerOverdrive()` become a subscriber to it instead of being called directly.
- **Pros**: Matches the GDD's literal event-based wording; decouples Tension from Overdrive at the code level.
- **Cons**: ADR-0004 already made and justified the opposite call — reopening it here would contradict an Accepted ADR for a purely cosmetic gain (both mechanisms deliver identical same-frame, exactly-once semantics; the "decoupling" benefit is theoretical, since Tension and Overdrive state already live on the same `AMoonCharacterBase` instance with no plausible future need to run in separate actors).
- **Rejection Reason**: Would reverse a considered, Accepted decision (ADR-0004) without new information — same standard this project already applies (e.g., ADR-0009 Alternative 2's identical reasoning for not reopening Core Rule 9).

### Alternative 2 (TR-tension-002): Convert all three trigger paths to a full pub/sub event bus

- **Description**: Add `OnSpellHit`, a Just-Dodge success delegate, and the (not-yet-existing) `OnDamageApplied` as real broadcast events on their owning systems; Tension Gauge subscribes to all three instead of being called directly.
- **Pros**: Matches the GDD's literal "구독" wording for all three sources; a textbook decoupled-systems architecture.
- **Cons**: All three producers and the one consumer already live on or directly reference the same `AMoonCharacterBase` instance at the moment the action resolves (Spell abilities operate on the casting Character; Dash's Just-Dodge check operates on the dashing Character; HDC's damage pipeline already fires a Health delegate on that same Character's `AttributeSet`) — adding a broadcast layer between systems that already share a direct reference buys no decoupling, only indirection, and would require touching Spell Casting (ADR-0003), Dash (ADR-0007), and Health/Damage Core (ADR-0008) — three Accepted ADRs — to add delegates none of them currently need for any other consumer.
- **Rejection Reason**: No other current or foreseeable consumer needs these as broadcast events; Combat HUD (the one system that legitimately needs decoupled read access) already gets it for free via the `TensionGauge`/`TensionGaugeMax` GAS attribute delegate, which is a broadcast mechanism by GAS's own design — a second, purpose-built event layer under it would be pure duplication.

### Alternative 3 (TR-tension-004): Order-independent commutative penalty (make Penalty additive instead of proportional-then-deferred)

- **Description**: Instead of deferring `ApplyTensionDamagePenalty()` to `Tick()`, redefine the Damage Penalty as a fixed-point subtraction rather than a proportional multiply, so that Gain-then-Penalty and Penalty-then-Gain produce related-but-different, both-acceptable results, sidestepping the need for a strict ordering guarantee at all.
- **Pros**: No deferred-flag mechanism needed; both call sites stay fully synchronous and independent.
- **Cons**: Directly contradicts the GDD's own Formula (`TensionAfterDamage = CurrentTension × (1 − DamagePenaltyPercent)`, explicitly proportional, not fixed-subtraction) and its explicit Edge Case/AC9, which mandates a specific evaluation order producing a specific number (Gain +20 then Penalty ×0.8, not the reverse) — this alternative would require reopening and reauthoring an already-Approved GDD's formula and Edge Case, not just an implementation detail.
- **Rejection Reason**: Changes game-facing balance math to solve an architecture problem that Decision 3's deferred-flag approach solves without touching the formula at all.

## Consequences

### Positive

- Closes the last 5 requirement gaps in the project (TR-tension-002/003/004/005/007) — after this ADR and ADR-0010, `architecture-review` should have zero remaining coverage gaps.
- TR-tension-005's resolution is pure formalization (zero code change beyond deleting dead code) — the hard part was already solved correctly by ADR-0004.
- Decision 5 avoids reopening ADR-0008 (Accepted) for a new interface, exactly the same discipline ADR-0010's Decision 3 applied to `OnHealthPercentCrossed`/`HealthPercentThresholds`.
- Wiring Just-Dodge and Damage-Penalty (Decision 4) are small, additive, one-line changes to already-Accepted systems' existing success/damage paths — low implementation risk.

### Negative

- The Damage Penalty gains latency (deferred to the `TG_PostUpdateWork` tick function) it didn't have before (when it had no caller at all, so the question was moot) — bounded to within the same frame, not a full frame later, but still not instantaneous. A deliberate, documented trade-off, not a regression from previously-correct behavior.
- `AMoonCharacterBase` gains a second, separately-scheduled tick function (`TensionResolveTickFunction`) alongside its existing `Tick()`, joining the growing list of per-frame concerns already on this class (C-4 from `architecture-review-2026-07-27.md` — Player Movement's timers, Overdrive's countdown, and now Tension's pending-penalty resolution) — same accretion risk ADR-0009/0010 already flagged, not newly introduced by this ADR but incrementally worse, and now spread across two tick entry points instead of one.
- Removing the deprecated `OnOverdriveTriggered()` stub is a small breaking change for any Blueprint graph that still overrides it (none currently exist, per the audit, but worth a grep-before-delete step in Migration Plan).
- `ApplyTensionDamagePenalty()`'s name no longer accurately describes its behavior once Decision 3 changes it from an immediate mutator to a flag-setter — a future caller (C++ or Blueprint) reading only the name/signature could reasonably assume a synchronous effect. Not renamed in this ADR (scope discipline — renaming ripples into any future Blueprint reference), but the doc-comment must say so explicitly (Migration Plan item 8).

### Risks

- **The Penalty/Decay-to-`TG_PostUpdateWork` deferral could theoretically interact badly with a same-frame Death** (player takes lethal damage — Penalty gets queued, then Death fires before the tick function runs). Mitigation: Decision 6's inline Death-path reset unconditionally zeroes `TensionGauge` and clears the pending flag, so a queued-but-never-applied penalty on a dead player is moot — explicitly checked, not assumed.
- **`bPendingDamagePenaltyThisFrame` as a single bool (not a count or accumulated value) assumes at most one "penalty event" per frame matters** — if two separate lethal-adjacent hits land in the same frame from two different enemies, both set the same flag, and only one proportional penalty applies. This matches Rule 4/Edge Cases' treatment of Just-Dodge's "1 event, no matter how many targets" precedent, but the GDD does not explicitly address multiple-hits-same-frame for the *damage* penalty specifically (only for Just-Dodge). Flagged as a genuine open question, not silently resolved — worth a game-designer confirmation before Production that "one penalty per frame regardless of hit count" is the intended read of Rule 4, matching Just-Dodge's precedent rather than requiring per-hit stacking.
- **Grep-before-delete for `OnOverdriveTriggered()`** (Consequences → Negative) is a Migration Plan item, not yet performed as part of this ADR — if any Blueprint asset does override it, that graph silently stops firing once the C++ declaration is removed; must be checked, not assumed absent.
- **`TG_PostUpdateWork`'s ordering guarantee is verified for current planned call sites, but not for arbitrary future latent work.** Future latent `AbilityTask` completions or Blueprint ticks scheduled at `TG_PostUpdateWork`/`TG_LastDemotable` must not call `AddTension*` unless they declare a prerequisite before `TensionResolveTickFunction`.
- **All `TensionGauge` mutations (Gain, Penalty, Decay) go through direct `AttributeSet->SetTensionGauge()` calls, not through a GameplayEffect Modifier/Execution** — a pre-existing pattern (already true of the shipped, unratified code this ADR formalizes) that this ADR extends rather than reverses. This is a real GAS-idiom deviation ("all stat changes should go through Gameplay Effects") — acceptable here because the project is single-player/non-replicated and no other system needs to intercept or predict Tension changes the way damage/mana already flow through GameplayEffects, but it is named explicitly here rather than left implicit, so a future replication or GAS-prediction pass doesn't discover it by surprise.

## GDD Requirements Addressed

| GDD Document | Requirement | How This ADR Satisfies It |
|---|---|---|
| combo-tension-gauge.md | TR-tension-002 (read-only reactive subscription, no upstream modification) | Decision 2 (direct-call pattern ratified) + Decision 4 (wiring) + Decision 5 (reuses existing Health delegate, no new interface) |
| combo-tension-gauge.md | TR-tension-003 (decay continues through invulnerability) | Ratifies existing decay logic (moved into `TensionResolveTickFunction` by Decision 3) — no `State.Invulnerable` special-case, which is correct by the GDD's own Rule 3 |
| combo-tension-gauge.md | TR-tension-004 (deterministic Gain→Penalty→Decay, same-frame) | Decision 3 — Penalty resolved in a dedicated `TG_PostUpdateWork` tick function, structurally ordered after any same-frame Gain |
| combo-tension-gauge.md | TR-tension-005 (`OnOverdriveTriggered` exactly once, same-frame reset) | Decision 1 — formally traced to ADR-0004's already-Accepted `TriggerOverdrive()` direct-call mechanism |
| combo-tension-gauge.md | TR-tension-007 (expose read-only value + Building/Decaying downstream) | Already satisfied by the existing GAS attribute delegate Combat HUD (ADR-0010) consumes — no change needed |

## Performance Implications

- **CPU**: One additional lightweight `FTickFunction` per player Character (negligible — one bool check plus the existing decay math, now just relocated). One new attribute-change delegate subscription (`HandleHealthAttributeChanged`) — same delegate Combat HUD already subscribes to, no new GAS infrastructure.
- **Memory**: None.
- **Load Time**: None.
- **Network**: Out of scope, consistent with all prior ADRs.

## Migration Plan

1. Delete the deprecated `OnOverdriveTriggered()` `BlueprintImplementableEvent` stub from `AMoonCharacterBase` — first grep all Widget/Anim Blueprints for an override, confirm none exist (per the Risks note), then remove (Decision 1).
2. Move the existing `Tick()` Tension-decay block into a new `FTensionResolveTickFunction : public FTickFunction`, registered with `TickGroup = TG_PostUpdateWork` in `AMoonCharacterBase::PostInitializeComponents()` (or equivalent registration point). Change `ApplyTensionDamagePenalty()` to set `bPendingDamagePenaltyThisFrame = true` instead of mutating `TensionGauge` immediately. Inside the new tick function: apply the pending penalty first (if flagged, clear the flag), **then** read `CurrentTension` for the Decay grace/rate check — the existing code currently caches `CurrentTension` before the decay check; that read must move to occur after the penalty step, not before, or Decay would operate on a pre-penalty value (Decision 3).
3. Add `AMoonCharacterBase::HandleHealthAttributeChanged`, subscribed to `GetGameplayAttributeValueChangeDelegate(GetHealthAttribute())` alongside the AttributeSet's other init-time bindings; call `ApplyTensionDamagePenalty()` when `Data.GEModData != nullptr && Data.NewValue < Data.OldValue` — the `GEModData` check excludes ADR-0008's `MaxHealth`-driven `PreAttributeChange` reclamp, which is not a damage hit (Decision 5). `GEModData` null/non-null semantics were verified against local UE5.8 headers/source on 2026-07-27.
4. Add one call to `AddTensionFromJustDodge()` in `AMoonGameplayAbility_Dash`'s existing just-dodge-confirmed branch (Decision 4).
5. Add the `TensionGauge`/pending-flag/`LastTensionGainTime` reset **inline** to `AMoonCharacterBase::HandleDeath_Implementation()` (ADR-0008's existing method — do not add a separate `OnDeath` subscription for this; Decision 6 is explicit that a same-class self-subscription here would be pointless indirection).
6. Update `ApplyTensionDamagePenalty()`'s doc-comment to state it now defers the mutation to the next `TensionResolveTickFunction` pass rather than applying synchronously (Consequences → Negative).
7. Preserve the verified ordering invariant during implementation: no current or future Gain-triggering call site may execute at/after `TG_PostUpdateWork` unless it adds an explicit prerequisite before `TensionResolveTickFunction`.
8. Confirm with `game-designer` whether multiple same-frame damage-penalty triggers should stack or collapse to one (Risks note) before Production sign-off — not blocking for Accept, but should be resolved before final tuning pass.
9. Re-run `/architecture-review` in a fresh session to confirm TR-tension-002/003/004/005/007 flip to ✅ and the project reaches full coverage.

**Rollback plan**: Decisions 2, 4, 5, 6 are additive (new subscriptions/calls, no removed behavior). Decision 3 changes both `ApplyTensionDamagePenalty()`'s timing (immediate → deferred) and its resolution point (Character's default `Tick()` → a dedicated `TG_PostUpdateWork` tick function) — reverting to immediate mutation is possible if the tick-function split proves problematic in practice, though doing so would reopen TR-tension-004's ordering gap exactly as it existed before this ADR. Decision 1's deletion is reversible via version control if a grep-before-delete surprise turns up a live BP reference.

## Validation Criteria

- `combo-tension-gauge.md`'s own Acceptance Criteria 1–10, in particular AC9 (Just-Dodge + same-frame hit — Gain-then-Penalty ordering, now structurally guaranteed by Decision 3's `TG_PostUpdateWork` placement, not merely asserted), AC8 (Death reset, now implemented inline by Decision 6), and AC10 (Overdrive-lock, already correctly implemented via ADR-0004's `IsTensionGainLocked()`). Because Penalty now resolves in a later tick group than the frame's initial damage-application, any test asserting AC6/AC9's post-penalty `TensionGauge` value must read it after that frame's tick functions have all run, not synchronously immediately after applying damage.
- Re-run `/architecture-review` — confirms Combo/Tension Gauge's coverage row moves to full ✅ and the project-wide gap count reaches 0.

## Related Decisions

- ADR-0001 (Player Movement and GAS Core) — origin of the `TensionGauge`/`TensionGaugeMax` attributes this ADR governs the gain/decay/penalty rules for.
- ADR-0003 (Spell Casting GAS Implementation) — `AddTensionFromSpellHit` call sites (unchanged by this ADR).
- ADR-0004 (Luna Overdrive Fixed Window) — owns and already resolved TR-tension-005 (`TriggerOverdrive()` direct-call mechanism); also owns `IsTensionGainLocked()`, consumed unchanged by this ADR's existing Gain path.
- ADR-0007 (Dash/Evasion Just-Dodge) — gains one new call site (`AddTensionFromJustDodge()`) per Decision 4.
- ADR-0008 (Health/Damage Core Death Event Contract) — `OnDeath` (newly consumed here, Decision 6) and the Health attribute-change delegate (newly consumed here for the damage-penalty trigger, Decision 5) — no changes to ADR-0008 itself.
- ADR-0010 (Combat HUD Widget Architecture) — the one existing downstream consumer of `TensionGauge`/`TensionGaugeMax` (read-only, unaffected by this ADR) and the sibling `OnDeath` subscriber whose display-only reset this ADR's Decision 6 complements with the actual attribute reset.
- `design/gdd/combo-tension-gauge.md` — primary GDD source (Core Rules 1–7, Formulas, Edge Cases, Acceptance Criteria).
- `docs/architecture/architecture-review-2026-07-27-v3.md` — the coverage gap this ADR resolves.
