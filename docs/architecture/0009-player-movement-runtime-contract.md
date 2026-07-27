# ADR-0009: Player Movement — Runtime Contract

## Status
Accepted

## Date
2026-07-27

## Engine Compatibility

| Field | Value |
|-------|-------|
| **Engine** | Unreal Engine 5.8 |
| **Domain** | Movement (`CharacterMovementComponent`) / Animation (hitstop presentation layer) / Input |
| **Knowledge Risk** | HIGH — `player-movement.md` itself already flags `SetMovementModeWithCustomMode()` deprecation as unverified against real 5.8 headers (only doc-section-level, not signature-level, per the GDD's own Open Questions). This ADR resolves that specific risk by architectural avoidance (Decision 2) rather than by picking an API — see below. |
| **References Consulted** | `docs/engine-reference/unreal/VERSION.md`, `breaking-changes.md`, `deprecated-apis.md`, `modules/physics.md` (Character Movement section), `modules/input.md` (Enhanced Input `Triggered` delegate) |
| **Post-Cutoff APIs Used** | None. Decision 2 deliberately avoids `SetMovementModeWithCustomMode()`/custom movement modes entirely — Ascending/Falling stay derived from `Velocity.Z` sign on top of the single native `MOVE_Falling` mode, exactly as `player-movement.md`'s own "구현 노트" already mandates. |
| **Verification Required** | `TRACE_CPUPROFILER_EVENT_SCOPE` channel behavior and Enhanced Input's `Triggered` delegate firing point (Decision 6) should be confirmed against 5.8 Enhanced Input internals before the Insights-based latency AC is trusted as accurate; `BrakingDecelerationFalling`/`FallingLateralFriction` native interaction (already an Open Question in the GDD, not this ADR's to resolve) still needs a gameplay-programmer engine spike before Tuning Knobs are finalized. |

## ADR Dependencies

| Field | Value |
|-------|-------|
| **Depends On** | ADR-0001 (Accepted) — extends `AMoonCharacterBase`'s existing CMC setup and jump/hitstop code; does not replace it. |
| **Enables** | Closes the last Foundation-layer gap group in `architecture-review-2026-07-27.md` — after this ADR, the only remaining coverage gaps are Combo/Tension Gauge and Combat HUD (Feature/Presentation layer, not Foundation). Does not gate any other ADR's Acceptance (ADR-0004/0006/0007 are gated on ADR-0008, not this one). |
| **Blocks** | None. |
| **Ordering Note** | None — independent of the Health/Damage Core death contract (ADR-0008); can be Accepted on its own schedule. |

## Context

### Problem Statement

`architecture-review-2026-07-27.md` scored Player Movement at 1✅/2⚠️/7❌ of 10 requirements — the largest single coverage gap in the project despite ADR-0001 being titled "Player Movement and GAS Core." ADR-0001's actual Decision text covers exactly two of ten TRs (TR-mov-001, TR-mov-004); the other seven (`TR-mov-002/003/006/007/008/009/010`) were never picked up by any ADR even though `player-movement.md` (Approved, extensively detailed — Core Rules 1–11, hard Joint AirTime Bound, delta-time-based buffer/coyote windows) already specifies them unambiguously. Separately, the same review found **V-1**: shipped code (`MoonCharacterBase.cpp:282`, `MoonGameplayAbility_Dash.cpp:211`) uses `CustomTimeDilation` for hitstop, which Core Rule 9 explicitly and unconditionally forbids ("어떤 형태의 Time Dilation도 사용하지 않음") — a real design disagreement, not an oversight (the code has a deliberate comment explaining the choice), and it must be resolved explicitly rather than left silent.

### Constraints

- Gameplay tick (movement input processing, cast-state processing) must run at 100% normal rate at all times — hitstop, execution presentation, and any future "impact freeze" moment are presentation-only (Core Rules 8/9/11 all state the same invariant three separate times).
- Movement module must have zero compile-time reference to Spell Casting types/state (Core Rule 7's "정적 검증 가능한 아키텍처 불변식").
- `MovementLocked` may only ever be set by the (not-yet-designed) Status Effect system — Spell Casting must never gain write access, even accidentally.
- Ascending/Falling are not native UE movement modes — both map to `MOVE_Falling`; `Velocity.Z` sign is the **sole** source of truth, sampled only after CMC's own tick has run each frame (stale-value risk explicitly called out in the GDD).
- Jump Input Buffer (150ms) and Coyote Time (150ms) are delta-time based (not fixed tick counts), owned by the `Character` class, with an inclusive `<=` boundary operator.
- All locomotion animations must be non-root-motion; none may block `AddMovementInput` (no montage-based input lock, ever).

### Requirements

- TR-mov-002 — Movement module compile-independent of Spell Casting.
- TR-mov-003 — Airborne sub-states derived from `Velocity.Z` sign post-CMC-tick; external Z-impulse re-entry (Ascending↔Falling) works with no special-casing.
- TR-mov-006 — `MovementLocked` write access restricted to Status Effect system only.
- TR-mov-007 — Jump input buffer 150ms / coyote time 150ms, delta-time based, `Character`-owned.
- TR-mov-008 — Hitstop/execution presentation uses no Time Dilation of any form (**V-1**).
- TR-mov-009 — Input→velocity latency budget p95 ≤33ms, time-to-95%-speed ≤50ms, Insights trace scopes.
- TR-mov-010 — All locomotion anims non-root-motion (`bEnableRootMotion==false`).

## Decision

1. **TR-mov-002 — single-module + CI contract, not a physical module split.** This project has one C++ module (`Moon`, per ADR-0001) — there is no separate `Movement` UBT module to declare a `Build.cs` dependency against, and introducing one purely to satisfy this TR would be new architectural surface the project doesn't otherwise need at MVP scope. TR-mov-002 is satisfied by the static-analysis contract `player-movement.md`'s own Acceptance Criteria already specifies: CI grep for zero references to Spell Casting types/state in Movement-owning files, **and** (the GDD's own "보강" clause) confirming no `PublicDependencyModuleNames`/`PrivateDependencyModuleNames` entry exists for a hypothetical SpellCasting module — trivially true today since none exists. If the project ever splits into multiple UBT modules, this decision must be revisited; not before.

2. **TR-mov-003 — Ascending/Falling are a derived value, never a stored state machine.** `AMoonCharacterBase::Tick()`, after `Super::Tick()` (i.e., after CMC's own movement tick has run this frame — required ordering per the GDD's explicit stale-value warning), reads `GetCharacterMovement()->Velocity.Z` and derives `EMoonAirborneSubState` (`Ascending` if `>0`, `Falling` if `<=0`) purely for consumers that need to distinguish them (VFX, Combat HUD indicators, etc.) — there is no transition table, no custom movement mode, and no `SetMovementModeWithCustomMode()` call anywhere in this path. External Z-impulse re-entry (Dash/Launch pushing `Velocity.Z` positive while Falling) requires no special-casing: the same one-line sign check on the next tick naturally reads `Ascending` again. Ceiling-bump deceleration to `Falling` (an engine-side `HandleImpact` effect) is likewise not special-cased — it is the sign rule doing exactly what it's defined to do, matching the GDD's explicit note that this is intended, not a bug.

3. **TR-mov-006 — `MovementLocked` is a private bool with a single reserved setter, currently uncallable.** `AMoonCharacterBase` gets a private `bool bMovementLocked` and a `private` `SetMovementLocked(bool)` — not `BlueprintCallable`, not `public`, and not called from anywhere in the current codebase, since the Status Effect system (its sole authorized caller per the GDD) is not yet designed. This closes the TR as an access-control **reservation**: the field exists, is checked by `AddMovementInput`-gating logic (`if (bMovementLocked) return;` at the top of `Move()`), and has exactly one write path in the class. When Status Effect is designed, its ADR must expose the setter (e.g. via a narrow friend/interface grant) rather than this ADR speculatively building an access-control mechanism for a consumer that doesn't exist yet.

4. **TR-mov-007 — confirm/implement both timers as `Character`-owned float accumulators.** `JumpInputBufferTimer` and `CoyoteTimeTimer` (both `float`, seconds) count down via `-= DeltaTime` in `Tick()`, starting at `0.150f` on their respective trigger (post-jump-input for buffer, post-ground-exit-without-jump for coyote) — never a fixed frame count. Boundary check is `ElapsedTime <= 0.150f` (inclusive), matching the GDD's explicit boundary-operator note (149ms/150ms pass, 151ms fails). Both timers live on `AMoonCharacterBase`, alongside the existing jump state machine (same class, same pattern already established for `Landed()`).

5. **TR-mov-008 / V-1 — remove `CustomTimeDilation` entirely; replace with a capture-and-blend presentation freeze.** `MoonCharacterBase.cpp:282`/`MoonGameplayAbility_Dash.cpp:211`'s `CustomTimeDilation` calls are deleted. Hitstop becomes a pure presentation technique, per Core Rule 9's mandated pattern:
   - On hitstop trigger: capture `FreezeStartMeshTransform = GetMesh()->GetComponentTransform()` (and, if the camera is also meant to freeze, the camera's transform at that instant).
   - For the hitstop duration (40ms / `HitStopDuration`), the mesh's animation position is held (`bPauseAnims`-equivalent) while the mesh's world transform is forced back to `FreezeStartMeshTransform` every tick — even though the Capsule (the actual gameplay position, driven by CMC at 100% normal rate) continues moving underneath it. This is what makes the freeze visual-only: gameplay position is never touched.
   - On unfreeze: blend the mesh (and camera, if offset) from the captured transform back to the *actual* current transform over 1–2 frames (fast `InterpTo`, not an instant snap) — this is the specific requirement Core Rule 9 added to prevent the "camera/character snaps on unfreeze" artifact.
   - AnimNotify slip (landing SFX scheduled inside the freeze window) is an advisory follow-up, not blocking — GDD marks it non-blocking; `sound-designer`/`technical-artist` confirm at implementation time whether notifies need pre-trigger or a freeze-independent timeline.
   - This is a code-fix decision, not a GDD revision — Core Rules 8, 9, and 11 all converge on the same "judgment and presentation are different pipelines" principle across three separate features (dash-cancel input, hitstop, execution presentation); revising Rule 9 alone to permit `CustomTimeDilation` would make it the sole exception to a principle the rest of this GDD (and `health-damage-core.md` Rule 5) treats as foundational.

6. **TR-mov-009 — instrument now, not later.** Add a `TRACE_CPUPROFILER_EVENT_SCOPE` channel `MovementInputTrace` at exactly the two points the GDD specifies: (a) start of the Enhanced Input `Triggered` delegate callback for `IA_Move` (excludes engine input-pipeline jitter from the measured window, per the GDD's explicit definition), (b) the first frame `CharacterMovementComponent::TickComponent` completes with `Velocity` updated toward the new target. These scopes are cheap (always-on in Development builds) and are what makes the Insights-based p95-latency and time-to-95%-speed Feel ACs measurable at all — without them, those ACs are unimplementable exactly as the GDD's own "기존엔 QA가 실제로 구현 불가능할 만�level 미명시" note says.

7. **TR-mov-010 — no code change, formal closure only.** `player-movement.md`'s own Acceptance Criteria already specifies the exact CI check (asset-import-setting verification that all jump/landing `AnimSequence` assets have `bEnableRootMotion==false`). This ADR's role is to formally trace TR-mov-010 to that existing, already-correct check — there is no architectural decision left to make here beyond confirming the check is CI-enforced, not just documented.

### Architecture Diagram

```
AMoonCharacterBase
 ├─ Tick()
 │    ├─ Super::Tick() ─────────────► CMC moves the Capsule at 100% normal rate (never dilated)
 │    ├─ EMoonAirborneSubState = sign(Velocity.Z)      [TR-mov-003 — derived, not stored/transitioned]
 │    ├─ JumpInputBufferTimer -= DeltaTime  (if > 0)   [TR-mov-007]
 │    ├─ CoyoteTimeTimer -= DeltaTime  (if > 0)        [TR-mov-007]
 │    └─ if (HitStopActive) ForceMeshTransform(FreezeStartMeshTransform)   [TR-mov-008 / V-1 — mesh only, never the Capsule]
 │
 ├─ bMovementLocked : bool (private)                   [TR-mov-006]
 │    └─ SetMovementLocked(bool) : private, uncalled until Status Effect ADR grants access
 │
 ├─ Move(FInputActionValue) ─── TRACE_CPUPROFILER_EVENT_SCOPE("MovementInputTrace.InputTriggered") [TR-mov-009]
 │    └─ if (bMovementLocked) return;  else AddMovementInput(...)
 │
 └─ (no Spell Casting #include, no Spell Casting Build.cs dependency)     [TR-mov-002]

CharacterMovementComponent::TickComponent() completion ─── TRACE_CPUPROFILER_EVENT_SCOPE("MovementInputTrace.VelocityUpdated") [TR-mov-009]

Hitstop trigger (Landed() / Dash finish)
 └─ FreezeStartMeshTransform = GetMesh()->GetComponentTransform()
      → hold anim + force mesh transform for HitStopDuration
      → blend mesh back to real transform over 1-2 frames on unfreeze
      (Capsule/CMC never touched — gameplay tick unaffected, no CustomTimeDilation anywhere)
```

### Key Interfaces

```
AMoonCharacterBase
    EMoonAirborneSubState GetAirborneSubState() const   // derived read-only, TR-mov-003
    bool IsMovementLocked() const                        // read-only query, TR-mov-006
    private: void SetMovementLocked(bool)                 // uncalled until Status Effect exists
    private: float JumpInputBufferTimer, CoyoteTimeTimer  // TR-mov-007, Character-owned
    private: void TriggerHitStop()                        // rewritten: no CustomTimeDilation, capture+blend only

TRACE_CPUPROFILER_EVENT_SCOPE channels (Development builds):
    "MovementInputTrace.InputTriggered"   — Enhanced Input Triggered delegate start
    "MovementInputTrace.VelocityUpdated"  — first frame post-CMC-tick with Velocity at target
```

## Alternatives Considered

### Alternative 1 (TR-mov-002): Split Movement into its own UBT module now

- **Description**: Create a `MoonMovement` module with its own `Build.cs`, giving TR-mov-002 a literal `PublicDependencyModuleNames` check instead of a grep-based convention.
- **Pros**: Physically enforces the independence at the build-system level — impossible to violate by accident.
- **Cons**: This project is one module by ADR-0001's own decision; introducing a second module now is a structural change with no other driver, adds build-graph complexity for a single-team MVP, and every other system (Spell Casting, Dash, Combo Gauge) still lives in the one `Moon` module — Movement would be inconsistently isolated relative to everything else.
- **Rejection Reason**: Solves a problem (accidental future coupling) that a CI grep check already solves cheaply; premature structural investment for MVP scope.

### Alternative 2 (V-1): Keep `CustomTimeDilation`, revise Rule 9 to permit it

- **Description**: Accept the shipped hitstop implementation as correct and relax Core Rule 9's blanket ban.
- **Pros**: Zero code change; hitstop already ships and presumably feels fine in the 2026-07-21 spike.
- **Cons**: `CustomTimeDilation` dilates the actor's own Tick/AnimInstance update rate — even if it "only" affects one actor, it is precisely the mechanism Rule 9 names as forbidden, and the same "presentation never gates gameplay judgment" principle is independently restated in Rule 8 (montage lock ban) and Rule 11 (execution presentation) and in `health-damage-core.md` Rule 5 (death judgment/presentation split). Revising Rule 9 alone would make hitstop the sole exception to a principle this project applies everywhere else, and would not actually fix the artifact Rule 9's capture-and-blend requirement exists to prevent (pose sliding relative to real position during dash-cancel-through-hitstop).
- **Rejection Reason**: The GDD's own design-review already reclassified this from advisory to blocking specifically because of this recurrence risk; reopening it now would reverse a considered decision without new information.

### Alternative 3 (TR-mov-003): Promote Ascending/Falling to real custom `EMovementMode`s

- **Description**: Use `SetMovementModeWithCustomMode()` to give Ascending/Falling first-class CMC movement-mode status instead of deriving them from `Velocity.Z` sign each tick.
- **Pros**: Slightly more "native-feeling" state representation; movement-mode-change delegates fire naturally.
- **Cons**: Introduces exactly the deprecated-API risk `player-movement.md`'s own Open Questions flags as unverified against real 5.8 headers (highest hallucination risk category — post-cutoff engine, signature-level claim). The GDD's "구현 노트" already explicitly rejects this ("커스텀 모드로 승격할 경우..." is presented as a hypothetical to warn against, not a recommendation), and a stored custom-mode transition reintroduces exactly the `AirTime/2`-as-timer failure mode the GDD spends a full paragraph refuting (external Z-impulse desyncs a stored mode from a timer in a way a live sign-check cannot).
- **Rejection Reason**: Higher engine risk, no concrete benefit the derived-value approach lacks, and contradicts the GDD's own explicit guidance.

## Consequences

### Positive

- Closes 7 of 10 Player Movement TRs in one ADR, resolving the largest single Foundation-layer gap from `architecture-review-2026-07-27.md`.
- V-1 resolved by fixing code to match a GDD principle applied consistently across three rules and one sibling GDD — no special-case exception introduced.
- TR-mov-009's instrumentation is cheap and immediately unlocks two previously-unimplementable Feel ACs (p95 latency, time-to-95%-speed) that were blocked purely on missing trace scopes, not missing design.
- `MovementLocked`'s access-control reservation (private, uncalled) means Status Effect's future ADR has zero ambiguity about where the field lives or who may write it.

### Negative

- `TriggerHitStop()` requires a real rewrite (capture-and-blend, not a one-line removal) — this is nontrivial engineering work, not a documentation fix, and touches code that already shipped and was spike-verified once (2026-07-21) under the old (incorrect) mechanism.
- The mesh-transform-forcing technique adds a small amount of per-tick work during the 40ms freeze window specifically — negligible at this scale, but is new code surface on `AMoonCharacterBase`, which C-4 (from the same architecture review) already flags as accreting responsibility across four ADRs.
- `MovementLocked`'s reserved-but-uncalled setter is dead code until Status Effect exists — acceptable (the TR explicitly asks for the access-control contract now), but worth noting as intentional, not an oversight, if a future linter flags it as unused.

### Risks

- **Hitstop rewrite could reintroduce the exact artifact it's meant to fix** if the capture-and-blend timing is off (e.g., blend duration too long reads as a second, smaller hitstop). Mitigation: the new Feel AC (`player-movement.md`'s "히트스탑 시각 아티팩트" AC, N≥8 playtesters, <20% "slid/snapped" response) is the acceptance gate — implementation is not done until that AC passes, not just when `CustomTimeDilation` is gone from a grep.
- **`SetMovementModeWithCustomMode` deprecation claim remains doc-section-level, not signature-level, verified** (carried forward from the GDD's own Open Question) — irrelevant to this ADR's own Decision 2 (which avoids the API entirely) but still an open risk for ADR-0007's Dash code, which does call the legacy `SetMovementMode()` overload (E-1, already tracked against ADR-0007, not duplicated here).
- **Camera-side freeze compensation (if implemented) could interact with ADR-0005's SpringArm lag model** — Core Rule 9 mentions "포즈/카메라" together, but ADR-0005 owns all camera tuning/lag behavior. Mitigation: if the camera half of the freeze technique needs SpringArm-specific changes, that implementation detail should be cross-checked against ADR-0005 rather than assumed compatible; flagged here rather than silently deferred.

## GDD Requirements Addressed

| GDD Document | Requirement | How This ADR Satisfies It |
|---|---|---|
| player-movement.md | TR-mov-002 (module compile-independence) | Decision 1 — CI grep + Build.cs dependency-name check within the single existing `Moon` module |
| player-movement.md | TR-mov-003 (airborne sub-states, external Z-impulse re-entry) | Decision 2 — derived `Velocity.Z`-sign value, sampled post-CMC-tick, no stored transition table |
| player-movement.md | TR-mov-006 (`MovementLocked` write restricted to Status Effect) | Decision 3 — private field + private setter, uncalled until Status Effect exists |
| player-movement.md | TR-mov-007 (jump buffer / coyote time, delta-time based) | Decision 4 — `Character`-owned float accumulators, inclusive `<=` boundary |
| player-movement.md | TR-mov-008 / Core Rule 9 (no Time Dilation, V-1) | Decision 5 — `CustomTimeDilation` removed, capture-and-blend presentation technique |
| player-movement.md | TR-mov-009 (input latency budget, Insights trace scopes) | Decision 6 — two `TRACE_CPUPROFILER_EVENT_SCOPE` channels at the GDD-specified measurement points |
| player-movement.md | TR-mov-010 (non-root-motion locomotion anims) | Decision 7 — formal trace to the GDD's existing asset-import CI check |

## Performance Implications

- **CPU**: Two trace scopes (Development-build only, negligible in Shipping), one derived sign-check per tick, two float countdowns per tick, one mesh-transform force during the 40ms hitstop window only — all within the existing per-frame Tick budget, no new per-frame allocations.
- **Memory**: One captured `FTransform` during an active hitstop window (transient, not persistent) — negligible.
- **Load Time**: None.
- **Network**: Out of scope, consistent with ADR-0001/0002 (no multiplayer requirement in any GDD to date).

## Migration Plan

1. Remove `CustomTimeDilation` calls at `MoonCharacterBase.cpp:282` and `MoonGameplayAbility_Dash.cpp:211`; implement the capture-and-blend `TriggerHitStop()`/unfreeze rewrite (Decision 5).
2. Add `EMoonAirborneSubState` derivation to `AMoonCharacterBase::Tick()`, placed after `Super::Tick()` (Decision 2).
3. Add `bMovementLocked`/`SetMovementLocked()` (private) and the `Move()` gate check (Decision 3).
4. Confirm/refactor `JumpInputBufferTimer`/`CoyoteTimeTimer` as delta-time countdowns with inclusive boundary if not already exactly this shape (Decision 4).
5. Add the two `MovementInputTrace` scopes (Decision 6).
6. Confirm the existing CI static checks cover TR-mov-002 (Build.cs + grep) and TR-mov-010 (asset import settings) — add if missing, no new design needed either way (Decisions 1, 7).
7. Re-run `/architecture-review` in a fresh session to confirm TR-mov-002/003/006/007/008/009/010 flip to ✅/⚠️ as appropriate and V-1 clears.

**Rollback plan**: Decisions 1–4, 6, 7 are additive/non-breaking. Decision 5 (hitstop rewrite) is the only behavior-changing piece — if the capture-and-blend technique fails its Feel AC, the rollback is to the prior `CustomTimeDilation` implementation only as a temporary measure while re-tuning the blend timing, not as a permanent reversion (that would re-open V-1).

## Validation Criteria

- `player-movement.md`'s own Acceptance Criteria for: static Spell-Casting-reference grep (TR-mov-002), Falling→Ascending re-entry (TR-mov-003), `MovementLocked` ownership stub test (TR-mov-006), coyote/buffer boundary tests at 149/150/150.5/151ms (TR-mov-007), hitstop positive-assertion + visual-artifact Feel AC (TR-mov-008/V-1), p95 latency + time-to-95%-speed Insights traces (TR-mov-009), root-motion asset-import check (TR-mov-010).
- Re-run `/architecture-review` — confirms the Player Movement coverage row moves from 1✅/2⚠️/7❌ toward full coverage.

## Related Decisions

- ADR-0001 (Player Movement and GAS Core) — this ADR fills the runtime-contract gaps ADR-0001's Decision text never addressed despite its title.
- ADR-0005 (Camera System SpringArm) — flagged in Risks re: camera-side freeze compensation; no conflict identified, cross-check recommended at implementation time.
- ADR-0007 (Dash/Evasion Just-Dodge) — carries its own separate, already-tracked deprecated-API finding (E-1, legacy `SetMovementMode()`); not duplicated by this ADR.
- `design/gdd/player-movement.md` — primary GDD source (Core Rules 1–11, States and Transitions, Formulas, Tuning Knobs, Acceptance Criteria).
- `docs/architecture/architecture-review-2026-07-27.md` — findings V-1 and the Player Movement coverage gap this ADR resolves.
