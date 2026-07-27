# ADR-0010: Combat HUD — Widget Architecture and Data Binding Contract

## Status
Accepted

**Accepted:** 2026-07-27 (fresh `/architecture-review` delta pass in
`architecture-review-2026-07-27-v4.md` — closes TR-hud-001..007 and TR-dash-008,
no unresolved blocking dependency; all `Depends On` ADRs are already Accepted.
Verification Required item for UE5.8 CommonUI glyph/input-method details remains
an implementation-time risk, non-blocking, consistent with ADR-0004/0006/0008/0009.)

## Date
2026-07-27

## Engine Compatibility

| Field | Value |
|-------|-------|
| **Engine** | Unreal Engine 5.8 |
| **Domain** | UI (UMG / CommonUI) |
| **Knowledge Risk** | MEDIUM — `docs/engine-reference/unreal/modules/ui.md` is only verified to 5.7. The one verified 5.8-era change relevant to this domain (`breaking-changes.md` "5.7→5.8 Delta") is the Enhanced Input / Common Input unification, which directly affects TR-hud-007's glyph-swap mechanism (Decision 7). No breaking change is documented for plain UMG `UUserWidget` usage. |
| **References Consulted** | `docs/engine-reference/unreal/VERSION.md`, `breaking-changes.md` (5.7→5.8 Delta — input consolidation), `deprecated-apis.md` (no UI-domain entries), `modules/ui.md` |
| **Post-Cutoff APIs Used** | None directly. `UCommonInputSubsystem`'s post-unification API surface (Decision 7) is referenced but not verified against real 5.8 headers — see Verification Required. |
| **Verification Required** | `UCommonInputSubsystem`'s input-method-changed delegate name/signature, **and** where the execution-prompt glyph asset itself lives post-5.8-unification (the pre-5.8 CommonUI glyph-data-asset pattern may be one of the "duplicate data asset requirements" the 5.8 delta removed) — both must be confirmed against real 5.8 CommonUI headers before implementation, `ue-umg-specialist` sign-off required (this is the same Open Question `combat-hud.md` already flags for its `gameplay-programmer` owner, scope widened by this ADR's own engine-specialist validation pass). |

## ADR Dependencies

| Field | Value |
|-------|-------|
| **Depends On** | ADR-0001 (Accepted, GAS ASC/AttributeSet incl. shared `TensionGauge`/`DashCharges` attributes), ADR-0003 (Accepted, Spell Casting — extended by Decision 2 below), ADR-0004 (Accepted, Luna Overdrive `OnOverdriveStarted`/`OnOverdriveEnded` signal), ADR-0007 (Accepted, Dash `DashChargesAttribute`), ADR-0008 (Accepted, Health/Damage Core `OnDeath`/`OnHealthPercentCrossed`) |
| **Enables** | Closes the last Presentation-layer coverage gap group from `architecture-review-2026-07-27-v3.md` (TR-hud-001..007, TR-dash-008) — after this ADR, the only remaining architecture gap in the project is Combo/Tension Gauge (Feature layer, its own ADR). |
| **Blocks** | None. |
| **Ordering Note** | Independent of the still-unwritten Combo/Tension Gauge ADR — this ADR consumes the `TensionGauge`/`TensionGaugeMax` attributes already placed on `UMoonAttributeSet` by ADR-0001, not anything the future Combo/Tension Gauge ADR would define. If that future ADR changes attribute ownership or adds a decay-state enum, this ADR's W6 binding may need a follow-up amendment — flagged, not blocking. |

## Context

### Problem Statement

`combat-hud.md` (Approved) and `design/ux/combat-hud.md` (In Design) fully specify a 7-widget, read-only HUD mirroring five upstream systems, but no ADR governs the widget architecture, the exact binding contract per widget, or the update/coalescing policy. `architecture-review-2026-07-27-v3.md` scored this system at 0✅/0⚠️/7❌ (plus TR-dash-008, the eighth gap) — the last Presentation-layer block, and (with Combo/Tension Gauge) one of the only two remaining gaps in the entire project.

Unlike a from-scratch design, a substantial implementation already ships: `Moon/Source/Moon/UI/MoonCombatHUDWidget.{h,cpp}` — a single `UUserWidget` subclass that binds Health/Mana/DashCharges/TensionGauge GAS attribute-change delegates and the Overdrive Started/Ended signal, with a `NativeTick` used only for Tension's visual lerp. This ADR's job is the same shape as ADR-0007's for Dash and ADR-0009's for Player Movement: **ratify what's correct, fix what deviates from the GDD, and specify what's still missing** — not redesign from zero.

Auditing the shipped code against `combat-hud.md` found:
- **Correct and to be ratified**: plain `UUserWidget` base (not `UCommonActivatableWidget`) for a non-focusable HUD; event-driven push for Health/Mana/Dash; Tension's Tick-based lerp with `Charged` gated on the real (non-interpolated) value, exactly as Rule 5 requires.
- **Gap — W4 unimplemented**: no spell-cooldown query exists anywhere in the codebase. `spell-casting-base.md` names the requirement (`OnCooldown`/remaining time exposed for HUD) but ADR-0003 never specified the C++ query mechanism.
- **Deviation — W2 bypasses the GDD's stated interface**: the GDD binds W2 to `OnHealthPercentCrossed(LowHealthWarningThreshold)` (ADR-0008's interface), but the shipped code recomputes the threshold-crossing locally from the raw `Health`/`MaxHealth` delegate instead.
- **Gap — no `OnDeath` subscription**: Rule 9 (HUD reset on player Death) has zero wiring in `MoonCombatHUDWidget`.
- **Deviation — `BindToPlayer` pushes stale-or-initial values immediately, without waiting for a first real delegate**: found during this ADR's engine-specialist validation pass, missed by the initial audit. `combat-hud.md`'s Edge Cases table explicitly requires widgets stay hidden until their first real upstream delegate fires ("초기화 완료 이벤트 수신 후 일괄 표시... 미표시가 오표시보다 안전"), but shipped `BindToPlayer` immediately calls `OnHealthChanged`/`OnManaChanged`/`OnDashChargesChanged`/`OnOverdriveStateChanged` with whatever the AttributeSet already holds at bind time — see Decision 10.
- **Expected-incomplete, non-blocking**: TR-hud-007 (input-device glyph swap) and W7's execution-prompt half are unimplemented — both are GDD-acknowledged Open Questions (Core Extraction Execution doesn't exist yet; UE5.8 unified Input System needs `ue-umg-specialist` verification), not oversights this ADR is expected to close.

> **Note on audit completeness**: this ADR's own engine-specialist validation pass (`ue-umg-specialist`, 2026-07-27) found the `BindToPlayer` deviation above after the initial draft's audit had already framed its findings as exhaustive. Recorded here rather than silently folded in, since the ADR's stated methodology is "ratify what's correct, fix what deviates" — an audit that misses a real deviation undermines that claim if left uncorrected.

### Constraints

- HUD owns zero gameplay state — every value's truth source is an upstream system; HUD is a pure mirror (`combat-hud.md` Overview, Rule 10).
- All 7 widgets are non-focusable, excluded from pad focus-navigation (Rule 1) — no CommonUI activation/focus-stack participation.
- Update is event-driven by default; ticking is permitted only for active visual interpolation (Tension lerp) and time sweeps (Cooldown/Overdrive remaining-time), zero tick when idle (Rule 3).
- Per-widget rebuild/layout-invalidation capped at once per frame, last-value-wins, even under same-frame event floods (Rule 4).
- Gameplay-meaningful signals (Charged highlight, Overdrive flash) must trigger off real values, never interpolated display values (Rule 5).
- No new upstream interfaces may be invented by this ADR for interfaces that already exist (TR-hud-002) — but a genuinely missing interface (W4's cooldown query) may be added *additively* to its owning system, same pattern as ADR-0007 adding `IsInAttackRange`-style accessors to Enemy AI.

### Requirements

- TR-hud-001 — UMG + CommonUI base, all widgets non-focusable, excluded from pad focus graph.
- TR-hud-002 — Bind 7 widgets exclusively to interfaces already exposed upstream; no new upstream interfaces (except the one additive exception this ADR documents explicitly).
- TR-hud-003 — Event-driven update; tick only for active interpolation/time-sweep, zero tick when idle.
- TR-hud-004 — Per-frame update coalescing, at most one rebuild/invalidation per widget per frame, last-value-wins.
- TR-hud-005 — Gameplay-meaningful triggers off real values, never interpolated display values.
- TR-hud-006 — Mirror upstream reset/Death/Overdrive transitions without HUD making gameplay judgments.
- TR-hud-007 — Input-device detection and key-glyph swap (keyboard ↔ gamepad), affecting glyph only.
- TR-dash-008 — Expose dash charge stack count + cooldown gauge data to Combat HUD.

## Decision

1. **Base class stays plain `UUserWidget`; CommonUI is scoped narrowly to input-glyph swapping only (TR-hud-001).** The shipped `UMoonCombatHUDWidget : public UUserWidget` is correct and is ratified as-is — a 100%-read-only, non-focusable HUD has no use for `UCommonActivatableWidget`'s activation/focus-stack machinery, which exists for screens that *do* participate in navigation (menus, inventory). `combat-hud.md`'s "UMG + CommonUI 기반" phrase is satisfied narrowly: CommonUI is used only for its input-method-detection utility (Decision 7), never as the widget's inheritance base. Rejects Alternative 1 (full `UCommonActivatableWidget` root).

2. **W4 cooldown query — additive accessor on Spell Casting, triggered by the per-element cooldown GameplayTag ADR-0003 already grants; duration exposed alongside remaining time.** Add three thin accessors to `UMoonAbilitySystemComponent`: `GetElementCooldownTag(EMoonSpellElement Element) const -> FGameplayTag` (the existing per-element cooldown tag spell-casting-base.md Core Rule 6 already mandates — "GAS Cooldown GameplayEffect + 태그" — exposed, not newly invented), `GetElementCooldownRemaining(Element) const -> float` (seconds, via the engine-standard `GetActiveEffectsTimeRemaining(FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(...))` pattern, queried against the tag above — the same idiom Epic's own GAS sample content uses for cooldown-percent display), and `GetElementCooldownDuration(Element) const -> float` (the element's already-defined cooldown-length constant — Blackhole 6.0/Fire 2.0/Lightning 2.5 — read from the same `UMoonSpellData` each ability already sources it from, not a new value). `FractionRemaining = Remaining / Duration`, computable entirely from these three.
   The trigger is the tag itself: `Combat HUD` calls `BoundASC->RegisterGameplayTagEvent(GetElementCooldownTag(Element), EGameplayTagEventType::NewOrRemoved)` per element in `BindToPlayer` — a stock, version-stable `UAbilitySystemComponent` API, not a new delegate this ADR invents. The same callback fires on both cooldown-grant (start — read remaining+duration, compute fraction, arm the Tick-sweep) and cooldown-tag-removal (end — fraction snaps to 0, sweep disarms), so no separate "end" event is needed.
   This is the same additive-extension pattern already registered in `architecture.yaml` as `enemy_telegraph_query` (ADR-0007 adding a query onto ADR-0006's system) — these three accessors are registered against **ADR-0003 as owner**, this ADR as the adding party. (Which GAS-domain mechanism to use for the *tag-granting* side — already decided by ADR-0003's existing Cooldown GE — was cross-checked with `ue-gas-specialist`-relevant code (`MoonGameplayAbility_Spell.h`/`_Fire.cpp`/`_Blackhole.cpp`) during this ADR's engine-specialist validation pass; if Spell Casting's cooldown-tag scheme ever changes, only these three accessors' internals need updating, not this ADR's Decision text.)

3. **W2 keeps its shipped local recomputation; `OnHealthPercentCrossed` is reserved for Boss Phase.** `HandleHealthChanged`'s existing `(Data.NewValue / MaxHealth) <= LowHealthWarningThreshold` check is ratified, not replaced. Rationale: `combat-hud.md`'s own cross-constraint explicitly requires that changing `LowHealthWarningThreshold` alone (a single HUD-owned Tuning Knob) must be sufficient — no companion edit elsewhere. ADR-0008 implements `HealthPercentThresholds` as a fixed `EditDefaultsOnly TArray<float>` **on `UMoonAttributeSet` itself** (default `{0.5, 0.25}`), intended for Health/Damage Core's own designer-set threshold list (its stated future consumer is Boss Phase, which needs a shared, HDC-owned threshold list for phase transitions). Binding W2 to that interface would mean every future change to the HUD's `LowHealthWarningThreshold` also requires editing HDC's `HealthPercentThresholds` array — violating the GDD's own single-knob cross-constraint. The local-recompute approach reads an interface that already exists and is already bound for W1 (the `Health`/`MaxHealth` attribute-change delegate) — it does not invent a new upstream interface, it reuses one already claimed by W1, satisfying TR-hud-002 without the coupling cost.

4. **Update policy — event-driven push for Health/Mana/Dash/Cooldown-transitions, Tick-sourced-read for Tension/Cooldown-sweep/Overdrive-sweep (TR-hud-003).** Confirms the shipped pattern for Health/Mana/Dash (delegate fires → `BlueprintImplementableEvent` called directly, no polling) and extends the same Tick-conditional-work rule already used for Tension's lerp to the two new time-sweep cases: Cooldown overlay fraction (only while ≥1 element on cooldown, armed/disarmed by Decision 2's tag event) and Overdrive remaining-time countdown (already implemented — only while `bOverdriveActive`). Precisely: `NativeTick()` itself still runs every frame (it is not conditionally registered/unregistered — that complexity buys nothing at one branch's cost), but the substantive work inside each of its three branches only executes while that branch's driving condition holds.

5. **Per-frame coalescing relies on UMG's own invalidation, not a new dirty-flag layer, for non-interpolated widgets (TR-hud-004) — corrected rationale.** Slate's invalidation tracking is a per-widget *flag*, not a queue: marking a widget dirty N times in one frame costs the same as marking it once, and this holds regardless of whether the mark is Paint-only or Layout-triggering — so no separate dirty-flag/coalescing layer is needed for Health/Mana/Dash/Cooldown, full stop. (An earlier draft of this Decision claimed the coalescing was safe specifically *because* these are paint-only writes — that claim is not reliably true: `SetPercent` on `UProgressBar` is paint-only, but `SetText` on an auto-sizing `TextBlock` can trigger layout invalidation when the string's rendered length changes, e.g. "75/100" → "100/100". The corrected reasoning above does not depend on that distinction, so it holds either way.) Only Tension needed an explicit Tick-based single-read-per-frame pattern, because its value additionally drives a continuous lerp calculation (not just a display write) — that pattern stays exactly where it already is, it is not generalized to the other widgets. **AC10's performance-counter verification must include at least one text-bearing widget (Health or Mana), not only Tension** — this is a scope widening from the original draft, not a new requirement. Migration Plan adds a cheap mitigation: use fixed-width, non-auto-wrap text styles for HUD numeric readouts, which guarantees paint-only invalidation for those widgets and sidesteps the layout-invalidation edge case entirely regardless of what AC10 measures.

6. **`OnDeath` subscription added — mirrors Rule 9's reset contract.** `BindToPlayer` gets a new `Character->OnDeath.AddDynamic(this, &UMoonCombatHUDWidget::HandleDeath)` (the same `FOnMoonDeath` delegate ADR-0008 already defines and ADR-0002/0004/0006 already consume — no new interface). `HandleDeath()`: clears the execution-prompt/world-marker state (`OnExecutionPromptChanged(false)`), forces `OnOverdriveStateChanged(false)` if Overdrive was active (idempotent with the existing `OnOverdriveEnded(PlayerDeath)` path — both may fire, second call is a no-op mirror), and resets `DisplayedTension`/`TargetTension` to `0` (`OnTensionStateChanged` re-fires with zeroed values). This closes the one Rule 9 gap the code audit found.

7. **TR-hud-007 — CommonUI's input-method-changed signal drives glyph swap only; specifics deferred to implementation-time verification.** Subscribe to `UCommonInputSubsystem`'s current-input-type-changed notification (exact delegate name/signature to be confirmed against real 5.8 CommonUI headers by `ue-umg-specialist` before implementation — this ADR does not guess the signature, consistent with not fabricating post-cutoff API specifics). On change, swap only the F-key/gamepad-button glyph texture on the execution-prompt widget — no other HUD behavior differs by input device (`combat-hud.md` Rule 7 / UI Requirements, `design/ux/combat-hud.md` Platform & Input Variants). This decision fixes the *mechanism* (CommonUI input-method signal → glyph-only swap); the *exact API* remains a Verification Required item, matching the GDD's own Open Question ownership (`gameplay-programmer`, target: implementation time).

8. **TR-dash-008 — already satisfied, closed formally.** W5's existing binding to `UMoonAttributeSet::GetDashChargesAttribute()`'s change delegate (shipped, `HandleDashChargesChanged`) already exposes dash charge count + recharge fraction to the HUD exactly as `dash-evasion.md`'s Dependencies table requires. No code change; this ADR's role is the same as ADR-0009's Decision 7 for TR-mov-010 — formal closure of an already-correct implementation.

9. **W7's execution-prompt half (world markers, nearest-target screen prompt) stays an explicitly assumed interface — not this ADR's to close.** `combat-hud.md` Rule 6 already frames this as provisional pending the not-yet-designed Core Extraction Execution GDD; this ADR does not invent the proximity-check or multi-target-marker logic. The existing `OnExecutionPromptChanged(bool)` stub is left as the minimal placeholder the GDD itself sanctions, same treatment as ADR-0009 leaving `SetMovementLocked` reserved-but-uncallable for the not-yet-designed Status Effect system.

10. **`BindToPlayer`'s immediate bind-time push is fixed, not ratified — gated on a `bHasReceivedFirstUpdate` flag per widget group, to honor the GDD's explicit Edge Case.** Rather than asserting (without proof) that the AttributeSet is always fully initialized before `BindToPlayer` runs — an ordering guarantee this ADR cannot verify from the widget class alone, since it depends on `PlayerController`/HUD-creation timing relative to `AMoonCharacterBase::PossessedBy`/ability-actor-info init — `MoonCombatHUDWidget` gets a small per-group `bool` gate (`bHealthInitialized`, `bManaInitialized`, `bDashInitialized`; Tension/Overdrive/Cooldown already default to a correct hidden/inactive state and don't need one). Each relevant `Handle*Changed` sets its gate `true` on first invocation and only then calls the corresponding `BlueprintImplementableEvent`; the widget's initial visibility (set in `BindToPlayer`, before any delegate has fired) is `Collapsed` for the gated groups, matching the GDD's "미표시가 오표시보다 안전" requirement exactly, and without needing to prove an ordering guarantee that may not always hold (e.g. hot-reload, PIE quirks, or a future change to widget-creation timing). This replaces `BindToPlayer`'s current unconditional initial push.

### Architecture Diagram

```
UMoonCombatHUDWidget : UUserWidget   (non-focusable, excluded from pad focus graph — TR-hud-001)
 ├─ BindToPlayer(APawn*)
 │    ├─ GAS attribute delegates: Health, Mana, DashCharges, TensionGauge   [existing]
 │    ├─ Character->OnOverdriveStarted / OnOverdriveEnded                   [existing]
 │    ├─ Character->OnDeath                                                 [NEW — Decision 6, TR-hud-006]
 │    └─ BoundASC->RegisterGameplayTagEvent(CooldownTag, NewOrRemoved) ×3   [NEW — Decision 2, TR-hud-002/W4, stock ASC API]
 │
 ├─ NativeTick()  — active only while a driving condition holds (TR-hud-003)
 │    ├─ Tension lerp (existing, Rule 3 exception #1)
 │    ├─ Cooldown overlay sweep (NEW — only while ≥1 element on cooldown, Rule 3 exception #2)
 │    └─ Overdrive remaining-time sweep (existing, only while bOverdriveActive)
 │
 ├─ HandleHealthChanged()      — local LowHealthWarningThreshold recompute (Decision 3, W1+W2, one knob)
 ├─ HandleManaChanged()        — direct push (W3)
 ├─ HandleDashChargesChanged() — direct push (W5, TR-dash-008 — already closed)
 ├─ HandleElementCooldownTagChanged() — direct push + arms/disarms Tick sweep (NEW — W4)
 ├─ HandleTensionChanged()     — sets TargetTension only, Tick reads it (W6)
 ├─ HandleOverdriveStarted/Ended() — existing (W7 indicator half)
 └─ HandleDeath()              — NEW: clear execution prompt, force Overdrive off, zero Tension display (Rule 9)

UMoonAbilitySystemComponent (Spell Casting, ADR-0003-owned, additively extended here)
 ├─ GetElementCooldownTag(Element): FGameplayTag                        [NEW — Decision 2, exposes existing tag]
 ├─ GetElementCooldownRemaining(Element): float                        [NEW — Decision 2]
 └─ GetElementCooldownDuration(Element): float                         [NEW — Decision 2]
      (trigger = stock ASC::RegisterGameplayTagEvent(CooldownTag, NewOrRemoved) — no new delegate)

CommonUI input-method-changed signal ──► glyph swap only (execution-prompt widget)   [Decision 7, Verification Required]
```

### Key Interfaces

```
UMoonCombatHUDWidget : public UUserWidget   // unchanged base — Decision 1
    void BindToPlayer(APawn*)                // existing, extended: + OnDeath, + per-element RegisterGameplayTagEvent (Decision 2), + init-gate (Decision 10)

UMoonAbilitySystemComponent   // Spell Casting (ADR-0003-owned), additive
    FGameplayTag GetElementCooldownTag(EMoonSpellElement Element) const       // NEW — exposes the tag ADR-0003 already grants
    float GetElementCooldownRemaining(EMoonSpellElement Element) const       // NEW
    float GetElementCooldownDuration(EMoonSpellElement Element) const        // NEW

UMoonCombatHUDWidget (new protected members)
    void HandleDeath()                                        // NEW — Decision 6
    void HandleElementCooldownTagChanged(EMoonSpellElement, int32 NewCount)  // NEW — Decision 2, bound via RegisterGameplayTagEvent
    UFUNCTION(BlueprintImplementableEvent) void OnCooldownStateChanged(EMoonSpellElement Element, bool bOnCooldown, float FractionRemaining)  // NEW — W4
    bool bHealthInitialized, bManaInitialized, bDashInitialized = false      // NEW — Decision 10
```

## Alternatives Considered

### Alternative 1 (TR-hud-001): Root widget as `UCommonActivatableWidget`

- **Description**: Make the HUD's top-level widget a `UCommonActivatableWidget`, participating in CommonUI's screen/activation stack like a menu would.
- **Pros**: Uniform CommonUI usage across all UI in the project; some built-in input-context handling comes for free.
- **Cons**: Activation/focus-stack machinery exists to solve a problem this HUD explicitly doesn't have (Rule 1: zero focusable widgets, explicitly excluded from pad navigation). Adopting it would require actively suppressing behavior CommonUI turns on by default, for no benefit.
- **Rejection Reason**: Solves a non-problem; the shipped plain-`UUserWidget` base already satisfies every stated requirement with less surface area.

### Alternative 2 (W4): HUD queries GAS's cooldown tag/effect directly, no Spell Casting accessor

- **Description**: Skip adding accessors to `UMoonAbilitySystemComponent`; have the HUD hardcode the three elements' cooldown `GameplayTag`s itself and call `RegisterGameplayTagEvent`/`GetActiveEffectsTimeRemaining()` directly against them.
- **Pros**: Zero new lines in Spell Casting's code.
- **Cons**: GAS's raw tag/effect-query API is an engine primitive, not a Spell Casting interface — the HUD would need to independently know each element's exact cooldown tag name and duration constant, duplicating knowledge ADR-0003 already owns. If Spell Casting's cooldown-tag scheme ever changes, every direct-query call site breaks silently instead of one accessor's internals changing.
- **Rejection Reason**: TR-hud-002's "no new upstream interfaces" principle is about not inventing HUD-side knowledge of upstream internals — routing through three thin owned accessors keeps that boundary intact; hardcoding tag names in the HUD erodes it. (This ADR's `ue-umg-specialist` validation pass confirmed `GetActiveEffectsTimeRemaining()` against the tag is the correct idiomatic GAS call for this purpose — the only fix needed was exposing it through Spell Casting rather than which underlying GAS API to use.)

### Alternative 3 (W2): Bind directly to `OnHealthPercentCrossed`, add `LowHealthWarningThreshold` to `HealthPercentThresholds` at `BindToPlayer` time

- **Description**: Have the HUD push its own threshold into HDC's `HealthPercentThresholds` array at bind time, then subscribe to `OnHealthPercentCrossed` and filter for that specific value.
- **Pros**: Uses the GDD's literally-named interface; one shared crossing-detection code path for all consumers (HUD today, Boss Phase later).
- **Cons**: Requires HDC's array to be runtime-mutable (currently `EditDefaultsOnly`, i.e., designer-set, not append-at-runtime) — a scope change to ADR-0008 this ADR would have to make unilaterally. Even if made mutable, two independent Tuning Knobs (`LowHealthWarningThreshold` on Combat HUD, an appended runtime entry on HDC) now describe the same number through two ADRs — exactly the cross-system coupling the GDD's own cross-constraint note warns against.
- **Rejection Reason**: Higher coordination cost for zero behavioral gain over the already-correct local computation; would require reopening an Accepted ADR (0008) for a Presentation-layer convenience.

## Consequences

### Positive

- Closes the last 8 requirement gaps outside Combo/Tension Gauge — TR-hud-001 through 007 and TR-dash-008 — leaving exactly one system (Combo/Tension Gauge) short of full project coverage.
- Ratifies a substantial amount of already-correct shipped code rather than requiring a rewrite, same low-risk shape as ADR-0007's Dash ratification.
- The additive Spell Casting accessor (Decision 2) follows an already-proven pattern (`enemy_telegraph_query`) rather than inventing a new cross-ADR-extension convention.
- W2's local-recompute ratification directly honors an explicit GDD cross-constraint that a naive "just use the named interface" reading would have silently violated.

### Negative

- `UMoonAbilitySystemComponent` gains three new public query methods whose correctness depends on Spell Casting's cooldown GE/tag scheme staying stable — if ADR-0003's cooldown-tag scheme changes, these accessors must change with it (documented dependency, not a hidden one).
- `HealthPercentThresholds` (ADR-0008) and `LowHealthWarningThreshold` (this ADR) both encode a 0.25 default independently — currently harmless (values happen to match) but a future change to either without checking the other could silently desync the "expected" 25% warning point from HDC's own default threshold list, even though the two no longer share a code path. Worth a comment at both definition sites (Migration Plan item 6).
- Cooldown Tick-sweep (Decision 4) adds one more conditionally-active tick branch to `MoonCombatHUDWidget`'s `NativeTick()`, alongside Tension's lerp and Overdrive's countdown — three independent "only-while-active" branches now coexist in one function, worth watching for creeping complexity as more widgets are added.
- Three new per-widget-group init-gates (`bHealthInitialized`/`bManaInitialized`/`bDashInitialized`, Decision 10) are small but permanent bookkeeping state that must each be set exactly once — a future refactor that adds a fourth gated widget must remember the pattern rather than reintroducing the un-gated bind-time push this Decision fixes.

### Risks

- **AC10's coalescing claim now rests on Slate's flag-not-queue invalidation model, which is standard and stable UMG/Slate behavior, but has not been profiled on this project's actual widget tree.** Mitigation: `performance-analyst`'s already-flagged Open Question (HUD frame-budget measurement under full Overdrive load) must explicitly include a same-frame-event-flood test for at least one text-bearing widget (Health or Mana), not only Tension, per Decision 5's corrected scope — this is a widened test, not a new one.
- **CommonUI input-method-changed API (Decision 7) is unverified against real 5.8 headers — and the verification scope is broader than just the delegate's name/signature.** Given `breaking-changes.md`'s "removes duplicate data asset requirements" phrasing, the pre-5.8 CommonUI glyph-data-asset pattern itself may be one of the things removed, meaning the glyph asset's source of truth may have moved into Enhanced Input directly. Mitigation: `ue-umg-specialist` must confirm both the input-method-changed API *and* where the glyph asset lives in 5.8 before implementation, per this ADR's own Verification Required field and Migration Plan item 5 — already the GDD's stated plan, widened here rather than newly introduced.
- **The two-independent-defaults risk (0.25 in two places, Negative above) could silently diverge.** Mitigation: implementation should add a code comment on both `HealthPercentThresholds`'s default and `LowHealthWarningThreshold`'s default cross-referencing each other, so a future balance pass notices the pairing.
- **`HandleHealthChanged`/`HandleManaChanged` divide by `MaxHealth`/`MaxMana` with no zero-guard**, unlike `HandleTensionChanged`'s Tick path (which checks `MaxTension > 0.0f`). Not currently reachable (both Max attributes are always positive by GDD design) but inconsistent with the guarded sibling path. Mitigation: Migration Plan item 4 adds the guard.

## GDD Requirements Addressed

| GDD Document | Requirement | How This ADR Satisfies It |
|---|---|---|
| combat-hud.md | TR-hud-001 (UMG+CommonUI base, non-focusable) | Decision 1 — plain `UUserWidget` ratified, CommonUI scoped to glyph-swap only |
| combat-hud.md | TR-hud-002 (bind to existing interfaces only) | Decisions 2/3 — one documented additive exception (W4 cooldown query on Spell Casting), all other bindings reuse existing interfaces |
| combat-hud.md | TR-hud-003 (event-driven, tick only for active interpolation/sweep) | Decision 4 — Tension lerp (existing) + Cooldown/Overdrive sweeps, all conditionally active only |
| combat-hud.md | TR-hud-004 (per-frame coalescing) | Decision 5 — UMG paint-invalidation for non-interpolated widgets, Tick-single-read for Tension |
| combat-hud.md | TR-hud-005 (trigger off real values, not interpolated) | Ratifies existing `bIsCharged` computed off `TargetTension`, not `DisplayedTension` |
| combat-hud.md | TR-hud-006 (mirror reset/Death/Overdrive without HUD judgment) | Decision 6 — new `OnDeath` subscription closes the Death-reset gap; Overdrive already correctly wired |
| combat-hud.md | TR-hud-007 (input-device glyph swap) | Decision 7 — CommonUI input-method signal, glyph-only scope, exact API deferred to verified implementation |
| dash-evasion.md | TR-dash-008 (dash charge/cooldown → HUD) | Decision 8 — already satisfied by shipped `DashChargesAttribute` binding, formally closed |

## Performance Implications

- **CPU**: Two additional conditionally-active tick-branches (Cooldown sweep, already-existing Overdrive sweep) join Tension's lerp inside one still-always-running `NativeTick()` — each branch's substantive work only executes while its own state is active. New `GetElementCooldownRemaining()`/`GetElementCooldownDuration()` queries are O(1) against GAS's existing active-effect tracking, called on `RegisterGameplayTagEvent` firings (cooldown start/end) plus once per active tick-sweep frame. `BlueprintImplementableEvent` handlers for all three Tick-driven branches must stay simple property writes — a BP implementer wiring a full widget-animation restart into one of them is where actual per-frame frame-budget risk lives, not the branch dispatch itself.
- **Memory**: No new persistent allocations — cooldown state is read from GAS's existing active-effect tracking, not duplicated on the HUD.
- **Load Time**: None.
- **Network**: Out of scope, consistent with all prior ADRs (no multiplayer requirement in any GDD to date).

## Migration Plan

1. Add `UMoonAbilitySystemComponent::GetElementCooldownTag(Element)` / `GetElementCooldownRemaining(Element)` / `GetElementCooldownDuration(Element)`, the latter two backed by `GetActiveEffectsTimeRemaining(FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(...))` and the existing per-element cooldown-duration constant respectively (Decision 2).
2. Add `HandleElementCooldownTagChanged` + `OnCooldownStateChanged` (`BlueprintImplementableEvent`, now carrying `bOnCooldown` + `FractionRemaining`) to `UMoonCombatHUDWidget`; call `BoundASC->RegisterGameplayTagEvent(GetElementCooldownTag(Element), EGameplayTagEventType::NewOrRemoved)` per element in `BindToPlayer`; arm/disarm the Cooldown Tick-sweep from the same callback (Decision 2/4).
3. Add `Character->OnDeath` subscription in `BindToPlayer`; implement `HandleDeath()` per Decision 6 (clear execution prompt, force Overdrive-off idempotently, zero Tension display).
4. Leave W2's existing local-recompute logic untouched (Decision 3 — no code change, ratification only); add a zero-guard to `HandleHealthChanged`/`HandleManaChanged`'s division (currently only `HandleTensionChanged`'s Tick path guards against `MaxTension <= 0`).
5. Implement TR-hud-007's glyph swap once `ue-umg-specialist` confirms, against real 5.8 CommonUI headers, both the input-method-changed API **and** where the glyph asset itself now lives post-unification (the ADR does not assume the pre-5.8 CommonUI glyph-data-asset pattern survived the "removes duplicate data asset requirements" change verbatim) — do not implement against a guessed signature or a guessed data-asset location.
6. Add the cross-reference comment pairing `HealthPercentThresholds`'s and `LowHealthWarningThreshold`'s 0.25 defaults (Negative/Risks).
7. Add `bHealthInitialized`/`bManaInitialized`/`bDashInitialized` gates per Decision 10; set initial `Visibility` to `Collapsed` (not `Hidden`, per this project's own widget-visibility convention) for the gated groups until each fires once.
8. Use fixed-width, non-auto-wrap text styles for HUD numeric readouts (Health/Mana) — guarantees paint-only invalidation for those widgets independent of what AC10's measurement finds (Decision 5).
9. `HUDUpdateMode` (GDD Tuning Knob: EventDriven/TickPolling, debug-only) has no C++ representation in this ADR — acceptable as a Blueprint/editor-only debug toggle for AC10 verification, not a dropped requirement.
10. Re-run `/architecture-review` in a fresh session to confirm TR-hud-001..007 and TR-dash-008 flip to ✅ and the project's only remaining gap is Combo/Tension Gauge.

**Rollback plan**: All decisions are additive (new methods, new subscriptions, new conditionally-active tick branches) — nothing shipped is removed or behaviorally changed except W2 (kept as-is) and `BindToPlayer`'s initial-push behavior (Decision 10, gated instead of unconditional). Any single Migration Plan item can be reverted independently without affecting the others.

## Validation Criteria

- `combat-hud.md`'s own Acceptance Criteria 1–12, in particular AC2 (cooldown overlay fraction display, now implementable via Decision 2), AC10 (same-frame event-flood coalescing — performance-counter verification must cover **at least one text-bearing widget, e.g. Health or Mana, not only Tension**, per Decision 5's corrected rationale), AC11 (Death-frame reset, now implementable via Decision 6), and AC12 (input-device glyph swap, gated on Decision 7's implementation-time verification). Also verify the GDD's "hidden until first real delegate" Edge Case (Decision 10) directly — a widget must render nothing, not a zero value, before its first bind-time update.
- Re-run `/architecture-review` — confirms the Combat HUD + Dash-HUD-surface coverage rows move to full ✅ and the project's remaining-gap count drops to Combo/Tension Gauge only.

## Related Decisions

- ADR-0001 (Player Movement and GAS Core) — origin of the shared `TensionGauge`/`DashCharges` GAS attributes this ADR's W5/W6 bind to.
- ADR-0003 (Spell Casting GAS Implementation) — remains the owning ADR for the cooldown query accessor added additively by Decision 2 (same relationship as ADR-0006/0007's `enemy_telegraph_query`).
- ADR-0004 (Luna Overdrive Fixed Window) — `OnOverdriveStarted`/`OnOverdriveEnded` signal this ADR's W7 indicator half already consumes.
- ADR-0007 (Dash/Evasion Just-Dodge) — `DashChargesAttribute` this ADR's W5 already consumes (TR-dash-008); also the precedent for reverse-documenting already-shipped code in an ADR.
- ADR-0008 (Health/Damage Core Death Event Contract) — `OnDeath` (newly consumed here, Decision 6) and `OnHealthPercentCrossed`/`HealthPercentThresholds` (explicitly not bound here, Decision 3/Alternative 3 — reserved for Boss Phase).
- `design/gdd/combat-hud.md` — primary GDD source (Core Rules 1–10, Formulas, Acceptance Criteria).
- `design/ux/combat-hud.md` — layout/visual spec, not architecturally binding but confirms no additional widgets beyond the GDD's 7.
- `docs/architecture/architecture-review-2026-07-27-v3.md` — the coverage gap this ADR resolves.
