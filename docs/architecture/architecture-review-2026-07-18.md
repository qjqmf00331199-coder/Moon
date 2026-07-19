# Architecture Review Report

- **Date:** 2026-07-18
- **Engine:** Unreal Engine 5.8
- **GDDs Reviewed:** 9 (player-movement, camera-system-base, health-damage-core, enemy-ai-base, spell-casting-base, dash-evasion, combo-tension-gauge, luna-overdrive, combat-hud)
- **ADRs Reviewed:** 3 (ADR-0001, ADR-0002-spell, ADR-0002-checkpoint)
- **Mode:** full / solo
- **Verdict:** 🔴 **FAIL**

---

## Traceability Summary

74 technical requirements extracted (finer-grained than architecture.md's 19 conversational baseline — expected, not an error).

| Status | Count |
|--------|-------|
| ✅ Covered | 9 |
| ⚠️ Partial | 6 |
| ❌ Gap | 59 |

### Coverage by system

| System | Layer | TRs | ADR(s) | Result |
|--------|-------|-----|--------|--------|
| Player Movement | Foundation | 10 | ADR-0001 | ⚠️ 2 partial, 8 gap |
| Health/Damage Core | Foundation | 9 | ADR-0001, ADR-0002-checkpoint | ✅ 4, ⚠️ 2, ❌ 3 |
| Camera System (base) | Core | 9 | — | ❌ 9 gap |
| Enemy AI (base) | Core | 8 | — | ❌ 8 gap |
| Spell Casting (base) | Core | 9 | ADR-0002-spell | ✅ 5, ⚠️ 2, ❌ 2 |
| Dash/Evasion | Core | 8 | — | ❌ 8 gap |
| Combo/Tension Gauge | Feature | 7 | — | ❌ 7 gap |
| Luna Overdrive | Feature | 7 | — | ❌ 7 gap |
| Combat HUD | Presentation | 7 | — | ❌ 7 gap |

Only the Foundation layer has real ADR coverage. Three Core-layer systems (Camera, Enemy AI, Dash/Evasion) have **zero** ADR coverage.

---

## Traceability Matrix

| TR-ID | GDD | Requirement (short) | ADR Coverage | Status |
|-------|-----|---------------------|--------------|--------|
| TR-mov-001 | player-movement | CMC + camera-relative input + facing decouple | ADR-0001 | ⚠️ |
| TR-mov-002 | player-movement | Movement module compile-independent of SpellCasting | — | ❌ |
| TR-mov-003 | player-movement | Airborne substate via Velocity.Z sign | — | ❌ |
| TR-mov-004 | player-movement | Data-driven tuning with hard clamps + joint bound | ADR-0001 | ⚠️ |
| TR-mov-005 | player-movement | External velocity/Z-launch injection API | — | ❌ |
| TR-mov-006 | player-movement | MovementLocked access restricted to Status Effect | — | ❌ |
| TR-mov-007 | player-movement | Jump buffer / coyote timers (delta-time) | — | ❌ |
| TR-mov-008 | player-movement | Hitstop/execution = no Time Dilation | — | ❌ |
| TR-mov-009 | player-movement | Input→velocity latency budget ≤33ms p95 | — | ❌ |
| TR-mov-010 | player-movement | Non-root-motion locomotion anims | — | ❌ |
| TR-hp-001 | health-damage-core | GAS ASC+AttributeSet, single ApplyDamage entry | ADR-0001 | ✅ |
| TR-hp-002 | health-damage-core | Shield/armor intercept in ExecCalc + bBypassDefense | ADR-0001 | ✅ |
| TR-hp-003 | health-damage-core | State.Invulnerable i-frame gating | ADR-0001 | ✅ |
| TR-hp-004 | health-damage-core | Ref-counted overlay tags, auto-clear on death | ADR-0001 | ⚠️ |
| TR-hp-005 | health-damage-core | State.Executable + TryExecute API | ADR-0001 | ⚠️ |
| TR-hp-006 | health-damage-core | Immediate presentation-decoupled death detection | — | ❌ |
| TR-hp-007 | health-damage-core | Event exposure (OnDeath/OnExecuted/OnHealthPercentCrossed) | — | ❌ |
| TR-hp-008 | health-damage-core | Runtime MaxHealth reclamp (absolute) | — | ❌ |
| TR-hp-009 | health-damage-core | Death = instant checkpoint respawn (no reload) | ADR-0002-checkpoint | ✅ |
| TR-cam-001 | camera-system-base | SpringArm→Camera hierarchy, controller-driven | — | ❌ |
| TR-cam-002 | camera-system-base | IA_Look routing + pitch clamp via CameraManager | — | ❌ |
| TR-cam-003 | camera-system-base | Camera-relative movement basis (resolves mutual dep) | — | ❌ |
| TR-cam-004 | camera-system-base | Camera lag + CameraLagMaxDistance cap | — | ❌ |
| TR-cam-005 | camera-system-base | Collision test + separate destructible channel | — | ❌ |
| TR-cam-006 | camera-system-base | Overdrive FOV / execution blend as render overlay | — | ❌ |
| TR-cam-007 | camera-system-base | ResetCameraLag() on teleport/checkpoint | — | ❌ |
| TR-cam-008 | camera-system-base | Look-input suppression + camera-shake caps | — | ❌ |
| TR-cam-009 | camera-system-base | All camera params data-asset driven | — | ❌ |
| TR-ai-001 | enemy-ai-base | AIController+BT/StateTree, 6-state machine | — | ❌ |
| TR-ai-002 | enemy-ai-base | AIPerception Sight+Hearing (open MakeNoise contract) | — | ❌ |
| TR-ai-003 | enemy-ai-base | OnAttackTelegraphed/Committed delegates, commit-frame damage | — | ❌ |
| TR-ai-004 | enemy-ai-base | Consume HDC OnDeath as sole Dead transition | — | ❌ |
| TR-ai-005 | enemy-ai-base | TriggerStagger/ClearStagger interrupt hooks | — | ❌ |
| TR-ai-006 | enemy-ai-base | Archetype tags + read-only overlay tags | — | ❌ |
| TR-ai-007 | enemy-ai-base | NavMesh pursuit + pathfind-fail timeout | — | ❌ |
| TR-ai-008 | enemy-ai-base | Swarm (dozens) within 60fps budget (unverified) | — | ❌ |
| TR-spell-001 | spell-casting-base | GAS pipeline, InstancedPerActor, same-frame | ADR-0002-spell | ✅ |
| TR-spell-002 | spell-casting-base | Never MovementLocked, 100% movement during cast | ADR-0002-spell | ✅ |
| TR-spell-003 | spell-casting-base | Per-element cooldowns, shared Mana | ADR-0002-spell | ✅ |
| TR-spell-004 | spell-casting-base | CostBypass.Active checked in both gates | ADR-0002-spell | ✅ |
| TR-spell-005 | spell-casting-base | Cast-rate limiter (per-frame + MaxCastsPerSecond) | ADR-0002-spell | ✅ |
| TR-spell-006 | spell-casting-base | OnExecuted mana snap + regen + clamp | — | ❌ |
| TR-spell-007 | spell-casting-base | MakeNoise per impact + route via ApplyDamage | ADR-0002-spell | ⚠️ |
| TR-spell-008 | spell-casting-base | Expose cast/hit events + state for HUD | — | ❌ |
| TR-spell-009 | spell-casting-base | Deterministic same-frame tag→gate ordering | ADR-0002-spell | ⚠️ |
| TR-dash-001 | dash-evasion | Fractional charge accumulation | — | ❌ |
| TR-dash-002 | dash-evasion | Velocity Override via Movement impulse API | — | ❌ |
| TR-dash-003 | dash-evasion | Grant/remove State.Invulnerable i-frames | — | ❌ |
| TR-dash-004 | dash-evasion | Just-Dodge window + spatial test off AI telegraph | — | ❌ |
| TR-dash-005 | dash-evasion | Grant State.Executable, cap 1 charge refund | — | ❌ |
| TR-dash-006 | dash-evasion | Read camera-relative dir + trigger camera shake | — | ❌ |
| TR-dash-007 | dash-evasion | Respect MovementLocked, fire mid-cast | — | ❌ |
| TR-dash-008 | dash-evasion | Expose charge/cooldown to HUD | — | ❌ |
| TR-tension-001 | combo-tension-gauge | GAS TensionGauge attribute, event-driven only | — | ❌ |
| TR-tension-002 | combo-tension-gauge | Read-only subscribe to upstream event delegates | — | ❌ |
| TR-tension-003 | combo-tension-gauge | Decay timer w/ grace, ticks during Invulnerable | — | ❌ |
| TR-tension-004 | combo-tension-gauge | Deterministic Gain→Penalty→Decay ordering + clamp | — | ❌ |
| TR-tension-005 | combo-tension-gauge | OnOverdriveTriggered once-per-frame at Max, reset | — | ❌ |
| TR-tension-006 | combo-tension-gauge | Read CostBypass.Active to gate gain multiplier | — | ❌ |
| TR-tension-007 | combo-tension-gauge | Expose read-only value + state to HUD | — | ❌ |
| TR-overdrive-001 | luna-overdrive | Subscribe OnOverdriveTriggered, enter Active | — | ❌ |
| TR-overdrive-002 | luna-overdrive | SetLooseGameplayTagCount(1/0) sole grantor (non-counted) | — | ❌ |
| TR-overdrive-003 | luna-overdrive | Timer-variable duration (no Duration-GE owns tag) | — | ❌ |
| TR-overdrive-004 | luna-overdrive | Lazy CurrentTime>=EndTime evaluation at all points | — | ❌ |
| TR-overdrive-005 | luna-overdrive | Same-frame race determinism (expiry/refresh/death) | — | ❌ |
| TR-overdrive-006 | luna-overdrive | Death cancels timer + clears tag, idempotent | — | ❌ |
| TR-overdrive-007 | luna-overdrive | Expose Started/Ended(EndReason) + TimeRemaining | — | ❌ |
| TR-hud-001 | combat-hud | UMG+CommonUI read-only, non-focusable widgets | — | ❌ |
| TR-hud-002 | combat-hud | Bind only to existing upstream interfaces | — | ❌ |
| TR-hud-003 | combat-hud | Event-driven, no idle tick, 60fps budget | — | ❌ |
| TR-hud-004 | combat-hud | Coalesce per-frame updates, last-value-wins | — | ❌ |
| TR-hud-005 | combat-hud | Signals off real values, not interpolated display | — | ❌ |
| TR-hud-006 | combat-hud | Mirror upstream death/overdrive transitions | — | ❌ |
| TR-hud-007 | combat-hud | Device-detect key glyph swap (5.8 unified Input) | — | ❌ |

**Totals: 9 ✅ / 6 ⚠️ / 59 ❌ (74 total).**

---

## Coverage Gaps (no ADR exists)

### Core layer (highest priority — blocks FAIL→PASS)

- **Camera System (base)** — TR-cam-001..009. Suggested: `/architecture-decision Camera System (SpringArm + data-driven modes)`. Domain: Camera/Input. Engine Risk: LOW-MEDIUM.
- **Enemy AI (base)** — TR-ai-001..008. Suggested: `/architecture-decision Enemy AI (BT/Perception + archetype data)`. Domain: AI. Engine Risk: MEDIUM (swarm perf unverified — TR-ai-008).
- **Dash/Evasion** — TR-dash-001..008. Suggested: `/architecture-decision Dash/Evasion (impulse + Just-Dodge)`. Domain: Physics/GAS. Engine Risk: LOW-MEDIUM.

### Foundation layer (partially covered — close the gaps)

- Player Movement TR-mov-002/003/005/006/007/008/009/010 uncovered — ADR-0001 is effectively a *GAS-foundation* ADR despite its title; movement specifics need either an ADR-0001 expansion or a dedicated Movement ADR.
- Health/Damage Core TR-hp-006/007/008 uncovered — death detection, event exposure, MaxHealth reclamp. TR-hp-006/007 also block ADR-0002-checkpoint (see conflicts).

### Feature / Presentation layer (lower priority)

- Combo/Tension Gauge TR-tension-001..007. Suggested: `/architecture-decision Combo/Tension Gauge (attribute + decay)`. Engine Risk: LOW.
- Luna Overdrive TR-overdrive-001..007. Suggested: `/architecture-decision Luna Overdrive (non-counted loose tag ownership)`. Engine Risk: MEDIUM (`SetLooseGameplayTagCount` signature unverified).
- Combat HUD TR-hud-001..007. Suggested: `/architecture-decision Combat HUD (UMG/CommonUI mirror)`. Engine Risk: LOW-MEDIUM (5.8 unified Input glyph swap).

---

## Cross-ADR Conflicts

### 🔴 CONFLICT 1 — Duplicate ADR number (numbering collision)

Type: Integration / process.
Two files both claim **ADR-0002**: `0002-spell-casting-gas-implementation.md` and `0002-checkpoint-persistence.md`.
Impact: Story references and dependency-order fields become ambiguous — "ADR-0002" resolves to two different decisions.
Resolution: Renumber one. Checkpoint ADR is the more complete / template-conformant → renumber the spell ADR to **ADR-0003**.

### 🔴 CONFLICT 2 — Checkpoint ADR depends on an ADR-0001 hook that doesn't exist

Type: Integration contract.
ADR-0002-checkpoint Decision §3 states death handling is "already gated through the `UGameplayEffectExecutionCalculation` damage pipeline **per ADR-0001**" and calls `AMoonCharacterBase::OnDeath()`.
ADR-0001 defines only the damage ExecutionCalculation — it does **not** define a death event/hook (`OnDeath`). This is a forward-reference to functionality that does not yet exist (also surfaces as gaps TR-hp-006/007).
Impact: Checkpoint restore cannot be wired without an actual death-broadcast entry point; implementer would have to invent the contract.
Resolution options:
1. Add a **Death Detection** subsection to ADR-0001 (ExecCalc detects `Health<=0` → fires multicast `OnDeath`), then reference it from checkpoint.
2. Downgrade checkpoint's assumption to an explicit open dependency item until ADR-0001 defines the hook.

---

## ADR Dependency Order

`Depends On` graph:
- ADR-0001 — no dependencies (Foundation).
- ADR-0002-checkpoint — Depends On ADR-0001.
- ADR-0002-spell — no explicit ADR Dependencies section (missing), implicitly depends on ADR-0001's ASC/AttributeSet.

Recommended implementation order (topological):
1. ADR-0001 (Player Movement + GAS Core Foundation)
2. ADR-0002-spell → renumber ADR-0003 (Spell Casting GAS) — requires ADR-0001
3. ADR-0002-checkpoint (Checkpoint Persistence) — requires ADR-0001

⚠️ **All three ADRs are `Status: Proposed`.** Per `docs/CLAUDE.md`, any story referencing a Proposed ADR is auto-blocked. **Nothing is implementable until they reach `Accepted`.**

No dependency cycles detected.

---

## GDD Revision Flags

**None — all GDD assumptions are consistent with verified engine behaviour.**

The HIGH-RISK engine items (GAS attribute-set init deprecation, `SetLooseGameplayTagCount` signature) are already carried as explicit Open Questions inside `health-damage-core.md`, `spell-casting-base.md`, and `luna-overdrive.md`. A GDD that flags the uncertainty is consistent with engine reality — it does not contradict it. No systems-index change required.

---

## Engine Compatibility Issues

### Engine Audit Results

- Engine: Unreal Engine 5.8.
- ADRs with an Engine Compatibility section: **1 / 3** (only ADR-0002-checkpoint).
- Missing Engine Compatibility section: ADR-0001, ADR-0002-spell.
- Missing ADR Dependencies section: ADR-0001, ADR-0002-spell.
- Missing GDD Requirements Addressed section: ADR-0001, ADR-0002-spell.
- Deprecated API references in ADRs: none directly named.
- Stale version references: none (all state UE 5.8).

### Confirmed engine risks

- **GAS attribute-set initialization** — UE 5.8 deprecated the legacy init functions; replacement pattern undocumented in engine-reference. ADR-0001 never names an init mechanism. Needs `ue-gas-specialist` WebSearch verification before implementation.
- **`SetLooseGameplayTagCount`** — zero engine-reference coverage in this pin (not in `plugins/gameplay-ability-system.md`, `deprecated-apis.md`, or `breaking-changes.md`). Verify signature before Luna Overdrive.
- **Documentation debt** — `docs/engine-reference/unreal/plugins/gameplay-ability-system.md` is titled UE 5.7 (last verified 2026-02-13), predates the 5.8 pin, and shows constructor-based attribute defaults that conflict with the 5.8 init deprecation. Refresh before specialists trust its code samples.

### Engine Specialist Findings (unreal-specialist)

ADR-quality / correctness findings — block `Accepted` status, not traceability rows:

- **ADR-0002-spell Decision #2 is internally undecided:** "override `CommitAbility` **or** implement custom `CommitExecute`." Not interchangeable — `CommitExecute` runs *after* the base cost/cooldown check, so bypass logic there either double-applies or silently fails to skip. Bypass must live in `CanActivateAbility`/`CommitCheck`. Resolve to one concrete override point before Accepted.
- **ADR-0002-spell `InstancedPerActor` justification conflates instancing policy with re-activation.** Risk of stacked/orphaned instances if a montage-driven instance outlives its frame while a rapid same-element refire arrives, unless `EndAbility` is deterministic every activation. State whether `NonInstanced` was considered/rejected.
- **ADR-0001 never addresses attribute clamping** (`PreAttributeChange`/`PostGameplayEffectExecute` for Health≤MaxHealth, Mana≤MaxMana) — the single most common GAS bug class. Add as explicit requirement before Accepted.
- **ADR-0002-checkpoint snapshots bare status-effect tags**, losing the underlying GameplayEffect (duration remaining, periodic tick, magnitude). Duration-based debuffs desync on restore. Either scope out (tags list always empty at capture) or snapshot actual GE spec data.

Positive / sound-pattern confirmations:
- Checkpoint restore correctly routes through GameplayEffect application (not direct attribute writes), and correctly rejected PlayerState-owned checkpoint data (Alternative C) for consistency with ADR-0001's Character-owned ASC. No issue.

Watch-items (non-blocking):
- ADR-0001 should state explicitly that `InitAbilityActorInfo(this, this)` happens once in `BeginPlay`/`PossessedBy` for a single-player, locally-owned ASC (don't copy the reference doc's replicated `OnRep_PlayerState` split).
- When UI work starts, review ADR-0001's Enhanced Input setup against 5.8's unified Enhanced Input + CommonInput (removes duplicate data-asset requirements).

---

## Architecture Document Coverage

`docs/architecture/architecture.md` (v1.0, 2026-07-18) is **stale**:

- Its ADR Audit table and "Required ADRs" section reference only **ADR-0001** and list persistence (QQ-01) and "Spell Casting GAS ability structure" as *gaps / required-new*. Both are now addressed by ADR-0002-checkpoint and ADR-0002-spell respectively.
- Layer map and dependency flow still match `systems-index.md` — those remain valid.
- Action: update the ADR Audit table + Required ADRs + Open Questions (QQ-01 partially resolved) to reflect the two new ADRs.

No orphaned architecture (every architecture-doc system maps to a systems-index entry).

---

## Verdict: 🔴 FAIL

Two independent grounds:
1. **Core-layer coverage gap** — Camera, Enemy AI, Dash/Evasion (all Core layer) have zero ADR coverage. Skill definition: FAIL when Foundation/Core requirements are uncovered.
2. **Blocking cross-ADR conflicts** — duplicate ADR-0002 number, and all three ADRs stuck at `Proposed` (auto-blocks every story).

## Blocking Issues (must resolve before PASS)

1. Renumber duplicate ADR-0002 (spell → ADR-0003).
2. Promote all three ADRs to `Accepted` after backfilling required sections — else nothing is implementable.
3. Resolve CONFLICT 2 — add Death Detection subsection to ADR-0001 (or downgrade checkpoint's assumption).
4. Author the three missing Core ADRs (Camera, Enemy AI, Dash/Evasion).

## Required ADRs (prioritised, most foundational first)

1. **Camera System** — Core, unblocks camera-relative movement + execution/overdrive presentation.
2. **Enemy AI** — Core, unblocks Dash Just-Dodge dependency chain.
3. **Dash/Evasion** — Core, depends on Enemy AI telegraph + Camera + HDC tags.
4. Combo/Tension Gauge — Feature.
5. Luna Overdrive — Feature (verify `SetLooseGameplayTagCount` first).
6. Combat HUD — Presentation.
7. (Revise) ADR-0001 expansion for movement specifics + Death Detection + attribute clamping.

## Non-review follow-ups

- `ue-gas-specialist`: verify UE 5.8 GAS attribute-init pattern + `SetLooseGameplayTagCount` signature.
- Refresh `plugins/gameplay-ability-system.md` to the 5.8 pin.
- Update `docs/architecture/architecture.md` ADR Audit to reflect ADR-0002-spell / ADR-0002-checkpoint.
