# Architecture Review — 2026-07-27

- **Date:** 2026-07-27
- **Engine:** Unreal Engine 5.8 (pinned 2026-07-16, post-LLM-cutoff — HIGH risk)
- **Mode:** `full` (independent reviewer session — did not author ADR-0004..0007)
- **GDDs Reviewed:** 9 MVP + `game-concept.md` + `systems-index.md`
- **ADRs Reviewed:** 7 (0001–0007)
- **Previous review:** [architecture-review-2026-07-18.md](architecture-review-2026-07-18.md) — verdict FAIL, 9/6/59

> **Engine Specialist Consultation: SKIPPED.** This session operates under a standing
> instruction not to spawn subagents unless explicitly requested. Phase 5 findings below are
> audit-only and have **not** been second-opinioned by `unreal-specialist`. Two engine findings
> (E-2, E-3) would normally warrant that review — treat them as unconfirmed by a domain expert.

---

## Verdict: 🔴 FAIL

Down from 59 gaps to 23, but the failure cause is unchanged in kind: **Foundation-layer
requirements remain uncovered, and three ADRs depend on a contract no ADR defines.**

| Criterion | Result |
|---|---|
| All requirements covered | ❌ 23 gaps / 76 |
| No blocking cross-ADR conflicts | ❌ 1 blocking (C-1, recurring since 2026-07-18) |
| Foundation layer complete | ❌ Player Movement 7/10 uncovered, Health/Damage 3/9 uncovered |
| Engine consistency | ⚠️ 1 deprecated-API finding against an ADR's own "none" claim |

`/create-epics` stays gated. `/gate-check pre-production` stays gated.

---

## Traceability Summary

| | 2026-07-18 | 2026-07-27 | Δ |
|---|---|---|---|
| Total active requirements | 74 | **76** (+4 new, −2 superseded) | +2 |
| ✅ Covered | 9 (12%) | **38 (50%)** | +29 |
| ⚠️ Partial | 6 (8%) | **15 (20%)** | +9 |
| ❌ Gap | 59 (80%) | **23 (30%)** | **−36** |

### Where the −36 came from

| Source | Gaps closed or upgraded |
|---|---|
| ADR-0005 (Camera) | 9 (7 ✅ / 2 ⚠️) — system went 0/9 → 9/9 addressed |
| ADR-0006 (Enemy AI) | 8 (6 ✅ / 2 ⚠️) — system went 0/8 → 8/8 addressed |
| ADR-0007 (Dash/Evasion) | 7 (5 ✅ / 2 ⚠️), 1 still gap |
| ADR-0004 (Luna Overdrive) | 10 — all 7 overdrive TRs (+2 newly registered) + `TR-tension-008` + `TR-spell-009` upgrade + `TR-spell-006` upgrade + `TR-spell-010` |
| **Prior-review correction** | **2** — `TR-spell-008` and `TR-tension-001` were scored ❌ on 07-18 but were already addressed by ADR-0003 §6 and ADR-0001 §3 respectively. Not new work. |

**Honest delta: 34 closed by the new ADRs, 2 were scoring errors in the previous review.**

### Coverage by system

| System | ✅ | ⚠️ | ❌ | Governing ADR |
|---|---|---|---|---|
| Camera System (base) | 7 | 2 | 0 | ADR-0005 |
| Luna Overdrive | 7 | 1 | 0 | ADR-0004 |
| Enemy AI (base) | 6 | 2 | 0 | ADR-0006 |
| Spell Casting (base) | 7 | 3 | 0 | ADR-0003, ADR-0004 |
| Dash/Evasion | 5 | 2 | 1 | ADR-0007 |
| Health/Damage Core | 4 | 2 | 3 | ADR-0001, ADR-0002 |
| Player Movement | 1 | 2 | 7 | ADR-0001, ADR-0005 |
| Combo/Tension Gauge | 1 | 1 | 5 | ADR-0001 (attribute only), ADR-0004 (lock only) |
| Combat HUD | 0 | 0 | 7 | **none** |

---

## Coverage Gaps (23)

### 🔴 Foundation layer — cause of the FAIL verdict

| TR-ID | Requirement | Engine Risk | Note |
|---|---|---|---|
| **TR-hp-006** | Immediate presentation-decoupled death detection (`Health<=0` → Death same frame, `OnDeath` exactly once) | MEDIUM | **Blocks ADR-0002, ADR-0004, ADR-0006** — see C-1 |
| **TR-hp-007** | Event exposure: `OnDeath`, `OnExecuted`, `OnHealthPercentCrossed`, Health change delegates | MEDIUM | **Blocks ADR-0002, ADR-0004, ADR-0006, and TR-spell-006** — see C-1 |
| TR-hp-008 | Runtime `MaxHealth` re-clamp (absolute preservation, downward clamp must not itself kill) | LOW | |
| TR-mov-002 | Movement module compile-independent of SpellCasting (Build.cs exclusion) | LOW | |
| TR-mov-003 | Airborne sub-states from `Velocity.Z` sign after CMC tick, external Z-impulse re-entry | MEDIUM | Interacts with ADR-0007's new `AirDashZImpulse` |
| TR-mov-006 | `MovementLocked` write access restricted to Status Effect system only | LOW | ADR-0007 reads it; nothing owns the write restriction |
| TR-mov-007 | Jump input buffer 150ms / coyote time 150ms, delta-time based | LOW | |
| **TR-mov-008** | Hitstop/execution = **no Time Dilation of any form** | LOW | **Shipped code violates this — see V-1** |
| TR-mov-009 | Input→velocity latency budget p95 ≤33ms, time-to-95%-speed ≤50ms, Insights trace scopes | MEDIUM | |
| TR-mov-010 | All locomotion anims non-root-motion (`bEnableRootMotion==false`) | LOW | |

→ Suggested: `/architecture-decision Health-Damage Core death and event contract` (highest priority),
then `/architecture-decision Player Movement runtime contract` (the 7 mov gaps ADR-0001 never covered
despite its title).

### 🟠 Feature layer

| TR-ID | Requirement | Note |
|---|---|---|
| TR-tension-002 | Read-only subscription to `OnSpellHit` / `OnTagAdded(State.Executable)` / `OnDamageApplied` / Death | Depends on TR-hp-007 |
| TR-tension-003 | Decay timer with grace gating, keeps ticking during `State.Invulnerable` | |
| TR-tension-004 | Deterministic same-frame Gain → Penalty → Decay ordering, clamp [0,Max] | |
| TR-tension-005 | `OnOverdriveTriggered` exactly once at Max, reset to 0 same frame | **ADR-0004 consumes this event but no ADR defines its emission** |
| TR-tension-007 | Expose read-only value + Building/Decaying state downstream | |

→ Suggested: `/architecture-decision Combo/Tension Gauge`. This is the **producer** of the event
ADR-0004 is built on — leaving it uncovered means the Overdrive ADR's entry condition is unspecified.

### 🟡 Core / Presentation layer

| TR-ID | Requirement | Note |
|---|---|---|
| TR-dash-008 | Expose dash charge stack + cooldown gauge to Combat HUD | ADR-0007 does not address the HUD surface |
| TR-hud-001..007 | Entire Combat HUD system (7 TRs) | **Zero ADR coverage.** `WBP_CombatHUD` is already substantially built in-engine with no governing architectural decision — the implementation is ahead of the architecture here. |

→ Suggested: `/architecture-decision Combat HUD`. Note this ADR would be partly **reverse-documenting**
existing work (`WBP_CombatHUD` widget tree, event bindings, the known `write_graph_dsl` round-trip
limitation) rather than greenfield design.

### Note on one ⚠️ that is close to a gap

**TR-cam-003** (camera-relative movement basis; Movement and Camera both read `PlayerController`
rotation **independently — no execution-order dependency, no circular reference**) is scored
⚠️ Partial, not ✅. ADR-0005's requirements table addresses it with "Already implemented
(Enhanced Input + camera-relative vector) — unaffected by this ADR." That settles that the ADR
changes nothing; it does not settle the contract the TR actually asks about. `systems-index.md:25`
still records Camera ↔ Player Movement as an explicit **mutual** dependency, which is precisely the
ambiguity TR-cam-003 exists to close. The ADR's architecture diagram implies the independent-read
model but never states it as a decision. Worth one paragraph in ADR-0005 before it is Accepted.

---

## Cross-ADR Conflicts

> **Known conflict-prone areas** (from `docs/consistency-failures.md`): ADR numbering collisions
> (logged 2026-07-18, since resolved on disk — log entry not updated), and **downstream ADRs
> forward-referencing interfaces the cited upstream ADR never actually defines** (logged 2026-07-18,
> **still open and now three times wider**).

### 🔴 C-1 — `OnDeath` consumed by three ADRs, defined by none (BLOCKING, recurring)

**Type:** Integration contract / State ownership
**ADRs involved:** ADR-0002 (Accepted) · ADR-0004 (Proposed) · ADR-0006 (Proposed) · ADR-0001 (Accepted, the cited source)

- **ADR-0002 §3** calls `AMoonCharacterBase::OnDeath()` and attributes it to ADR-0001
  ("already gated through the `UGameplayEffectExecutionCalculation` damage pipeline per ADR-0001").
- **ADR-0001's Decision text defines no death event, no `OnDeath` delegate, and no `Health<=0`
  transition.** It defines the ExecCalc for damage/defense-bypass/i-frames only. Its own
  "GDD Requirements Addressed" table does not list TR-hp-006 or TR-hp-007.
- **ADR-0004 §Risks** concedes it: *"Death handling is not fully implemented in the current
  Health/Damage slice. Mitigation: expose `ForceEndOverdrive(PlayerDeath)` and connect it when the
  canonical death delegate exists."* — i.e. it ships an unconnected interface.
- **ADR-0006 §7** makes HDC's `OnDeath` the **sole** Dead-state transition path for every enemy
  ("the BT itself never queries Health directly"). The entire enemy death path hangs on it.
- `health-damage-core.md` Rules 7/9 and ACs 4/7 define `OnDeath`/`OnExecuted` unambiguously at the
  GDD level. **The GDD is fine. The architecture never picked it up.**

**Impact:** ADR-0002's restore path, ADR-0004's `PlayerDeath` end-reason, and ADR-0006's entire
Dead-state branch are all unimplementable as written. This was flagged on 2026-07-18 with a
concrete resolution and was not acted on; two new ADRs have since been written on top of the same
undefined contract.

**Resolution options:**
1. **(Recommended)** New ADR — *Health/Damage Core death detection and event contract* — owning
   TR-hp-006/007/008 and `OnExecuted`. Then ADR-0002/0004/0006 cite it instead of ADR-0001.
2. Amend ADR-0001 with a Death Detection subsection (ExecCalc detects `Health<=0` → fires multicast
   `OnDeath` exactly once) and re-issue it as a revision. Cheaper, but overloads an already
   mis-titled ADR ("Player Movement and GAS Core" now also owning death).
3. Downgrade ADR-0002/0004/0006's claims to explicit open dependencies. Documents the problem
   without fixing it — not sufficient to clear a FAIL.

### 🟠 C-2 — Dash motion model: GDD Rule 4 vs ADR-0007 Decision 1

**Type:** Architecture pattern
**Involved:** `dash-evasion.md` Rule 2 vs Rule 4 (internal) · `TR-dash-002` · ADR-0007

- `dash-evasion.md` **Rule 2**: "즉시 위치 이동… 이동 경로는 충돌 검사를 수행" (instant swept position move).
- `dash-evasion.md` **Rule 4**: "임펄스 합성 방식: Override(덮어쓰기) — 대쉬 발동 시 기존 velocity
  (수평+수직 momentum 전부)를 대쉬 벡터로 완전히 덮어쓴다" (velocity override).
- `TR-dash-002` captured Rule 4's wording verbatim: *"Apply dash as full velocity Override … via
  Player Movement impulse/Velocity API"*.
- **ADR-0007 Decision 1 + Alternative 1 explicitly reject the velocity/`LaunchCharacter` model** and
  ratify swept `SetActorLocation` (commit `7f7e2e3`, "make dash an instant step").

**Impact:** Rule 2 and Rule 4 of an Approved GDD describe two different mechanisms. An implementer
reading Rule 4 (or a story quoting `TR-dash-002`) would build the design ADR-0007 rejected. The
same divergence would break ADR-0007's air-dash `AirDashZImpulse` design, which assumes the
position-step model.

**Resolution:** Revise `dash-evasion.md` Rule 4 to state override *semantics* (each dash lands the
same distance regardless of prior momentum) delivered via the Rule 2 position step, and revise
`TR-dash-002`'s registry text to match. Not an ADR change.

### 🟡 C-3 — `CameraLagMaxDistance`: 60 uu vs 200 uu inside one GDD

**Type:** Data ownership / value conflict
**Involved:** `camera-system-base.md` (internal) · `TR-cam-004` · ADR-0005

| Location | Value |
|---|---|
| `camera-system-base.md:69` (Rule — off-screen prevention) | **60.0 uu** |
| `camera-system-base.md:138` (Formula 3 — clamp operation) | **200.0 uu** |
| `camera-system-base.md:183` (Edge Case 4 — hard follow) | **60.0 uu** |
| `camera-system-base.md:204` (Arena Morphing dependency row) | **200 uu** |
| `camera-system-base.md:221` (Tuning Knobs, safe range 40–120) | **60.0 uu** |
| `camera-system-base.md:256` (Acceptance Criterion) | **60.0 uu** |
| `TR-cam-004` (registry) | **200 uu** |
| ADR-0005 (preserves shipped tuning) | **60.0 uu** |

**Impact:** Low today (ADR-0005 and 4 of 6 GDD sites agree on 60, and 200 is outside the GDD's own
declared safe range of 40–120 — so 60 is almost certainly correct). But Formula 3 is the formula an
implementer transcribes, and the Arena Morphing dependency row is a forward contract another GDD
will inherit. Left alone, Arena Morphing gets designed against 200 uu.

**Resolution:** Fix `camera-system-base.md:138` and `:204` to 60.0 uu, revise `TR-cam-004` text.

### 🟡 C-4 — `AMoonCharacterBase` responsibility accretion (CONCERN, not a contradiction)

**Type:** Architecture pattern
**Involved:** ADR-0001 · ADR-0004 · ADR-0005 · ADR-0007

No ADR owns the "what lives on the character vs. a component" boundary, and four ADRs have
independently placed state there:

| Source | What it puts on `AMoonCharacterBase` |
|---|---|
| ADR-0001 | ASC + AttributeSet ownership, `TryExecute()`, Enhanced Input routing |
| ADR-0004 | Overdrive 3-state machine, `OverdriveEndTime`/`RecoveryEndTime`, tag grant/clear, 2 delegates |
| ADR-0005 | FOV `FInterpTo` tick state, execution-blend tick state, `SetOverdriveFOVActive`, camera-settings load |
| ADR-0007 | Dash charge recharge tick (existing), `bJustDodgeSucceededThisActivation` |
| (shipped, no ADR) | Jump state machine, one-shot anim suppression, hitstop timer, locomotion anim swap in `Tick` |

Each individual placement is defensible; ADR-0005 §4 even justifies it explicitly ("consistent with
the existing one-shot-anim/jump-state-machine Tick pattern"). But nothing is tracking the aggregate.
`MoonCharacterBase.cpp` is already the single busiest file in the project and every new ADR adds to it.

**Not blocking.** Recommend the eventual `/create-control-manifest` pass set an explicit rule, or a
small ADR own the component-decomposition boundary before Production.

### ✅ Non-conflicts checked and cleared

- **ADR-0007 amending ADR-0006 in place** (2 accessors + `MeleeAttackRange`): correctly handled.
  ADR-0006's Key Interfaces section carries the additions with attribution, `architecture.yaml`
  records ADR-0006 as owner and ADR-0007 as additive consumer, and ADR-0006 is still Proposed so
  no shipped contract broke. Clean.
- **Non-counted `CostBypass.Active` (ADR-0004) vs ref-counted overlay tags (ADR-0001/0007):**
  different tags, deliberately different semantics, both GDDs state the reasoning. No conflict.
- **Performance budgets:** no ADR allocates a numeric ms budget, so no budget conflict is possible.
  (This is itself a gap — `technical-preferences.md` Performance Budgets is still `[TO BE CONFIGURED]`.)
- **Dependency cycles:** none. Topological sort below is clean.
- **ADR numbering collision** (logged 2026-07-18 as Open): resolved on disk — spell-casting is now
  `0003-spell-casting-gas-implementation.md`. The log entry needs its status flipped to Resolved.

---

## ADR Dependency Order

```
Foundation (no unresolved deps):
  1. ADR-0001  Player Movement and GAS Core          [Accepted]
  2. ADR-0002  Checkpoint Persistence                [Accepted]  ← requires ADR-0001 ✅
                                                                  ⚠ blocked at impl by C-1
  3. ADR-0003  Spell Casting GAS Implementation      [Accepted]  ← requires ADR-0001 ✅

Blocking insertion point:
  ★ NEW: Health/Damage Core death + event contract   [does not exist]
     unblocks ADR-0002 restore path, ADR-0004 PlayerDeath, ADR-0006 Dead state

Ready now:
  4. ADR-0005  Camera System (SpringArm)             [Proposed]  ← Depends On: None
                                                                  → can be Accepted immediately

Gated on ★:
  5. ADR-0004  Luna Overdrive Fixed Window           [Proposed]  ← ADR-0001 ✅, ADR-0003 ✅, ★ for Death
  6. ADR-0006  Enemy AI Behavior Tree                [Proposed]  ← ★ (its "Depends On" literally reads
                                                                  "Health/Damage Core (Approved, no ADR yet)")
  7. ADR-0007  Dash/Evasion Just-Dodge               [Proposed]  ← ADR-0006 (Proposed)
```

**Unresolved dependency flags:**

- ⚠️ **ADR-0006 depends on an ADR that does not exist.** Its `Depends On` field names
  "Health/Damage Core (Approved, no ADR yet — TR-hp-006/007/008 uncovered)". It is honest about
  this, but it means ADR-0006 cannot be safely Accepted while its upstream contract is unwritten.
- ⚠️ **ADR-0007 depends on ADR-0006, which is Proposed.** ADR-0007's own Ordering Note says so.
  Per `docs/CLAUDE.md`, stories referencing a Proposed ADR are auto-blocked — so the entire
  Just-Dodge chain (`ADR-0007 → ADR-0006 → ★`) is a three-deep block.
- ⚠️ **All four new ADRs (0004–0007) are `Proposed`.** None can back a story until Accepted.

**Cycles:** none detected.

---

## GDD Revision Flags (Architecture → Design Feedback)

| GDD | Assumption | Reality | Action |
|---|---|---|---|
| `dash-evasion.md` Rule 4 | "기존 velocity(수평+수직 momentum 전부)를 대쉬 벡터로 완전히 덮어쓴다" (velocity override via Movement API) | ADR-0007 Decision 1 / Alternative 1 ratifies swept `SetActorLocation`; velocity override explicitly rejected and already superseded by commit `7f7e2e3`. Rule 2 of the same GDD already says position step. | **Revise** — restate Rule 4 as override *semantics* delivered by Rule 2's position step |
| `camera-system-base.md` Formula 3 (:138) and Arena Morphing dependency row (:204) | `MaxDistance = 200.0 uu` | Rule (:69), Edge Case 4 (:183), Tuning Knobs (:221, safe range 40–120), and AC (:256) all say 60.0 uu; ADR-0005 preserves 60.0 | **Revise** — 200 is outside the GDD's own safe range |
| `dash-evasion.md` Tuning Knobs | Table has 7 knobs | ADR-0007 introduces `JustDodgeQueryRadius` (500 uu) and `AirDashZImpulse` (TBD), flagged by the ADR itself as needing a GDD addendum | **Addendum** — add both rows |
| `enemy-ai-base.md` Tuning Knobs | No melee hitbox radius exists for either archetype | ADR-0007 requires `MeleeAttackRange` on `UMoonEnemyArchetypeData`; ADR-0006's interface list already carries it | **Addendum** — add `MeleeAttackRange` |
| `dash-evasion.md` header | `Last Updated: 2026-07-17`, Status Approved | Its Rule 2 was changed by the 2026-07-20 instant-step pass without a re-review; C-2 is the visible symptom | **Re-review** via `/design-review design/gdd/dash-evasion.md` |

`enemy-ai-base.md`, `camera-system-base.md`, `player-movement.md`, `health-damage-core.md`,
`combat-hud.md` carry no engine-behaviour contradictions — their gaps are missing ADRs, not wrong
design.

---

## Implementation Violations of Approved GDDs

These are code-vs-GDD, not architecture-vs-GDD. They are in scope here because both are
architectural contracts an Approved GDD already binds, and both would be silently ratified if the
architecture is signed off without noting them.

### 🔴 V-1 — Hitstop uses `CustomTimeDilation`, which `player-movement.md` Rule 9 forbids (blocking-grade)

`player-movement.md` Core Rule 9, marked **blocking** by the 2026-07-16 design review:

> `SetGlobalTimeDilation`이나 액터 단위 `CustomTimeDilation` 등 **어떤 형태의 Time Dilation도
> 사용하지 않음**. 순수 시각적 프리즈(포즈/카메라)로만 구현하며 게임플레이 틱은 히트스탑 중에도
> 100% 정상 틱레이트로 계속됨

Its own acceptance criterion (`player-movement.md:273`) specifies the check as a grep for exactly
this API. Shipped code:

- `Moon/Source/Moon/Character/MoonCharacterBase.cpp:282` — `CustomTimeDilation = FMath::Clamp(DilationScale, 0.001f, 1.0f);`
- `MoonCharacterBase.cpp:161` — called on `Landed()` (the exact landing-hitstop case Rule 9 governs)
- `MoonGameplayAbility_Dash.cpp:211` — called on dash finish

`TR-mov-008` is an uncovered gap, so no ADR ratified this either way. Same failure shape as the
Camera Rule 6 rotation-flag deviation that ADR-0005 caught: **the GDD is correct and the code is
the bug.** Unlike the rotation flags, this one is not stock scaffolding — it was written
deliberately on 2026-07-20 with a comment explaining the choice, which means it is a real design
disagreement, not an oversight. Decide it explicitly: either fix the code to the delta-offset
freeze technique Rule 9 mandates, or revise Rule 9 via `/quick-design` with the rationale.

### 🟠 V-2 — Camera rotation flags (carried forward, still unfixed)

`MoonCharacterBase.cpp:43-44` — `bUseControllerRotationYaw=false`, `bOrientRotationToMovement=true`
contradict `camera-system-base.md` Rule 6 and `player-movement.md` Core Rule 2, both of which
mandate the opposite. Documented in ADR-0005 §Context and ratified there in the GDD's favour.
Tracked as background task `task_0964acf9`, not yet fixed. **Any strafe-aim playtest before this
fix is testing the wrong build** (ADR-0005 §Risks says the same).

---

## Engine Compatibility Audit

**Engine:** Unreal Engine 5.8
**ADRs with an Engine Compatibility section:** 7 / 7 ✅ (`architecture.md`'s claim that ADR-0001
lacks one is stale — ADR-0001 has had one since its 2026-07-18 revision)
**Version consistency:** ✅ all 7 state UE 5.8, none written against an older version
**Post-cutoff API conflicts between ADRs:** none — no two ADRs make contradictory claims about the
same API

### E-1 — Deprecated API in code that ADR-0007 ratifies, contradicting its own audit claim

`deprecated-apis.md:183` ("5.8 Additions — will be removed in 5.9, address now"):

| Deprecated | Replacement |
|---|---|
| `UCharacterMovementComponent::SetMovementMode()` (legacy overload) | `SetMovementModeWithCustomMode()` |

`Moon/Source/Moon/GAS/MoonGameplayAbility_Dash.cpp:187` uses the legacy overload:
```cpp
MoveComp->SetMovementMode(bRestoreFallingMovement ? MOVE_Falling : MOVE_Walking);
```

ADR-0007 ratifies this dash implementation wholesale ("no code change needed here") while its
Engine Compatibility table states **"Post-Cutoff APIs Used: None"** and **"Verification Required:
None beyond standard collision-query correctness at implementation time."** Both are incorrect for
the code it is ratifying.

**Action:** amend ADR-0007's Engine Compatibility table and add the migration to its Migration Plan.

### E-2 — Engine-reference library is missing two GAS deprecations the project already hit

`MoonGameplayAbility_Spell_Blackhole.cpp:17-19` and `..._Fire.cpp:17` correctly use `SetAssetTags()`
with an in-code comment noting `UGameplayAbility::AbilityTags` is `UE_DEPRECATED(5.5)`. The
`GE_SpellCooldown_*` effects use the `UTargetTagsGameplayEffectComponent` architecture.

Neither appears anywhere in `docs/engine-reference/unreal/deprecated-apis.md`. The code is right;
the reference library the ADRs audit against is incomplete, so ADR-0003's clean deprecation audit
was checking against a source that could not have caught it. A background task
(`task_a3fdf7fc`, per session state) was started to write these up — **verify it landed** before
relying on that file for the next audit.

### E-3 — Two ADRs carry unverifiable-by-reference claims (both self-flagged, correctly)

| ADR | Claim | Status |
|---|---|---|
| ADR-0004 | `SetLooseGameplayTagCount` UE5.8 signature/semantics | Not in engine-reference. Flagged in the ADR, in `luna-overdrive.md` Open Questions, and in `architecture.md` QQ-03. **Automation tests for the fixed window passed 2/2 (spike, 2026-07-21) — but those test the time predicates, not the tag API.** Still unverified. |
| ADR-0006 | `UAIPerceptionComponent` / `UAISenseConfig_Sight` / `UAISenseConfig_Hearing` signatures, and the claim that native Hearing Loudness-scaling maps 1:1 onto the GDD's `BaseHearingRadius × NoiseLoudness` formula | **`docs/engine-reference/unreal/modules/ai.md` does not exist**, despite `navigation.md` cross-referencing it. Decision point 4 is a load-bearing assumption with zero documentary backing. ADR-0006 flags this and recommends authoring the missing module doc. |

E-3's ADR-0006 half is the higher risk: if native Loudness scaling does **not** work as assumed, the
"no custom radius math needed" decision inverts and `spell-casting-base.md` AC7 (the observable
800uu/480uu hearing test) fails.

### Inherited HIGH-risk item (unchanged)

Legacy GAS attribute-set initialization is deprecated in 5.8 (`deprecated-apis.md:185`) with the
replacement pattern undocumented. Flagged by ADR-0001, ADR-0002, three GDDs, and `architecture.md`
QQ-02. Still unresolved, still requires a real-header cross-check before implementation.

---

## Architecture Document Coverage (`architecture.md`)

`docs/architecture/architecture.md` is **stale — version 1.0, dated 2026-07-18**, and its accuracy
has decayed materially:

| Section | Problem |
|---|---|
| Header: "ADRs Referenced: ADR-0001" | 6 ADRs written since |
| ADR Audit table | Single row, ADR-0001, marked **Proposed** — it has been Accepted since 2026-07-20. Claims it lacks ADR Dependencies / Engine Compatibility / GDD Requirements Addressed sections; **it has all three.** |
| Traceability Coverage | "19 baseline requirements" — the registry holds 76 |
| Required ADRs list | Lists Enemy AI, Spell Casting, Dash/Evasion, Luna Overdrive, Save/Persistence as still-needed. All five now exist (0002/0003/0004/0006/0007). |
| Module Ownership → Dash/Evasion row | "Engine APIs: `LaunchCharacter`/velocity override" — the model ADR-0007 rejected |
| Data Flow §3 (Save/load) | "**GAP — no Persistence system exists**" — ADR-0002 has covered this since 2026-07-18 |
| Open Question QQ-01 | Persistence undesigned — resolved by ADR-0002 |
| Open Question QQ-04 | ADR-0001 missing sections — resolved |

**Structurally still correct and worth keeping:** the layer map, module ownership boundaries, data
flow event graph, initialization order, the 5 architecture principles, and the API boundary sketch.
All still match `systems-index.md` and the current ADR set. Only the audit/coverage/status
bookkeeping has rotted.

**Systems coverage check:** all 9 MVP systems from `systems-index.md` appear in the layer map ✅.
No orphaned architecture (nothing in the doc lacks a GDD) ✅. Non-MVP systems (Destructible
Geometry, Status Effect, Core Extraction Execution, Arena Morphing, …) are correctly absent —
out of index scope.

→ Recommend `/create-architecture` refresh, or at minimum a bookkeeping pass, **after** the ★ ADR
and the ADR-0005 Accept.

---

## Registry Drift (`tr-registry.yaml`)

The 2026-07-23 GDD re-reviews changed contracts that the registry still records in their pre-revision
form. **Two entries now state the exact opposite of their own Approved GDD.** Any story generated
from these today would implement the inverted rule.

### Supersede vs. revise

`tr-registry.yaml`'s own rules (lines 8–14) distinguish two cases: a **reword with the same intent**
gets an in-place text update plus a `revised:` date and keeps its ID; a requirement that is
**replaced** gets `status: superseded-by` and a new ID. Two of the five drifted entries had their
behavioural contract *inverted*, not reworded — those are replacements under the registry's own rule,
regardless of how much surrounding text survived.

**Story-impact check performed:** `production/epics/` does not exist and no story files exist
anywhere in the repo. `grep` for the five affected IDs returns only the registry, the traceability
index, the two review reports, and ADR-0003 — **no story or epic cites any of them.** So the
practical risk of either approach is currently zero; the split below follows the registry's stated
rule rather than the cheaper path, so the audit trail stays honest once stories do exist.

| TR-ID | Registry says (v2, 2026-07-18) | GDD now says | Action |
|---|---|---|---|
| **TR-overdrive-005** | "re-trigger **refreshes** `OverdriveEndTime`" | `luna-overdrive.md` Rule 5 + AC5: re-trigger is **ignored**, `EndTime` unchanged (fixed 10s window). ADR-0004 §2 agrees. | **Supersede** → `status: superseded-by: TR-overdrive-009` |
| **TR-tension-006** | "Read `CostBypass.Active` gameplay tag at gain-evaluation time to gate the overdrive gain **multiplier**" | `combo-tension-gauge.md` Rule 7: **do not read the tag**; query Luna Overdrive's time state. **No multiplier** — full lock. The GDD header records `overdrive_tension_gain_multiplier` as deprecated in the constant registry. | **Supersede** → `status: superseded-by: TR-tension-008` |
| **TR-spell-004** | "skip `CommitAbility` entirely" when `CostBypass.Active` present | `spell-casting-base.md` Rule 10 (revised): bypass requires tag **AND** `CurrentTime < OverdriveEndTime`, checked at both gates. Tag alone on the expiry frame does **not** bypass. | **Revise in place** (added conjunct, same intent) — remains ✅ |
| **TR-dash-002** | "full velocity Override … via Player Movement impulse/Velocity API" | See C-2 — ADR-0007 ratifies swept `SetActorLocation` | **Revise in place** (corrected mechanism) — *after* the `dash-evasion.md` Rule 4 fix |
| **TR-cam-004** | "hard `CameraLagMaxDistance`=**200uu** cap" | See C-3 — 60.0 uu in 4 of 6 GDD sites and in ADR-0005 | **Revise in place** (corrected number) — *after* the `camera-system-base.md` fix |

### New requirements to register (4)

| Proposed TR-ID | System | Requirement | Source | Coverage |
|---|---|---|---|---|
| **TR-overdrive-008** | overdrive | Three-state boundary Inactive/Active/**Recovery**; Recovery (`OverdriveRecoveryDuration`, default 1.5s) locks tension gain only — mana regen, paid casts, movement, dash all resume immediately | `luna-overdrive.md` Rule 8 + States table (2026-07-23 rewrite) | ✅ ADR-0004 §5/§7 |
| **TR-overdrive-009** | overdrive | *(supersedes TR-overdrive-005)* Fixed-window race determinism: re-trigger during Active/Recovery is **ignored** and never refreshes `EndTime`/`RecoveryEndTime` or re-emits Started/Ended; expiry wins a same-frame tie with re-trigger; player Death takes eager priority | `luna-overdrive.md` Rule 5 + Edge Cases + AC5/AC9 (2026-07-23) | ✅ ADR-0004 §2/§5 |
| **TR-tension-008** | tension | *(supersedes TR-tension-006)* Gate all tension gain to zero during Luna Overdrive Active **and** Recovery by querying Luna Overdrive's **time state**, not the `CostBypass.Active` tag; no partial multiplier exists; `LastTensionGainTime` is not updated by suppressed events | `combo-tension-gauge.md` Rule 7 + AC10 (2026-07-23) | ✅ ADR-0004 §3 |
| **TR-spell-010** | spell | Passive Mana Regen pauses **only** while Luna Overdrive is Active (entry Mana preserved, no retroactive drain); resumes on Recovery entry | `spell-casting-base.md` Rule 7 + Mana Regen Tick formula (revised 2026-07-23), `luna-overdrive.md` Rule 9 | ✅ ADR-0004 §4 |

### `architecture.yaml` — checked, no drift

`docs/registry/architecture.yaml` (v1, `last_updated: 2026-07-23`) was grepped for the same
revision-sensitive terms (`refresh`, `multiplier`, `CameraLagMaxDistance`, `재트리거`,
`velocity override`, `LaunchCharacter`). One hit, and it is **correct**: `forbidden_patterns`
line 252 already records `LaunchCharacter`/velocity override as superseded by commit `7f7e2e3`,
matching ADR-0007. The file carries no stale re-trigger, multiplier, or camera-lag values.
**No changes needed.**

> ⚠️ `docs/registry/architecture.yaml.tmp.13448.bc1ec0e0fd01` is a leftover from an interrupted
> atomic write on 2026-07-23 (10:11, 15.0 KB) — the canonical file is newer and larger
> (10:23, 16.7 KB) and contains all ADR-0005/0006/0007 entries the `.tmp` lacks. **Left untouched
> and uncommitted.** Deleting it needs explicit authorisation; it is not safe to assume it is junk
> without the user confirming.
>
> ⚠️ A stale git worktree at `.claude/worktrees/cranky-shamir-f0f913/` holds an old copy of
> `tr-registry.yaml` and `traceability-index.md`. It matches repo-wide greps for TR-IDs and will
> produce false hits in future audits. Untouched here — flagged only so the next reviewer does not
> mistake it for a live document.

---

## Systems Index Correction

`design/gdd/systems-index.md` Progress Tracker contradicts the GDD files themselves.

| Metric | Current | Should be | Evidence |
|---|---|---|---|
| Design docs approved | 6 | **9** | All 9 MVP GDD headers read `Status: Approved` |
| MVP systems designed | `9/9 (6 Approved, 3 Needs Revision Review after 2026-07-21 fixed-window Overdrive change)` | `9/9 (9 Approved)` | `combo-tension-gauge.md`, `luna-overdrive.md`, `spell-casting-base.md` all re-reviewed and re-approved 2026-07-23 (commit `afe5e3e`) |

Also stale: `Status: Draft` and `Last Updated: 2026-07-20` in the header.

---

## Blocking Issues (must resolve before PASS)

1. **C-1** — write the Health/Damage Core death + event contract ADR (★). Unblocks ADR-0002,
   ADR-0004, ADR-0006 and closes TR-hp-006/007.
2. **Player Movement Foundation gaps** — 7 of 10 TRs uncovered. ADR-0001 is titled
   "Player Movement and GAS Core" but its Decision text is GAS-only; it addresses exactly
   TR-mov-001 and TR-mov-004 out of ten.
3. **Combo/Tension Gauge** — 5 of 7 uncovered, including TR-tension-005, the emission of the
   event ADR-0004 is built to consume.
4. **Combat HUD** — 0 of 7 covered, while the widget is already built in-engine.
5. **All four new ADRs are Proposed** — none can back a story. ADR-0005 has no unresolved
   dependency and can be Accepted today.

## Non-blocking but should be fixed in the same pass

6. **V-1** — decide the hitstop Time Dilation question explicitly (fix code or revise Rule 9).
7. **C-2 / C-3** — two Approved GDDs each contradict themselves internally.
8. **Registry drift** — 5 affected TRs: 2 supersede (contract inverted), 3 revise in place.
9. **`systems-index.md`** — Progress Tracker says 6 Approved, reality is 9.
10. **`architecture.md`** — bookkeeping sections stale by 6 ADRs.
11. **E-1** — ADR-0007's "Post-Cutoff APIs Used: None" is wrong.
12. **`consistency-failures.md`** — the ADR-numbering entry is still marked Open; it is resolved.

---

## Required ADRs (prioritised, most foundational first)

| # | ADR | Layer | Closes | Why now |
|---|---|---|---|---|
| 1 | **Health/Damage Core — death detection and event contract** | Foundation | TR-hp-006, TR-hp-007, TR-hp-008 | Unblocks three existing ADRs. Nothing else moves until this exists. |
| 2 | **Player Movement — runtime contract** (airborne sub-states, input grace windows, velocity injection API, MovementLocked ownership, hitstop model, latency budget, root-motion policy) | Foundation | TR-mov-002/003/005/006/007/008/009/010 | Largest remaining Foundation block; also the natural home for the V-1 decision |
| 3 | **Combo/Tension Gauge** — attribute, decay, ordering, trigger emission | Feature | TR-tension-002/003/004/005/007 | ADR-0004's upstream producer |
| 4 | **Combat HUD** — UMG/CommonUI widget architecture | Presentation | TR-hud-001..007 | Partly reverse-documentation of `WBP_CombatHUD` as built |

Plus one non-ADR item: `docs/engine-reference/unreal/modules/ai.md` does not exist and ADR-0006
depends on assumptions it would document (E-3).

---

## Pre-Gate Checklist

| Item | Status |
|---|---|
| `tests/unit/` | ⚠️ exists, `.gitkeep` only — no test files |
| `tests/integration/` | ⚠️ exists, `.gitkeep` only — no test files |
| `.github/workflows/tests.yml` | ❌ missing |
| `design/accessibility-requirements.md` | ❌ missing |
| `design/ux/interaction-patterns.md` | ❌ missing |

`/gate-check pre-production` cannot be offered — run `/test-setup` and `/ux-design` first, and clear
the blocking issues above.

---

## Recommended Next Actions

1. **Fresh session:** `/architecture-decision Health/Damage Core death and event contract` (★)
2. **Accept ADR-0005** — no unresolved dependencies, 9/9 camera TRs addressed, ready today
3. **Same pass as (1):** fix registry drift (2 supersede + 3 revise), register the 4 new TRs, fix
   `systems-index.md` Progress Tracker, flip the `consistency-failures.md` numbering entry to Resolved
4. Decide **V-1** (hitstop Time Dilation) — code fix or GDD revision, not silence
5. Re-run `/architecture-review` after ★ lands

> Re-run this review after each new ADR to confirm coverage actually improves — the jump from
> 12% to 51% came from four ADRs written in one day; the remaining 30% is concentrated in two
> systems (Player Movement, Combat HUD) plus one blocking contract.
