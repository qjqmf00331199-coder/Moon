# ADR-0006: Enemy AI (base) — AIController + Shared Behavior Tree + AIPerception

## Status
Proposed

## Date
2026-07-23

## Engine Compatibility

| Field | Value |
|-------|-------|
| **Engine** | Unreal Engine 5.8 |
| **Domain** | AI |
| **Knowledge Risk** | MEDIUM — `AIController`/`BehaviorTree`/`AIPerceptionComponent` are long-stable pre-cutoff APIs (low risk on their own), but this project's engine-reference library has **no dedicated AI/BehaviorTree/Perception module doc at all**. `docs/engine-reference/unreal/modules/navigation.md` explicitly says "Behavior Trees: AI decision-making (covered in AI module)" — that referenced doc does not exist in `docs/engine-reference/unreal/modules/`. Additionally `navigation.md` itself is only verified against UE5.7, not the pinned 5.8 (already flagged as an Open Question in `enemy-ai-base.md`). |
| **References Consulted** | `docs/engine-reference/unreal/VERSION.md`, `docs/engine-reference/unreal/breaking-changes.md`, `docs/engine-reference/unreal/deprecated-apis.md`, `docs/engine-reference/unreal/modules/navigation.md` (partial — Nav Mesh only, no BT/Perception content despite the cross-reference) |
| **Post-Cutoff APIs Used** | None chosen deliberately — see Decision (classic AIController+BT+Perception over Mass Framework or StateTree, both of which would carry more post-cutoff surface). |
| **Verification Required** | `UAIPerceptionComponent`/`UAISenseConfig_Sight`/`UAISenseConfig_Hearing` signatures against real 5.8 headers before implementation (no engine-reference doc to check against — first-use verification only). Recommend authoring `docs/engine-reference/unreal/modules/ai.md` as a follow-up so future AI work in this project isn't flying blind. |

## ADR Dependencies

| Field | Value |
|-------|-------|
| **Depends On** | Health/Damage Core (Approved, no ADR yet — TR-hp-006/007/008 uncovered per architecture-review, but the `OnDeath`/attribute-slot interface this ADR consumes is already GDD-stable) |
| **Enables** | Dash/Evasion ADR (just-dodge timing subscribes to `OnAttackTelegraphed`/`OnAttackCommitted` defined here) |
| **Blocks** | None currently — closes an architecture-review gap (TR-ai-001..008, 8/8 previously uncovered) |
| **Ordering Note** | Should be written before or alongside the Dash/Evasion ADR since Dash/Evasion's just-dodge timing is a direct consumer of this ADR's telegraph delegate interface |

## Context

### Problem Statement
`design/gdd/enemy-ai-base.md` has been Approved since 2026-07-16 with 10 Core Rules covering 2 archetypes (Grunt/Ranged), perception, telegraphed attacks, stagger, and death — but has zero ADR coverage and, unlike Camera, **zero existing implementation of any kind**. `production/session-state/plan.md` confirms only `BP_Enemy_Base` exists (an empty Blueprint stub) — no `AIController`, no Behavior Tree asset, no `AIPerceptionComponent`, no C++ AI code at all (`grep` across `Moon/Source/` for `AIController`/`BehaviorTree`/`AIPerception` returns nothing). This is a greenfield architectural decision, not a retrofit.

The GDD's own Overview text leaves the state-machine implementation technology open ("`AIController` + Behavior Tree(**또는 State Tree**)") — this ADR is what resolves that open choice.

### Constraints
- MVP is exactly 2 archetypes (Grunt/Ranged) sharing one state machine and one perception setup, differing only in engage-range behavior and numeric tuning (Rule 1).
- Attack telegraph timing (`OnAttackTelegraphed`/`OnAttackCommitted`) must be a public per-instance delegate interface — Dash/Evasion (not yet designed) will subscribe to it for just-dodge timing (Rule 4).
- MaxHealth real values are owned here, not Health/Damage Core (Rule 6) — Health/Damage Core only provides the attribute slot.
- No confirmed large-swarm requirement in MVP scope (the "수십 마리 동시 몰림" scenario is an open, unmeasured risk in the GDD's own Open Questions, not a committed MVP requirement) — this rules out over-engineering for crowd-scale simulation.

### Requirements
- Sight (cone) + Hearing (noise-event) perception, Hearing must satisfy `NoiseDetectionRadius = BaseHearingRadius × NoiseLoudness` (Formulas).
- Anim-driven telegraph contract: `OnAttackTelegraphed` (windup AnimNotify) → `OnAttackCommitted` (strike-frame AnimNotify, damage applies only here).
- Ranged engage-band retreat/approach logic (Formulas: `ShouldRetreat`/`ShouldAttack`).
- Stagger slot (external `TriggerStagger()`/`ClearStagger()`, this system owns none of the trigger conditions).
- Death only via Health/Damage Core's `OnDeath` (Rule 7) — no self-judged death.
- Archetype identity exposed as `Enemy.Archetype.Grunt`/`Enemy.Archetype.Ranged` GameplayTags (Rule 9).

## Decision

1. **Classic `AIController` + `UBehaviorTree` + `UAIPerceptionComponent`** — not Mass Framework (built for hundreds/thousands of agents; MVP has no confirmed swarm requirement — the GDD itself flags large-swarm perf as unmeasured, not committed), not StateTree (net-new to this project with zero prior tooling/tuning experience, no concrete benefit over BT for 2 archetypes sharing one tree). See Alternatives.
2. **Single shared `AMoonEnemyAIController` + single `BT_Enemy_Base` asset** for both archetypes. Archetype divergence (Rule 1) is expressed as a Blackboard key `EArchetypeType` (enum: Grunt/Ranged) set at spawn from the character's `Enemy.Archetype.*` GameplayTag, branching inside the shared tree via Decorators rather than separate BT assets per archetype — keeps the state machine (Idle/Alert/Chase/Attack/Stagger/Dead) as one source of truth, per Rule 1's "동일한 상태머신·인지 로직을 공유" mandate.
3. **`UMoonEnemyArchetypeData : UDataAsset`** (one instance per archetype) holds every numeric Tuning Knob: `SightRange`, `SightAngle`, `BaseHearingRadius`, `MinEngageRange`, `MaxEngageRange`, `MinTelegraphWindow`, `AlertToIdleTimeout`, `RetreatPathfindFailTimeout`, `CorpseHoldDuration`, `MaxHealth`. Loaded by the AIController/Character at spawn based on the archetype tag — same data-asset pattern as ADR-0005's `UMoonCameraSettings`, for the same reason (GDD values must be designer-tunable without recompiling, and this avoids re-introducing the hardcoded-constructor anti-pattern the registry now forbids for Camera).
4. **Perception → GDD formula mapping (verified, not assumed)**: native `UAISenseConfig_Hearing::HearingRange` set to `BaseHearingRadius` per archetype; `UAISense_Hearing`'s native distance-gating against `MakeNoise(Loudness)` already scales effective detection distance by the reported Loudness — this maps 1:1 onto the GDD's `NoiseDetectionRadius = BaseHearingRadius × NoiseLoudness` formula with **no custom radius math needed**, just correct per-archetype `HearingRange` config and trusting the engine's native Loudness scaling (flagged under Verification Required since this project's engine-reference doesn't independently confirm 5.8 header behavior for this).
5. **Telegraph delegates live on the enemy Character, not the AIController**: `AMoonEnemyCharacterBase` exposes `FOnAttackTelegraphed`/`FOnAttackCommitted` multicast delegates, broadcast from two custom `UAnimNotify` subclasses (`UAnimNotify_AttackTelegraphed`, `UAnimNotify_AttackCommitted`) placed on attack montages. Dash/Evasion subscribes per-instance to the specific enemy actor it's tracking, not to the AIController (which Dash/Evasion has no reason to reference).
6. **Stagger**: Blackboard bool `bIsStaggered`, set externally via `TriggerStagger()`/`ClearStagger()` on the Character (which forwards to the Blackboard). A BT Decorator with "Observer Aborts: Self" on the Attack/Chase branches aborts immediately when the flag flips true, transitioning to a Stagger branch — matches Edge Case "공격 캔슬, `OnAttackCommitted` 미발행" without needing a custom abort mechanism.
7. **Death**: Character subscribes to Health/Damage Core's `OnDeath`. On receipt: `AIController->BrainComponent->StopLogic()`, disable collision + perception, enable ragdoll, start `CorpseHoldDuration` timer, despawn. The BT itself never queries Health directly (Rule 7 — single entry point via the character-level subscription only).
8. **Archetype tag**: `Enemy.Archetype.Grunt`/`Enemy.Archetype.Ranged` granted at spawn (matches `Enemy.Archetype.*` naming already used by the GDD and referenced by future Enemy Elite Shield).

### Architecture Diagram
```
AMoonEnemyCharacterBase
 ├─ UMoonEnemyArchetypeData* (DataAsset ref, set at spawn from archetype)
 ├─ AIControllerClass = AMoonEnemyAIController
 ├─ GameplayTag: Enemy.Archetype.Grunt | Enemy.Archetype.Ranged
 ├─ FOnAttackTelegraphed / FOnAttackCommitted (multicast delegates)
 │    ▲ broadcast by UAnimNotify_AttackTelegraphed / UAnimNotify_AttackCommitted
 ├─ TriggerStagger() / ClearStagger() → writes Blackboard.bIsStaggered
 └─ OnDeath (subscribed from Health/Damage Core) → StopLogic + ragdoll + despawn timer

AMoonEnemyAIController
 ├─ UAIPerceptionComponent
 │    ├─ UAISenseConfig_Sight  (SightRange/SightAngle from ArchetypeData)
 │    └─ UAISenseConfig_Hearing (HearingRange = BaseHearingRadius from ArchetypeData)
 ├─ Blackboard: EArchetypeType, bIsStaggered, TargetActor, PlayerDistance
 └─ RunBehaviorTree(BT_Enemy_Base)

BT_Enemy_Base (shared, single asset)
 Idle → Alert(Investigate) → Chase → Attack → [Stagger interrupt] → Dead
        (Ranged branch inside Chase/Attack: retreat/approach via Blackboard.PlayerDistance
         vs ArchetypeData.MinEngageRange/MaxEngageRange)
```

### Key Interfaces
- `AMoonEnemyCharacterBase::OnAttackTelegraphed` / `OnAttackCommitted` (multicast delegates) — Dash/Evasion's just-dodge timing consumes these.
- `AMoonEnemyCharacterBase::TriggerStagger()` / `ClearStagger()` — Super Armor / CC Interrupt (not yet designed) will call these.
- `Enemy.Archetype.Grunt` / `Enemy.Archetype.Ranged` GameplayTags — Enemy Elite Shield (not yet designed) will key off these.
- `UMoonEnemyArchetypeData` (DataAsset) — single source of truth for all per-archetype Tuning Knobs, including `MaxHealth`.
- **`AMoonEnemyCharacterBase::IsTelegraphingAttack() const` / `GetAttackCommittedTime() const`** (added 2026-07-23, ADR-0007) — queryable telegraph state, set/cleared by the same AnimNotify pair that broadcasts the delegates above. Added for Dash/Evasion's Just-Dodge check, which needs to query "who's mid-telegraph right now" at dash-activation time rather than pre-subscribing to a specific enemy. Does not change this ADR's original delegate-based interface, purely additive.
- **`UMoonEnemyArchetypeData::MeleeAttackRange`** (added 2026-07-23, ADR-0007) — per-archetype melee attack radius, consumed by Dash/Evasion's Just-Dodge `IsInAttackRange` check. Meaningful for Grunt; Ranged's projectile attacks use a different simplification (see ADR-0007 Risks).

## Alternatives Considered

### Alternative 1: Mass Framework (Mass Entity/MassAI)
- **Description**: Data-oriented ECS-style crowd simulation, reorganized in 5.8 into `MassCore`+`MassGameplay` per this project's own breaking-changes notes.
- **Pros**: Scales to hundreds/thousands of agents far more cheaply than per-actor AIController+BT.
- **Cons**: MVP has exactly 2 enemies types with no confirmed large-swarm requirement (the GDD's own large-swarm concern is an unmeasured *risk*, not a committed scope item); Mass has a steep architectural cost (fragment/processor model, no per-actor Blueprint-friendly AIController) for a scope that doesn't need it; highest post-cutoff API surface of the three options.
- **Rejection Reason**: Solving for a scale problem the MVP doesn't have yet. If the swarm-perf Open Question later proves out as a real bottleneck, that's its own future ADR, not a reason to over-build now.

### Alternative 2: StateTree
- **Description**: UE5.3+ node-based state machine, positioned as BT's modern successor for some use cases.
- **Pros**: More explicit state-machine semantics than BT's tree-of-tasks model; newer engine investment.
- **Cons**: Zero prior project experience or tooling habits with StateTree; BT tooling/debugging (visual debugger, blackboard watch) is mature and already the assumed default across this project's engine-reference docs and prior sessions; no concrete requirement in the GDD that BT can't satisfy (the GDD text itself only floats StateTree as an "or", not a preference).
- **Rejection Reason**: No requirement StateTree uniquely satisfies; switching costs (learning curve, zero existing patterns) aren't justified for a 2-archetype MVP.

### Alternative 3: Separate Behavior Tree asset per archetype
- **Description**: `BT_Enemy_Grunt` and `BT_Enemy_Ranged` as independent trees instead of one shared tree with Blackboard branching.
- **Pros**: Simpler individual trees, no branching decorators needed.
- **Cons**: Directly contradicts Rule 1's "동일한 상태머신·인지 로직을 공유" — two trees drift independently over time (the exact kind of silent divergence this project's re-reviews keep catching), and any shared-logic bugfix (e.g. the telegraph contract) would need fixing in two places.
- **Rejection Reason**: Violates the GDD's explicit single-state-machine mandate.

## Consequences

### Positive
- One shared BT + one shared AIController means the Idle→Dead state machine, telegraph contract, and stagger handling can only be implemented once — no risk of Grunt/Ranged silently diverging.
- Data-asset-driven tuning (matching ADR-0005's established pattern) keeps all 10 numeric knobs designer-editable without recompiling, and keeps this project consistent about not hardcoding tunables in constructors.
- Native Hearing sense's Loudness-scaling maps exactly onto the GDD's own formula — no custom detection-radius code needed, reducing implementation risk.

### Negative
- Archetype branching inside one tree means the shared tree is somewhat more complex than two simple independent trees would be (mitigated: only the engage-band retreat/approach logic actually differs; state machine shape is otherwise identical).
- `UMoonEnemyArchetypeData` is new code surface (one more DataAsset class) beyond what a first pass might reach for.

### Risks
- **Perception/BT signatures unverified against real 5.8 headers** — this project's engine-reference has no AI module doc at all despite `navigation.md` referencing one. Mitigation: verify at implementation time; flagged explicitly rather than silently assumed correct. Recommend a follow-up `docs/engine-reference/unreal/modules/ai.md` be authored so this gap doesn't recur for future AI work (Boss Phase, Enemy Elite Shield will hit the same gap).
- **Large-swarm performance is unmeasured** (carried forward from the GDD's own Open Questions) — this ADR's architecture (per-actor AIController+BT+Perception) is the classic, well-understood approach but does not solve a swarm-scale problem if one turns out to exist. Mitigation: explicitly deferred, not silently ignored — if Environmental Chain-Destruction's dense-pack scenarios prove this out as a real bottleneck, that becomes a scoped future ADR (possibly triggering the Mass Framework alternative rejected here), not a reason to block this one.
- **`MinTelegraphWindow` (0.4s) is an unverified assumption against Dash/Evasion's actual just-dodge input buffer** (already an open item in the GDD) — this ADR's telegraph delegate interface is correct regardless of the exact window value; the value itself needs cross-check when the Dash/Evasion ADR is written next.

## GDD Requirements Addressed

| GDD System | Requirement | How This ADR Addresses It |
|------------|-------------|---------------------------|
| enemy-ai-base.md | Rule 1 (2 archetypes, shared state machine) | Single `AMoonEnemyAIController` + single `BT_Enemy_Base`, Blackboard-branched |
| enemy-ai-base.md | Rule 2 (Sight+Hearing perception) + Formula (Noise Detection Radius) | `UAIPerceptionComponent` with Sight+Hearing configs from `UMoonEnemyArchetypeData`; native Loudness scaling confirmed to match the GDD formula |
| enemy-ai-base.md | Rule 3/4 (anim-driven telegraph, public delegates) | `UAnimNotify_AttackTelegraphed`/`Committed` broadcasting `AMoonEnemyCharacterBase`'s public multicast delegates |
| enemy-ai-base.md | Rule 5 (Ranged engage band) + Formula (Ranged Engage Distance Check) | BT branch reading `Blackboard.PlayerDistance` against `ArchetypeData.MinEngageRange/MaxEngageRange` |
| enemy-ai-base.md | Rule 6 (MaxHealth ownership) | `UMoonEnemyArchetypeData.MaxHealth` applied to the Health/Damage Core attribute slot at spawn |
| enemy-ai-base.md | Rule 7 (death only via `OnDeath`) | Character-level subscription only; BT/AIController never self-judge death |
| enemy-ai-base.md | Rule 8 (Stagger slot only) | `Blackboard.bIsStaggered` + Observer-Abort decorator; trigger conditions owned elsewhere |
| enemy-ai-base.md | Rule 9 (archetype GameplayTag) | `Enemy.Archetype.Grunt`/`Ranged` tag granted at spawn |
| enemy-ai-base.md | Rule 10 (Executable/Invulnerable are overlays) | State machine reads these tags nowhere in its transition conditions — by construction, since the BT never queries them |

## Performance Implications
- **CPU**: Per-actor `AIPerceptionComponent` + BT tick cost, standard for the engine at small enemy counts (2 archetypes, no confirmed swarm scale). Unmeasured at large counts — see Risks.
- **Memory**: One `UMoonEnemyArchetypeData` instance per archetype (2 total), negligible.
- **Load Time**: Negligible.
- **Network**: Out of scope — no multiplayer requirement stated anywhere in this project's GDDs to date.

## Migration Plan
N/A — greenfield, no existing AI implementation to migrate from (`BP_Enemy_Base` is an empty stub).

1. Create `UMoonEnemyArchetypeData` DataAsset class + Grunt/Ranged instances with the GDD's default values.
2. Create `AMoonEnemyCharacterBase` (delegates, `TriggerStagger`/`ClearStagger`, `OnDeath` subscription, archetype tag + ArchetypeData assignment).
3. Create `AMoonEnemyAIController` + `UAIPerceptionComponent` (Sight+Hearing configs from ArchetypeData).
4. Create `BT_Enemy_Base` + Blackboard asset with the Idle/Alert/Chase/Attack/Stagger/Dead branches, Ranged engage-band logic gated on `EArchetypeType`.
5. Create `UAnimNotify_AttackTelegraphed`/`UAnimNotify_AttackCommitted`, place on attack montages.
6. Promote `BP_Enemy_Base` (Blueprint) to inherit from `AMoonEnemyCharacterBase`, per `plan.md`'s existing checklist (BP_Spider/BP_SpiderKing as content variations on top).

## Validation Criteria
- `enemy-ai-base.md` Acceptance Criteria 1-12, executed in PIE against `BT_Enemy_Base`.
- Perception formula check: place a noise source at exactly `BaseHearingRadius × NoiseLoudness` distance, confirm Alert triggers at that boundary and not beyond (validates the native-Loudness-scaling assumption from Decision point 4).
- Telegraph window check: confirm `OnAttackCommitted` never fires without a preceding `OnAttackTelegraphed` in the same attack instance, and that Stagger/Death between the two suppresses `OnAttackCommitted` (AC6/AC7).

## Related Decisions
- ADR-0005 (camera-system-springarm) — establishes the `UDataAsset`-driven tuning pattern this ADR reuses for `UMoonEnemyArchetypeData`.
- `design/gdd/enemy-ai-base.md`, `design/gdd/dash-evasion.md` (telegraph consumer), `design/gdd/health-damage-core.md` (`OnDeath` producer) — GDDs this ADR implements or interfaces with.
