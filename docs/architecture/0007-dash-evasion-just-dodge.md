# ADR-0007: Dash/Evasion — Instant Position Step + Just-Dodge Query Architecture

## Status
Proposed

## Date
2026-07-23

## Engine Compatibility

| Field | Value |
|-------|-------|
| **Engine** | Unreal Engine 5.8 |
| **Domain** | Physics / GAS |
| **Knowledge Risk** | LOW-MEDIUM — `SetActorLocation` (swept), `GameplayAbility`/`ActivationOwnedTags`, and sphere-overlap queries are all stable pre-cutoff APIs. No dedicated physics-module coverage of overlap queries in this project's engine-reference beyond `docs/engine-reference/unreal/modules/physics.md`'s general collision-channel material, which was consulted. |
| **References Consulted** | `docs/engine-reference/unreal/VERSION.md`, `docs/engine-reference/unreal/breaking-changes.md`, `docs/engine-reference/unreal/deprecated-apis.md`, `docs/engine-reference/unreal/modules/physics.md` |
| **Post-Cutoff APIs Used** | None. |
| **Verification Required** | None beyond standard collision-query correctness at implementation time. |

## ADR Dependencies

| Field | Value |
|-------|-------|
| **Depends On** | ADR-0006 (Enemy AI, Proposed) — Just-Dodge detection requires a **queryable** telegraph-state accessor on `AMoonEnemyCharacterBase` that ADR-0006 did not originally specify (that ADR only defined the broadcast delegates). This ADR proposes a small addition to ADR-0006's Key Interfaces (see Decision point 3) rather than re-deciding ADR-0006's architecture. |
| **Enables** | Core Extraction Execution GDD (consumes the `State.Executable` tag this ADR grants) |
| **Blocks** | None currently blocking an epic — closes an architecture-review gap (TR-dash-001..008, 8/8 previously uncovered) |
| **Ordering Note** | Should be Accepted together with or after ADR-0006, since it extends that ADR's interface |

## Context

### Problem Statement
`design/gdd/dash-evasion.md` has been Approved since 2026-07-17 with 9 Core Rules covering the dash charge system, i-frames, and — critically — the Just-Dodge → `State.Executable` mechanic that this project's entire core combat loop (콤보/오버드라이브/처형) is built on top of. It has zero ADR coverage.

Unlike Enemy AI, this is **not** greenfield — a real `UMoonGameplayAbility_Dash` already exists and is substantially implemented and shipped (commits `7f7e2e3` "make dash an instant step", `2e6ba9f` camera/dash tuning). Reading the actual source:

- **Implemented and correct**: charge consumption/gating (Rule 1, `CanActivateAbility` checks `DashCharges >= 1`), `MovementLocked` gate (Rule 9), instant swept `SetActorLocation` position step matching the exact tuning formula (`MaxWalkSpeed × DashSpeedMultiplier × DashDuration` = 300uu default, Rule 2/4), i-frames via `ActivationOwnedTags` granting `State.Invulnerable` for the ability's active lifetime (Rule 5), charge recharge via a Tick-driven formula in `MoonCharacterBase.cpp` that matches the GDD's `CurrentCharges = clamp(CurrentCharges + DeltaTime/RechargeRate, 0, MaxCharges)` formula exactly, and movement-lock-during-dash + restore-on-end (matching Rule 2's "대쉬 지속시간에는 일반 이동 입력을 받지 않는다").
- **Not implemented at all**: `UMoonGameplayAbility_Dash::CheckJustDodge()` is a literal no-op — `// TODO: Iterate over enemies and check their attack commit time vs JustDodgeWindow ... Skeleton implemented for now.` **The entire Just-Dodge → `State.Executable` grant → charge-refund mechanic (Rules 6/7, the mechanic this whole game's core loop depends on) does not exist yet.** This is distinct from and not proven by the Signature Combat Chain Spike (`prototypes/signature-combat-chain-spike-2026-07-21`) — that spike's causal chain was Blackhole→Fire→Fracture→Extract and never exercised Dash or Just-Dodge at all.
- **Not implemented**: Air-dash Z-axis hover impulse (Rule 3's "Z축 체공 시간을 연장하는 미세한 상승... 임펄스") — current `ApplyDashImpulse` only ever moves horizontally; falling state is tracked (`bRestoreFallingMovement`) only to restore movement mode afterward, not to apply any air-specific impulse.

This ADR's primary job is architecting the missing Just-Dodge detection, and secondarily ratifying the already-shipped instant-step mechanism as the formal architectural decision (closing the TR-dash gap) and flagging the air-dash gap.

### Constraints
- Must query, at the moment `CheckJustDodge` fires, **every** nearby enemy currently inside its `OnAttackTelegraphed`→`OnAttackCommitted` window (Rule 7 explicitly requires granting `State.Executable` to **all** qualifying enemies simultaneously, not just the nearest one) — but refund **exactly 1** dash charge regardless of how many enemies qualified (Rule 7/AC8).
- `IsInAttackRange` (Formulas: Just-Dodge Check) is asserted by the GDD but **no GDD defines what "attack range" concretely is** — `enemy-ai-base.md` only defines `MinEngageRange`/`MaxEngageRange` for the Ranged archetype's retreat/approach band, not a melee hitbox radius for either archetype. This is a genuine cross-doc gap this ADR must resolve, not silently paper over.
- Must not require Enemy AI to poll Dash, or Dash to poll every enemy in the level every tick — event-driven per this project's established GAS/delegate conventions.

### Requirements
- `IsJustDodge = (CurrentTime >= AttackCommittedTime - JustDodgeWindow) AND (CurrentTime <= AttackCommittedTime) AND IsInAttackRange` evaluated per-enemy at dash-activation time.
- All qualifying enemies get `State.Executable` (ref-counted overlay tag, per Health/Damage Core's tag convention).
- Exactly one charge refund per dash event regardless of multi-target count.

## Decision

1. **Ratify the already-shipped instant swept `SetActorLocation` position step** as the formal architecture for Rule 2/4 (not `LaunchCharacter`/velocity override) — see Alternative 1. No code change needed here, this closes the TR-dash-001..004 gap by documenting what's already correct.
2. **Air-dash Z-impulse (Rule 3) is a real implementation gap** — `ApplyDashImpulse` must add a small upward/anti-fall Z impulse when `MoveComp->IsFalling()` at dash activation, before the horizontal `SetActorLocation` step, sized to noticeably extend hang-time without granting a second jump (Tuning Knob, not yet in the GDD's Tuning Knobs table — needs one added: `AirDashZImpulse`).
3. **Just-Dodge detection — extends ADR-0006's interface**: `AMoonEnemyCharacterBase` (ADR-0006) must additionally expose `bool IsTelegraphingAttack() const` and `float GetAttackCommittedTime() const`, set by the same `UAnimNotify_AttackTelegraphed`/`UAnimNotify_AttackCommitted` pair that already broadcasts the delegates (Telegraphed sets both to `true`/commit-time; Committed or Stagger/Death clears `IsTelegraphingAttack` to `false`). This is additive to ADR-0006, not a redesign — the delegates ADR-0006 already defined remain for Dash's "did I successfully dodge in the moment it happened" narrative feedback (camera shake, VFX), while these new accessors serve `CheckJustDodge`'s **query-at-dash-time** need (Dash doesn't know in advance which enemy will attack, so it can't pre-subscribe to a specific one — it must ask "who's mid-telegraph right now" at the instant it dashes).
4. **`CheckJustDodge` implementation**: sphere-overlap query (`UKismet​SystemLibrary::SphereOverlapActors` or `UWorld::OverlapMultiByObjectType`) centered on the player, radius = `JustDodgeQueryRadius` (new Tuning Knob, default 500uu — generous relative to `MinEngageRange`=400uu so it never misses an enemy already in melee range), filtered to `AMoonEnemyCharacterBase`. For each result: if `IsTelegraphingAttack()==true`, evaluate the GDD's `IsJustDodge` formula using `GetAttackCommittedTime()` and the overlap-query itself as the resolved `IsInAttackRange` (see point 5). Grant `State.Executable` (ref-counted) to every match; if `MatchCount >= 1`, refund exactly 1 charge once (not per-match).
5. **Resolving `IsInAttackRange`**: rather than inventing a second, independent "attack range" number that could silently drift from whatever hitbox `OnAttackCommitted`'s actual damage application uses, this ADR defines `IsInAttackRange` as **the same overlap query's radius check against a new per-archetype `MeleeAttackRange` field on `UMoonEnceh​myArchetypeData` (ADR-0006)** — i.e., "was the player within this enemy's own declared attack range at the moment of the query," not a Dash-owned constant. This is flagged as a **simplifying stand-in** (a circle, not the eventual real hitbox shape if attacks become directional/cone-based) — acceptable for MVP archetypes (Grunt melee swing, Ranged doesn't need this check since its attacks are projectiles evaluated differently — see Edge Case note below) but explicitly not a final hitbox-fidelity decision.
6. **Charge refund**: track a local `bool bJustDodgeSucceededThisActivation` set the first time any enemy matches in step 4; refund happens once in `ActivateAbility`, not inside the per-enemy loop.

### Architecture Diagram
```
UMoonGameplayAbility_Dash::ActivateAbility()
 ├─ consume 1 charge (existing, correct)
 ├─ ApplyDashImpulse() — horizontal SetActorLocation (existing) + NEW: Z-impulse if IsFalling (Rule 3 gap)
 └─ CheckJustDodge() [TODO → this ADR's scope]
     ├─ SphereOverlapActors(player location, JustDodgeQueryRadius, AMoonEnemyCharacterBase filter)
     ├─ for each enemy: if enemy->IsTelegraphingAttack() [NEW on AMoonEnemyCharacterBase, ADR-0006 extension]
     │     evaluate IsJustDodge(CurrentTime, enemy->GetAttackCommittedTime() [NEW], JustDodgeWindow,
     │                           distance <= enemy->ArchetypeData->MeleeAttackRange [NEW field])
     │     if true: grant State.Executable (ref-counted) to enemy; mark bJustDodgeSucceededThisActivation
     └─ if bJustDodgeSucceededThisActivation: refund exactly 1 DashCharge (once)
```

### Key Interfaces
- `UMoonGameplayAbility_Dash::CheckJustDodge()` — implementation target of this ADR (currently a no-op stub).
- **New, extending ADR-0006**: `AMoonEnemyCharacterBase::IsTelegraphingAttack() const` / `GetAttackCommittedTime() const`.
- **New, extending ADR-0006's `UMoonEnemyArchetypeData`**: `MeleeAttackRange` field (Grunt only meaningfully; Ranged's projectile attacks don't need this check per Edge Case note below).
- **New Tuning Knobs** (not yet in `dash-evasion.md`'s Tuning Knobs table — flagged for a GDD addendum, not silently added to code without a paper trail): `JustDodgeQueryRadius` (default 500uu), `AirDashZImpulse` (value TBD, needs a feel pass).

## Alternatives Considered

### Alternative 1: `LaunchCharacter`/velocity-override for the dash motion instead of the already-shipped `SetActorLocation` step
- **Description**: What Rule 4's "임펄스" (impulse) wording literally suggests — override CMC velocity for the dash's duration instead of teleporting position.
- **Pros**: Slightly more "physical" (respects collision response mid-motion via CMC's own sweep-and-slide, rather than a single discrete position jump).
- **Cons**: Already tried and replaced — commit `7f7e2e3`'s own message is "make dash an instant step," explicitly moving away from a velocity/`LaunchCharacter` approach that existed before it (per the GDD's own Rule 2, updated in the same pass to say "즉시 위치 이동"). Re-introducing velocity-based motion now would regress a deliberate, already-tuned decision.
- **Rejection Reason**: Already superseded by a real, deliberate commit; GDD Rule 2 already reflects the position-step model.

### Alternative 2: Dash pre-subscribes to nearby enemies' telegraph delegates instead of querying at activation time
- **Description**: Whenever an enemy comes within some radius, the player character subscribes to that enemy's `OnAttackTelegraphed`/`OnAttackCommitted`; Just-Dodge success is judged reactively inside the delegate handler instead of via a query at dash-time.
- **Pros**: No overlap query needed at dash-time; feels more "event-driven all the way down."
- **Cons**: Requires tracking a dynamic subscribe/unsubscribe set as enemies enter/leave range (extra bookkeeping, extra failure mode if unsubscribe is missed on enemy death/despawn), and doesn't actually simplify the core check — Dash still needs "was I in range at commit time," which is the same spatial query either way. The query-at-dash-time model (point 4) is simpler because it only runs at the exact moment it's needed (on dash activation) rather than maintaining live subscriptions for every nearby enemy on every frame they're in range.
- **Rejection Reason**: More bookkeeping for no net simplification; the query-at-activation model is simpler and only costs a query on the (relatively rare) dash-activation event, not per-frame.

### Alternative 3: Invent a separate Dash-owned "attack range" constant instead of reading it from `UMoonEnemyArchetypeData`
- **Description**: A single global `JustDodgeAttackRange` Tuning Knob on the Dash side, independent of whatever the enemy's actual attack hitbox uses.
- **Pros**: Simpler — one number, no cross-ADR field needed.
- **Cons**: Guaranteed to drift from the real attack hitbox the moment Enemy AI's attack implementation is fleshed out (a Grunt's actual swing hitbox and a Dash-side guessed radius would silently diverge) — exactly the kind of silent cross-doc drift this project's re-reviews keep catching (see the 2026-07-23 GDD re-review session's registry findings).
- **Rejection Reason**: A single-owner value (on the enemy's own archetype data) is more likely to stay true to whatever the real hitbox becomes than a duplicated constant on the consumer side.

## Consequences

### Positive
- Closes the single biggest functional gap in the entire signature combat loop — Just-Dodge → Executable → F-key extraction is the game's stated core catharsis loop, and it currently does not exist in code at all.
- Reuses the already-correct, already-tuned instant-step dash motion without touching working code.
- `MeleeAttackRange` living on the enemy's own data (not duplicated on Dash) means future hitbox refinement only has one place to update.

### Negative
- Requires touching ADR-0006 (adding 2 accessor methods + 1 data field) even though ADR-0006 was just written — flagged explicitly rather than silently patched, since ADR-0006 hasn't been Accepted yet, this is a low-cost amendment, not a break of an already-shipped contract.
- The circle-based `MeleeAttackRange` is a known simplification that may need revisiting once real attack hitboxes (swing arcs, projectile paths) are implemented.

### Risks
- **`IsInAttackRange` for the Ranged archetype is not well-defined by this ADR** — Ranged's "attack" is a projectile spawn (Rule 5 of enemy-ai-base.md: "발사체 스폰 = `OnAttackCommitted`"), not a melee-range hitbox, so a circle-radius check against the Ranged unit's own position doesn't necessarily mean "the projectile would have hit the player." Mitigation: for MVP, treat Ranged Just-Dodge as always `IsInAttackRange=true` if the enemy is telegraphing at all (a Ranged unit's attack is aimed at the player by construction, so if it's mid-telegraph and the player dashes, the player was very likely in the line of fire) — this is a deliberate MVP simplification, not a silent gap; flagged as an Open Question for revisit once projectile implementation exists.
- **`JustDodgeQueryRadius`=500uu is a placeholder**, not validated against real attack ranges since Enemy AI's actual melee/projectile hitbox sizes aren't implemented yet. Mitigation: tune once Enemy AI's attacks are real; this ADR's architecture (a radius-filtered overlap query) doesn't change even if the radius value does.
- **Air-dash Z-impulse value is undetermined** (Rule 3 gap) — needs a feel pass, not a formula-derivable number like the other Tuning Knobs. Mitigation: implement as a Tuning Knob from day one so it's iterable without recompiling, per this project's now-consistent data-driven-tuning pattern (ADR-0005, ADR-0006).

## GDD Requirements Addressed

| GDD System | Requirement | How This ADR Addresses It |
|------------|-------------|---------------------------|
| dash-evasion.md | Rule 1 (charge system) + Formula (Dash Cooldown & Charges) | Already correctly implemented — ratified, no change |
| dash-evasion.md | Rule 2/4 (instant position step, override semantics) | Ratifies the already-shipped `SetActorLocation` approach as canonical (Alternative 1) |
| dash-evasion.md | Rule 3 (air-dash Z impulse) | Identified as an unimplemented gap; new `AirDashZImpulse` Tuning Knob proposed |
| dash-evasion.md | Rule 5 (i-frames) | Already correctly implemented via `ActivationOwnedTags` — ratified, no change |
| dash-evasion.md | Rule 6/7 (Just-Dodge detection + reward) + Formula (Just-Dodge Check) | This ADR's primary scope — overlap-query architecture, `IsInAttackRange` resolution, single-refund logic |
| dash-evasion.md | Rule 8 (no cast/anim interrupt) | Already satisfied by the existing GAS `InstancedPerActor` + same-frame activation pattern shared with Spell Casting |
| dash-evasion.md | Rule 9 (`MovementLocked` gate) | Already correctly implemented — ratified, no change |
| dash-evasion.md | Edge Case (multi-enemy simultaneous Just-Dodge) | Overlap query naturally handles all qualifying enemies; single-refund flag handles the charge-count rule |

## Performance Implications
- **CPU**: One sphere-overlap query per dash activation (not per-frame) — negligible at MVP enemy counts.
- **Memory**: Two new small accessor fields on the enemy character, one new field on the archetype DataAsset — negligible.
- **Load Time**: N/A.
- **Network**: N/A — no multiplayer requirement in this project's GDDs.

## Migration Plan
1. Add `IsTelegraphingAttack()`/`GetAttackCommittedTime()` to `AMoonEnemyCharacterBase` (ADR-0006 scope, implement alongside that ADR's `AnimNotify` pair).
2. Add `MeleeAttackRange` to `UMoonEnemyArchetypeData` (ADR-0006 scope).
3. Implement `UMoonGameplayAbility_Dash::CheckJustDodge()` per the Decision section's overlap-query algorithm.
4. Add `AirDashZImpulse` Tuning Knob + apply it in `ApplyDashImpulse` when `MoveComp->IsFalling()`.
5. Add `JustDodgeQueryRadius` and `AirDashZImpulse` to `dash-evasion.md`'s Tuning Knobs table (GDD addendum — this ADR introduces two values the GDD doesn't currently have).

## Validation Criteria
- `dash-evasion.md` AC3/AC4/AC5/AC8 (the Just-Dodge-specific criteria) — currently **cannot pass** since `CheckJustDodge` is a no-op; this ADR's implementation is what makes them testable at all.
- New check: multi-enemy Just-Dodge (2+ telegraphing enemies in range simultaneously) grants `State.Executable` to all of them but refunds exactly 1 charge (AC8, currently untestable).
- Ranged Just-Dodge simplification (always-true `IsInAttackRange` while telegraphing) — playtest whether this reads as fair or exploitable (e.g., dashing near a telegraphing Ranged unit from far outside its actual firing line still counts) — flagged as an Open Question, not silently assumed correct.

## Related Decisions
- ADR-0006 (enemy-ai-behavior-tree) — this ADR extends its Key Interfaces (2 new accessors + 1 new data field) rather than conflicting with it.
- ADR-0005 (camera-system-springarm) — the data-driven Tuning Knob pattern (`AirDashZImpulse`/`JustDodgeQueryRadius` as designer-tunable values) follows the same convention established there.
- `design/gdd/dash-evasion.md`, `design/gdd/enemy-ai-base.md` — GDDs this ADR implements or amends.
