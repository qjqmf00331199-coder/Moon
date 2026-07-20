<!-- STATUS -->
Epic: Moon Fragment Hunt — DDD Expansion
Feature: **Technical Setup gate PASSED** (2026-07-20, 3rd run: 11/13 artifacts, 2 non-blocking warnings). Pre-Production phase formally entered. Report: production/gate-checks/2026-07-20-preprod-final-pass.md.
Task: Track A — Pre-Production priorities: (1) Blackhole GAS slice PIE verify + first C++ test file [W1], (2) ADR expansion (Camera/EnemyAI/Dash) [W3 architecture-review gap], (3) /create-epics when ADR suite is stable. Track B — MCP recheck 2026-07-20: `MoonEditor` build and `BP_MoonCharacter` compile succeeded. Its CDO `DefaultAbilities` contains `GA_Dash` and `/Script/Moon.MoonGameplayAbility_Spell_Blackhole`; corrected PIE-world path (`/Game/Moon/Maps/UEDPIE_0_L_CombatTest.L_CombatTest:...`) confirmed both abilities are granted at runtime. Added and saved `TargetDummy_BlackholeTest` (`ATargetDummy`) at 400uu directly ahead of the player in `L_CombatTest`. Remaining: inject/trigger the Blackhole input in PIE, then read caster/target attributes and cooldown tag to verify mana 100→30, target Health 100→75, tension 0→70, and 6s cooldown. UX — Combat HUD design complete; all sections are approved and written in `design/ux/combat-hud.md`. **HUD MVP implementation done this session** (see extract below) — Health/Mana/LowHealth wired into `WBP_CombatHUD`; still open: numeric text readouts unbound (tool bug), Overdrive/Execution/3-cooldown-slots deferred, PIE visual verification not yet done. Visual HUD direction — normal-combat mockup generated in this Codex session (crystalline HP, central mana/spells, dash + segmented tension arc); pending user approval before importing/implementing project assets. **Jump + Dash/Blackhole motion implemented this session** (see extract below) — spacebar jump with Jump_Start/Apex/Land, Dash plays "Bound", Blackhole plays "Cast" (both reused from the Aurora skeleton the character already uses). Not yet PIE-verified by the user.
<!-- /STATUS -->

## Session Extract — Jump input/motion + Dash/Blackhole motion (Claude Code, this session)
- User asked for spacebar jump + jump motion, Dash motion, and skill (Blackhole) motion. Investigated first: confirmed zero jump existed (no input binding, no `Jump()`/`StopJumping()` calls) and neither ability triggered any animation.
- **Key discovery**: `BP_MoonCharacter`'s mesh uses the **ParagonAurora "Aurora" skeleton** directly (`IdleAnim`/`JogAnim` are already `/Game/ParagonAurora/Characters/Heroes/Aurora/Animations/Idle` & `Jog_Fwd`) — so the full Aurora animation library is usable with zero retargeting. Used `Jump_Start`/`Jump_Apex`/`Jump_Land` for jump, `Bound` for Dash, `Cast` for Blackhole.
- **Architecture note**: this project has no AnimBlueprint (confirmed no AnimBP editing toolset exists in unreal-mcp either) — locomotion is a raw `SkeletalMeshComponent->PlayAnimation()` swap in `MoonCharacterBase::Tick`. Extended this same pattern rather than introducing Montages/AnimBP: added `bPlayingOneShotAnim` (suppresses the idle/jog swap while a one-shot anim controls the mesh) plus a small jump state machine (fall-start detection in Tick → Jump_Start → Jump_Apex loop → `Landed()` override → Jump_Land → resume locomotion) and a generic `AMoonCharacterBase::PlayOneShotAnim(UAnimSequence*)` (`BlueprintCallable`) that Dash/Blackhole abilities call directly.
- **Known rough edge**: if Dash or Blackhole fires while mid-air (Jump_Apex looping), the one-shot anim overrides the loop and afterward resumes ground locomotion instead of correctly returning to Jump_Apex — acceptable for this pass, not handled.
- Files touched: `MoonCharacterBase.h/.cpp` (JumpAction binding, jump state machine, `PlayOneShotAnim`), `MoonGameplayAbility_Dash.h/.cpp` (`DashAnim` property, `ConstructorHelpers` default = `Bound`, calls `PlayOneShotAnim` in `ActivateAbility`), `MoonGameplayAbility_Spell_Blackhole.h/.cpp` (`CastAnim` property, `ConstructorHelpers` default = `Cast`, plays on cast regardless of hit/whiff). Blackhole has no BP wrapper (used as a raw native class in `DefaultAbilities`), so its anim default is set via `ConstructorHelpers` in the constructor rather than a BP CDO.
- New asset: `IA_Jump` (`/Game/Moon/Input/IA_Jump`, Boolean). Edited `IMC_MoonCombat` — added Spacebar → `IA_Jump` (found the real mapping data lives under the nested `defaultKeyMappings.mappings` property in UE5.8's new key-profile system, not the top-level `mappings` array which was empty). Set `BP_MoonCharacter` CDO: `jumpAction`, `jumpStartAnim`, `jumpApexAnim`, `jumpLandAnim`.
- Build: header/UPROPERTY changes required a full rebuild per this project's established practice — closed the editor, ran `Build.bat MoonEditor Win64 Development -project=...`, succeeded, relaunched editor, reconnected unreal-mcp, verified new properties exist before touching assets.
- Verified via `get_properties` that both `GA_Dash`'s CDO and Blackhole's native CDO (`/Script/Moon.Default__MoonGameplayAbility_Spell_Blackhole`) correctly picked up their `ConstructorHelpers` anim defaults (Bound / Cast) without any manual BP/CDO editing needed.
- **Not yet done**: user has not PIE-tested spacebar jump, Dash motion, or Blackhole cast motion yet.

## Session Extract — Combat HUD MVP implementation via unreal-mcp (Claude Code, this session)
- Delegating to `ue-umg-specialist` subagent failed — subagents spawned via the `Agent` tool do NOT inherit unreal-mcp tool access (only the main-thread session has it). Did the UMG work directly instead.
- `WBP_CombatHUD` (`/Game/Moon/UI/WBP_CombatHUD`) widget tree extended: added `ProgressBar_Health` + `TextBlock_HealthValue` (bottom-left), `ProgressBar_Mana` + `TextBlock_ManaValue` (bottom-center), `Border_LowHealthVignette` (full-screen overlay, initially Collapsed). All anchored with relative anchors (not absolute px) per `design/ux/combat-hud.md`. Existing `ProgressBar_0` (Tension) / `TextBlock_0` (Dash) bindings verified untouched.
- Event graph: wired `OnHealthChanged`→`ProgressBar_Health.SetPercent` (Current/Max), `OnManaChanged`→`ProgressBar_Mana.SetPercent`, `OnLowHealthWarningStateChanged`→ Branch → `Border_LowHealthVignette.SetVisibility` (HitTestInvisible/Collapsed). Built via manual `create_node`/`connect_pins`/`set_pin_value` (BlueprintTools), NOT `write_graph_dsl` — see bug note below.
- **Tool bug found**: `write_graph_dsl` cannot round-trip the pre-existing Dash `TextBlock_0.SetText` binding — its `read_graph_dsl` output identifies the node as `Class|Factory|SetText`, but feeding that exact string back into `write_graph_dsl` resolves to an unrelated `Factory.bText` setter (type mismatch error). Confirmed via `create_node` with explicit `declaring_class` that `Class|Factory|SetText` is not a valid TextBlock overload at all. Root cause: that binding was originally hand-built in the UMG editor by a human, never round-tripped through the DSL tool before. **Do not run `write_graph_dsl` on this graph's existing content again** — any whole-graph DSL rewrite will hit this and fail (transactional, rolls back cleanly, but blocks the whole write). Add new events via `create_node`/`connect_pins` instead.
- Consequence: `TextBlock_HealthValue`/`TextBlock_ManaValue` were added but **not bound** — no valid TextBlock::SetText node type_id could be found via `find_node_types`/`create_node` search (tried `Widget|SetText`, `Widget|SetText(Text)`, `Class|Text|SetText`, `Class|TextBlock|SetText` — all wrong class or nonexistent). They currently show static placeholder text "100 / 100". Numeric readout is marked "필요 시" (situational) in the UX spec, so this was deprioritized rather than risk further graph corruption. Follow-up: find the correct type_id (or wire manually in-editor) to bind these two.
- Compiled clean (`compile_blueprint`, no errors in `LogBlueprint`), all 3 new events show `bIsImplemented:true` via `list_events`, saved to disk.
- **Not yet done**: PIE visual verification (bars should track GAS Health/Mana in real time, vignette should appear <25% health). Overdrive/Execution-prompt/3-spell-cooldown-slots explicitly deferred (no runtime data contract yet, matches earlier scope decision).

## Session Extract — Track B takeover + real verification (Claude Code, this session)
- User asked to take over Track B entirely from Antigravity/Gemini going forward.
- **Root cause found before doing anything else**: the "did the tension bar show up?" question from
  last session could never be answered — the test scene (`L_TestScene`) was an **unsaved** `/Temp/Untitled_1`
  level. Editor had since restarted; that level was gone, no `.umap` for it ever existed on disk.
  All prior interactive verification (pawn placement, Auto Possess, widget test hookup) was lost.
  The "no Slate crash" result from earlier this session was against that same empty default level with
  no pawn — proved nothing (widget was never constructed, so the `bWasFound` assert path was never hit).
- **Fixed durably**: duplicated `/Game/_Spikes/L_Spike_ArenaMorph` → new real asset
  `/Game/Moon/Maps/L_CombatTest.umap` (has PlayerStart + floor already). Placed `BP_MoonCharacter`
  at PlayerStart, set `AutoPossessPlayer = Player0` via ObjectTools. Added a debug key binding
  (Keyboard `1` → `AddTension(self, 30)`) to `BP_MoonCharacter`'s EventGraph via
  `BlueprintTools.write_graph_dsl` — verified node-level via `get_node_infos` that Pressed→AddTension
  is actually wired (the DSL round-trip printer drops standalone InputKey continuations on read-back,
  cosmetic tool quirk, not a real gap). Compiled clean. Saved all assets.
- **Verified for real via unreal-mcp** (not just "hit Play and eyeball it"):
  - `StartPIE` with the pawn present and BeginPlay's `CreateWidget→BindToPlayer→AddToViewport` chain
    actually executing — **no crash**, matching the prior known `bWasFound` Slate assert signature
    (grepped full session log for it — never fires, including during this pawn-present run).
  - `GASToolsets.AbilitySystemInspectorToolset.GetAttributeValues` on the live PIE pawn confirmed
    GAS/attribute init is healthy: Health 100/100, Mana 100/100, TensionGauge 0/100, DashCharges 2/2.
  - `WBP_CombatHUD` widget tree confirmed intact post-recreation: parent class correct
    (`UMoonCombatHUDWidget`), 3 widgets (CanvasPanel root + `ProgressBar_0` + `TextBlock_0`),
    `OnTensionStateChanged` wired to `ProgressBar_0.SetPercent`, `OnDashChargesChanged` wired to
    `TextBlock_0.SetText` — Health/Mana/Overdrive/ExecutionPrompt events still unimplemented (expected,
    WIP, not a regression).
  - Could **not** get a pixel-level screenshot proof of the bar rendering — `CaptureViewport`
    only captures the 3D scene render (editor gizmos toggle only), and the Slate accessibility
    snapshot doesn't decompose the UMG game-layer overlay. Sending a real keypress into the PIE
    viewport via `SlateInspectorToolset.PressKey` also didn't take (no Slate focus on the game
    viewport from an out-of-process caller). Structural + runtime-state verification above is the
    evidence in place of a screenshot; a human hitting `1` in a real PIE session and eyeballing the
    bar move is still the one remaining manual check.
- **GameMode note**: still no GameMode set anywhere (project or level) — this test level relies on
  the manual `AutoPossessPlayer` flag on the placed pawn, same fragile-but-working pattern as before.
  Consider a `GM_MoonTest` with `DefaultPawnClass = BP_MoonCharacter` if this keeps needing rebuilding.
- **Not committed yet** — `Moon/Content/Moon/` and `Moon/Source/Moon/` are still fully untracked
  (first time either would ever be committed); asked user for commit-scope confirmation before
  sweeping them in alongside the untracked marketplace packs (ParagonAurora/Ghoul/GoodSky/etc.)
  sitting in the same `git status` output.

## Session Extract — /gate-check pre-production 2026-07-18
- Verdict: FAIL
- 3/13 required artifacts present. Missing: architecture.md, requirements-traceability.md, art-bible.md, accessibility-requirements.md, interaction-patterns.md, tests/ dir, CI workflow, architecture-review report. Only 1 ADR (0001, still Proposed, missing required sections).
- Report: production/gate-checks/2026-07-18-technical-setup-to-preprod.md

## Session Extract — /review-all-gdds 2026-07-18
- Verdict: FAIL
- GDDs reviewed: 9 (+ game-concept.md, systems-index.md)
- Flagged for revision: combo-tension-gauge.md, luna-overdrive.md, combat-hud.md (status corrected in systems-index.md: Approved → Needs Revision), plus minor edits needed in spell-casting-base.md, health-damage-core.md, dash-evasion.md, player-movement.md, camera-system-base.md
- Blocking issues: systems-index.md falsely claimed 9/9 GDDs Approved (Progress Tracker + 3 table rows) when only 6/9 actually say Approved in their own headers — corrected this session (approved count 9→6, 3 rows → Needs Revision)
- Design-theory warnings (non-blocking): W9 Overdrive refresh-chain damping lever (OverdriveTensionGainMultiplier) defaults to 1.0 = no protection, risks the tension curve's "sharp drop" never landing — recommend addressing during combo-tension-gauge.md's actual review; W10 player attention budget slightly over threshold (mitigated by HUD design + Overdrive-state collapse); W11 base spell RawDamage (assumed 25) has no owning GDD
- Recommended next: run the 3 pending /design-review passes (luna-overdrive re-review, combo-tension-gauge, combat-hud), folding in W9 as a review-time question
- Report: design/gdd/gdd-cross-review-2026-07-18.md

## What changed this session (Track B: C++ GAS Foundation implementation)
- **Implemented Foundation Classes**: Implemented `AMoonCharacterBase`, `UMoonAbilitySystemComponent`, and `UMoonAttributeSet` in C++ based on `ADR 0001`.
- **Attribute Set Setup**: Added Health, Mana, and TensionGauge attributes to `UMoonAttributeSet` with basic clamping in `PreAttributeChange` and `PostGameplayEffectExecute`.
- **Character Setup**: Hooked up `AbilitySystemComponent` and `AttributeSet` instantiation in `AMoonCharacterBase`, and prepared initialization hooks (`InitializeAttributes`, `InitializeAbilities`) for GE and default abilities.
- **Enhanced Input MCP Asset Generation**: Used `unreal-mcp` (DataAssetTools) to create `IA_Move` (Axis2D), `IA_Look` (Axis2D), `IA_Dash`, `IA_Spell_Blackhole`, `IA_Spell_Fire`, `IA_Spell_Lightning`, `IA_Execute`, and `IMC_MoonCombat` in `/Game/Moon/Input/`.
- **Enhanced Input C++ Integration**: Updated `MoonCharacterBase.h` and `.cpp` to expose these Input Actions and the Mapping Context, bound them in `SetupPlayerInputComponent()`, and implemented the `Move` and `Look` functions. `IMC_MoonCombat` is dynamically added during `BeginPlay`. Editor assignment completed successfully after full UBT DLL compile.
- **Spell Casting C++ Skeleton**: Drafted ADR 0002, created `UMoonGameplayAbility_Spell` with `CostBypass.Active` bypass logic. Implemented input routing in `AMoonCharacterBase` and the cast rate limiter (Rule 11: per-frame per-element cap + global MaxCastsPerSecond cap).
- **Next Steps for Track B**: 
  - Editor re-compile is required to pick up the new `UMoonGameplayAbility_Spell` class and new input bindings.
  - Proceed to Dash/Evasion C++ skeleton (Charge management, Just-dodge timer, Invulnerable/Executable tag grants).

## What changed this session (spell-casting-base.md cast-rate floor revision — BLOCKING cross-dep from luna-overdrive.md review)
- **Cast-rate floor added** to `spell-casting-base.md` as Core Rule 11 — per-element per-frame cap
  (동일 속성 프레임당 1회 제한) + 전역 `MaxCastsPerSecond` Tuning Knob (기본 20, Safe Range 10–30).
  이 규칙은 `CostBypass.Active` 우회 여부와 무관하게 항상 적용.
- **Edge Case 추가**: 오버드라이브 중 터보/매크로 장치로 프레임마다 동일 속성 캐스트 연사 시나리오.
- **AC 2건 추가**: AC10 (per-frame cap 검증) + AC11 (MaxCastsPerSecond 전역 상한 검증).
- **Tuning Knob 추가**: MaxCastsPerSecond (기본 20, 하드 클램프 최소 10).
- **Registry 등록**: `max_casts_per_second` 상수 — `entities.yaml`에 추가 완료.
- **luna-overdrive.md 반영**: BLOCKING Open Question을 RESOLVED로 마킹.
- **Status 헤더 업데이트**: spell-casting-base.md의 revision 내역 반영.
- Fable(Claude Code)의 luna-overdrive.md design-review 세션에서 식별된 유일한 BLOCKING 크로스
  의존성이 해소됨 — re-review 전제조건 충족.

## What changed this session (/design-review luna-overdrive.md — full depth)
- Full adversarial review: game-designer, systems-designer, qa-lead, ue-gas-specialist in parallel
  + creative-director senior synthesis. Verdict **NEEDS REVISION** → all 5 blocking items fixed
  in-session → status now "Designed (pending re-review)". Log:
  `design/gdd/reviews/luna-overdrive-review-log.md`.
- **Blocker fixes applied to luna-overdrive.md**:
  1. Same-frame ordering (release>gate, retrigger>expiry, death>trigger) now has an enforcement
     mechanism: lazy CurrentTime-vs-OverdriveEndTime comparison at every evaluation point +
     end-of-frame expiry/activation finalization — ordering true by construction, not tick luck.
  2. CostBypass.Active implementation note: NON-ref-counted set/clear mandated (documented
     exception to the HDC ref-count convention — single-grantor tag); death-path double-remove
     (LO Rule 6 vs HDC blanket overlay clear) pinned as idempotent/benign, LO owns release,
     ref-count adoption forbidden until that overlap is resolved.
  3. AC9 added (expiry+retrigger same-frame → refresh, NO Ended/Started events).
  4. Refresh-chain damper: **OverdriveTensionGainMultiplier** added to combo-tension-gauge.md
     (new Core Rule 7 — all tension gains ×multiplier while CostBypass.Active present; default
     1.0 = behavior unchanged, 0.0 = gauge frozen during overdrive). Registered in registry.
     Boundary case documented (coefficient 1.5 ⇒ Blackhole=105 ⇒ every cast refreshes).
  5. Cast-rate floor registered as BLOCKING cross-dependency on Spell Casting (base) —
     turbo/macro exploit (frame-instant + no recovery + bypass = input-rate-bound output);
     min contract (1 cast/spell/frame or global CPS knob) is spell-casting-base.md's to add
     before LO implementation. LO does not invent the cap (creative-director adjudication).
- **Recommended fixes also applied**: AC test-entry contract (harness publishes OnOverdriveTriggered;
  direct tag grant forbidden — opposite of spell-casting AC5's stub, reason documented), per-AC
  Logic/Integration evidence tags, Duration-GE implementation recommendation (refresh-duration
  stacking, GE remaining-time query — 5.8 header check flagged), fragment-absorption debug-trigger
  note.
- systems-index.md row #11 left at "Designed (pending review)" — accurate until re-review passes.
- Not run this session (by protocol): re-review — needs fresh session.

## What changed this session (handoff execution — Combat HUD GDD, 2 of 2 — MVP 9/9 designed)
- Authored complete GDD at `design/gdd/combat-hud.md`, solo review mode, status "Designed (pending review)".
- Core design: pure read-only mirror layer, 7-widget MVP inventory (health bar, low-health warning,
  mana bar + ∞ overdrive state, 3 spell cooldown slots, dash charge pips (floor + fraction), tension
  gauge with Building/Decaying/Charged visual language, overdrive indicator + execution prompt).
  Event-driven updates only (tick allowed solely for active lerp/sweep), per-frame update coalescing,
  display interpolation never gates gameplay signals (real-value triggers only). All widgets
  non-focusable — pad nav requirement satisfied by staying out of the focus graph; F-key glyph swaps
  to pad button glyph on device change.
- **Assumed interface (handoff-mandated)**: execution prompt binds to `State.Executable` tag presence
  + proximity assumption; precise trigger signal deferred to Core Extraction Execution GDD
  (Open Question, Owner: game-designer).
- **Value confirmed**: `TensionChargedHighlightThreshold` = 0.90 — the threshold combo-tension-gauge.md
  explicitly delegated to Combat HUD ("예: 90%+"). Registered as new constant.
- Registry: added `tension_charged_highlight_threshold`; appended combat-hud.md to referenced_by on
  `tension_gauge_max`, `cooldown_blackhole/fire/lightning`, `executable_duration`, `overdrive_duration`.
- systems-index.md: row #20 → "Designed (pending review)" + link; Dependency Map/enumeration row
  gained Health/Damage Core, Spell Casting (base), Dash/Evasion (upstream UI Requirements delegations
  the original index missed); progress started 9, **MVP 9/9 designed**.
- OLLAMA-INSTRUCTIONS.md: queued Task 14 (registry fact-check HUD), Task 15 (terminology HUD vs
  luna-overdrive), Task 16 (AC → QA checklist HUD).
- Manual /consistency-check procedure run (registry vs all 9 GDDs, grep-targeted): **0 conflicts**.
  Shared values verified: overdrive 10s, charged 0.90, tension gains 70/25, cooldowns 6.0/2.0/2.5,
  executable_duration 3.0, telegraph 0.4, engage 400/1200. Known cosmetic staleness (not a value
  conflict, left for review session): spell-casting-base.md prose still labels Luna Overdrive
  "(미설계)" in Rules 10/Interactions — LO is now designed.
- Solo mode skipped agent spawns: creative-director, ue-umg-specialist, ux-designer, art-director,
  qa-lead — flagged in GDD as needing manual review before Production.

<!-- CONSISTENCY-CHECK: 2026-07-17 | GDDs checked: 9 | Conflicts found: 0 | Scope: luna-overdrive.md + combat-hud.md 추가 반영 (MVP 9/9) -->

## What changed this session (handoff execution — Luna Overdrive GDD, 1 of 2)
- Executed `production/handoff/fable-mvp-remaining-2-systems.md` (user pre-approved, no-questions run).
- Authored complete GDD at `design/gdd/luna-overdrive.md`, solo review mode, status "Designed (pending review)".
- Core design: pure consumer of Combo/Tension Gauge's `OnOverdriveTriggered`; activation = grant
  `CostBypass.Active` (spell-casting-base.md Rule 10 hook — single tag bypasses mana AND cooldown
  simultaneously); duration timer 10.0s (`overdrive_duration`, game-concept.md value, Safe Range 6–15s);
  end = tag release with the bypass-release same-frame order contract (tag resolves BEFORE cast gate).
  Re-trigger while Active = refresh timer to full (no stacking, single tag grant maintained).
  End reasons: Expired | PlayerDeath (death forces immediate end, no carry-over).
  Emits `OnOverdriveStarted`/`OnOverdriveEnded(EndReason)` + `OverdriveTimeRemaining` for Combat HUD
  and Overdrive Visual State (#18, still undesigned — owns the real crimson inversion; this GDD only
  ships a minimal crimson post-process tint as MVP stand-in).
- **Upstream open question resolved**: `OnOverdriveTriggered` payload = NONE (parameterless) —
  reflected in combo-tension-gauge.md Open Questions (strikethrough + RESOLVED note, body unchanged).
- **Bidirectionality fix**: health-damage-core.md gained a one-line Luna Overdrive downstream entry
  (Interactions + Dependencies) for the player-Death subscription; systems-index.md row #11 and
  Dependency Map gained the same soft dependency.
- **Known risk documented, not capped**: refresh-chain (re-filling gauge during overdrive with free
  Blackhole spam can sustain overdrive indefinitely in dense packs) — allowed by upstream Core Rule 6,
  flagged as playtest Open Question; any future damper belongs on the gauge side, not here.
- Registry: added `overdrive_duration` constant; appended luna-overdrive.md to referenced_by on
  `effective_mana_cost`, `cast_gate_check`, `tension_gauge_max`; updated stale "undesigned" notes.
- systems-index.md: row #11 → "Designed (pending review)" + doc link, progress started 8, MVP 8/9.
- OLLAMA-INSTRUCTIONS.md: queued Task 11 (registry fact-check LO), Task 12 (terminology LO vs
  combo-tension-gauge), Task 13 (AC → QA checklist, new task type per handoff template).
- Solo mode skipped agent spawns: creative-director, systems-designer, art-director, qa-lead —
  flagged in GDD as needing manual review before Production.

## What changed this session (/design-system combo-tension-gauge)
- Authored complete 8-section GDD at `design/gdd/combo-tension-gauge.md`, solo review mode.
- Core design: single GAS Attribute `TensionGauge` (0-100), gains from `OnSpellHit` (proportional to
  spell-casting-base.md's ManaCost — Blackhole=70, Fire/Lightning=25) and Just-Dodge success (flat +20,
  detected indirectly via `OnTagAdded(State.Executable)` — dash-evasion.md left unmodified). Decays after
  3s grace at 10/sec, damage-taken penalty -20% (proportional), resets to 0 on Overdrive trigger or death.
- Resolved a scope conflict during design: game-concept.md's tension curve explicitly names "스펠 위빙
  콤보" as a tension-rising source — confirmed both spell-hit AND just-dodge count toward the gauge
  (not just-dodge alone).
- Registered 6 new constants in `design/registry/entities.yaml` (tension_gauge_max, tension_gain_coefficient,
  just_dodge_tension_bonus, tension_decay_grace_period, tension_decay_rate_per_sec, damage_penalty_percent).
  Added combo-tension-gauge.md to `referenced_by` on mana_cost_blackhole/fire/lightning (reuses those values).
- Updated `systems-index.md`: row #9 → "Designed (pending review)", progress counts (started 7, MVP 7/9).
- Solo mode skipped agent spawns: creative-director, systems-designer, art-director, qa-lead — flagged
  in GDD as needing manual review before Production.

## What changed this session (/design-review dash-evasion.md)
- Reviewed and **APPROVED** `design/gdd/dash-evasion.md` end-to-end (independent review session).
- Generated complete design review artifact [design_review_dash_evasion.md](file:///C:/Users/qjqmf/.gemini/antigravity-cli/brain/c10d9217-e8ef-42ec-b6a5-00f7b4193e8a/design_review_dash_evasion.md).
- Updated `dash-evasion.md` status to `Approved`. Fixed a missing `Combat HUD` dependency in its Dependencies table.
- Updated `systems-index.md` row #7 to `Approved`, incremented progress metrics (reviewed 6, approved 6).
- Auto-queued Task 9 (registry check) and Task 10 (terminology consistency vs Health/Damage Core) to `OLLAMA-INSTRUCTIONS.md` per auto-queueing policy.

## What changed this session (/design-review spell-casting-base.md — full depth)
- Full adversarial review: game-designer, systems-designer, qa-lead, ue-gas-specialist in
  parallel + creative-director senior synthesis. Initial verdict **NEEDS REVISION** → all 5
  blocking items fixed in-session → **APPROVED**. Log:
  `design/gdd/reviews/spell-casting-base-review-log.md`.
- **Design change**: Blackhole ManaCost 100 → **70**. Cost==MaxMana locked out ALL spells ~3.1s
  after cast, contradicting Rule 4 ("no forced recovery") and sealing the Supernova weave path.
  Registry `mana_cost_blackhole` updated (revised 2026-07-17, old value commented).
- **3 new tuning cross-constraints** (Tuning Knobs): (1) no-infinite-spam
  `ManaCost > Regen×CD`, (2) cooldown-binds-Blackhole, (3) weave-guarantee
  `MaxMana − Cost(BH) ≥ 25` — pillar-critical.
- **GAS contracts pinned** (were blank): Rule 4 — same-frame ability lifecycle, presentation-only
  cancel, no CancelAbility; Rule 10 — skip CommitAbility() when `CostBypass.Active` (documented
  exception to GE-only principle); mana snap Override GE vs periodic regen = benign race.
- **ACs**: AC1/2 renumbered values; AC5 stubbed (test harness applies tag directly); AC7 rewritten
  observable (800uu hearing check); old AC8 (Supernova shield-pierce) removed → deferred to
  Spell Weaving GDD; new AC8 = weave-guarantee execution check.
- **3 new edge cases**: Mana=0 cast, all-3-elements-on-cooldown, bypass-release same-frame race
  (tag change resolves BEFORE gate evaluation — Luna Overdrive GDD must assume this order).
- systems-index row #6 → Approved (reviewed 5, approved 5). Ollama queue: Task 7 (registry
  fact-check spell-casting) + Task 8 (terminology spell-casting vs health-damage-core) appended
  per auto-queueing policy.

<!-- CONSISTENCY-CHECK: 2026-07-17 | GDDs checked: 5 | Conflicts found: 1 (resolved) | Log: docs/consistency-failures.md -->

# Session State

## What changed this session (/design-system dash-evasion)
- Authored the complete 8-section GDD for Dash/Evasion at `design/gdd/dash-evasion.md`, solo review mode.
- MVP scope: Charge-based dash (2 charges, 2.0s recharge), horizontal + vertical impulse, i-frames (`State.Invulnerable`), and Just-Dodge functionality.
- Just-Dodge: Timing window of 0.2s before Enemy AI's `OnAttackCommitted`. Success grants `State.Executable` to the attacker and refunds 1 dash charge.
- Key interface contracts established: Dash does not lock Movement or interrupt Casting. Future Status Effect can lock Dash via `MovementLocked`.
- Registered in `design/registry/entities.yaml`: 3 new constants (`just_dodge_window`, `dash_invuln_duration`, `executable_duration`).
- Updated `systems-index.md`: row #7 → "Designed (pending review)", progress counts incremented (started 6, MVP designed 6/9).

## What changed this session (/design-system spell-casting-base)
- Authored the complete 8-section GDD for Spell Casting (base) at
  [spell-casting-base.md](file:///D:/moon-fragment-hunt/design/gdd/spell-casting-base.md), solo review mode
  (no specialist agents consulted — needs manual review before Production, and an independent
  `/design-review` pass in a fresh session before being marked Approved).
- **MVP scope**: 3 fixed elemental spell slots (Blackhole/Radial Force, Fire, Lightning) — no
  loadout swapping. Instant-cast (no windup/charge), per-element independent cooldowns, single
  shared Mana pool with passive regen + Core Extraction full-refill bonus.
- **Key numbers** (solo-proposed, unplaytested): MaxMana=100, ManaCost Blackhole=100/Fire=25/
  Lightning=25, Cooldown Blackhole=6.0s/Fire=2.0s/Lightning=2.5s, ManaRegenRate=8/s,
  NoiseLoudness Blackhole=1.0/Fire=0.6/Lightning=0.7.
- **Key interface contracts established** (carry forward when designing dependents):
  - Casting NEVER blocks movement (`MovementLocked` not used) — symmetric counterpart to Player
    Movement Core Rule 7, both docs now own this contract from their own side.
  - All spell damage goes through Health/Damage Core's single `ApplyDamage` entry point
    (Rule 1); base 3 spells default `bBypassDefense=false` — only Spell Weaving's synergy
    combos (Supernova etc.) may override to `true`.
  - This doc is the one that defines the Core Extraction Execution mana-refund amount
    (100% / snap to MaxMana on `OnExecuted`) — Health/Damage Core Rule 9 explicitly deferred
    that number to whichever system subscribes to the event, and this is that system.
  - Every spell impact must call `MakeNoise(loudness)` per Enemy AI (base)'s hearing contract —
    `NoiseLoudness` per element is a new registry formula (`spell_noise_loudness`) that chains
    directly into Enemy AI's `noise_detection_radius` formula.
  - Luna Overdrive (undesigned) will consume a single `CostBypass.Active` GameplayTag that
    bypasses BOTH mana cost and cooldown gating simultaneously — no partial-bypass mode exists
    in this MVP scope (Edge Cases explicitly closes that gap).
- **Registered in `design/registry/entities.yaml`**: 8 new constants (max_mana, mana_cost_*,
  cooldown_*, mana_regen_rate), 4 new formulas (effective_mana_cost, cast_gate_check, new_mana,
  spell_noise_loudness). Also appended `design/gdd/spell-casting-base.md` to the existing
  `noise_detection_radius` formula's `referenced_by` (cross-doc formula chain).
- **Updated `systems-index.md`**: row #6 → "Designed (pending review)", Design Doc link added,
  progress counts incremented (started 5, MVP designed 5/9).
- **Open questions flagged, not resolved by this doc** (see GDD's own Open Questions section):
  cast aiming method (auto-aim/skillshot/cursor) undecided, gamepad button mapping for
  3 spells + movement + dash undecided, UE5.8 GAS attribute-init pattern needs
  `ue-gas-specialist` header verification (same gap as health-damage-core.md).

## What changed this session (/design-review enemy-ai-base.md + Ollama pipeline restructure)
- Ran a formal `/design-review` on `design/gdd/enemy-ai-base.md` (solo depth, per project default) —
  **APPROVED**, no blockers, no doc edits needed. Doc's front-matter already said Approved from the
  prior manual pass; this was the first pass through the actual `/design-review` skill.
- Restructured the Ollama overnight pipeline at the user's request:
  - Moved the task queue `production/ollama-instructions.md` → `OLLAMA-INSTRUCTIONS.md` (repo root),
    so the bot reads it directly instead of the (never-actually-implemented) "Gemini scans the
    whole repo" design the docs used to describe.
  - Reformatted every task's "Context to inject" into strict `- {{PLACEHOLDER}}: path` lines and
    made Tasks 3/5/6 fully self-contained (they used to say "same prompt as Task 2/4") — required
    for the new parser, which treats each task block independently.
  - Rewrote `tools/overnight-bot/discord_ollama_bot.py`: removed the hardcoded `TASKS` list, added
    `parse_task_queue()` (regex-parses `OLLAMA-INSTRUCTIONS.md`, only runs `[ ]` blocks, warns in
    Discord about malformed ones instead of silently dropping them). Verified with a standalone
    parse test (all 6 tasks parse correctly) and `python -m py_compile` (syntax OK) — have not
    run the live bot (needs Discord/Gemini/Ollama credentials not available in this session).
  - Corrected the stale "Gemini reads the repo and picks tasks" framing in
    `production/ollama-delegation-criteria.md` and `production/overnight-protocol.md` to match
    reality: Gemini only 3-line-summarizes Ollama's output, never reads the queue or the repo.
  - Updated all path references: `.claude/docs/ollama-delegation.md`, `AGENTS.md`,
    `tools/overnight-bot/README.md`, `tools/overnight-bot/setup-plan-macbook.md`.
  - Note: this was a detour, not a step in the GDD design order below — next real design work is
    still Spell Casting (base).

## Cross-tool note (added 2026-07-16, updated 2026-07-16 — 3rd worker added)

This project now rotates across **three** workers: **Claude Code** and **Antigravity CLI**
(Gemini) do the real design/implementation work session to session; **Ollama** runs unattended
overnight on a small queue of low-risk, easy-to-verify tasks (its output is never trusted
directly — see below). Whichever tool picks up next:

- Read this file first, always — it's the shared handoff point for all three workers.
- **Check `production/overnight-output/` before starting real work** — if Ollama ran overnight,
  review its drafts against `OLLAMA-INSTRUCTIONS.md`'s (repo root) checklists (a couple minutes),
  promote what's good into the real files, discard the rest, then clear the directory. Full
  protocol: `production/overnight-protocol.md`. Ollama has no file/tool access — it only ever
  produces draft text that a human or one of you reviews; it never edits `design/gdd/`, `docs/`,
  or `entities.yaml` directly.
- Antigravity CLI now has its own project context mirror at [AGENTS.md](file:///D:/moon-fragment-hunt/AGENTS.md)
  (Claude Code keeps reading `CLAUDE.md` + `.claude/` as before — unchanged).
- `unreal-mcp` is now registered for **both** tools: Claude Code via `.mcp.json` /
  `Moon/.mcp.json` (already existed), Antigravity CLI via the newly created
  [.agents/mcp_config.json](file:///D:/moon-fragment-hunt/.agents/mcp_config.json) (same server,
  `http://127.0.0.1:8000/mcp`, just a different config key: `serverUrl` instead of `type`+`url`).
- Claude Code has guided slash-command skills (`/design-system`, `/design-review`, etc.)
  that Antigravity CLI doesn't have equivalents for. If Antigravity is doing the next design
  pass, follow the section list + rules written directly into `AGENTS.md` instead of expecting
  a slash command; the output should still satisfy the same 8-section GDD standard so either
  tool can review the other's work.
- Whoever finishes a session should update **this section's task pointer above and the
  "To resume in a new session" list below** before ending, so the other tool doesn't stall
  waiting for missing context.

## Current task — PARALLEL TRACKS (Sonnet + Antigravity)

**Track A (Sonnet): Design Review & Tutorial Planning**
- **Next System to Design**: `design/gdd/tutorial-flow.md` (Tutorial and onboarding sequence).
- **Reviews Pending**: `luna-overdrive.md` (re-review), `combo-tension-gauge.md`, `combat-hud.md`.
- **Handoff Prompt**: See `sonnet-prompt.md` in artifacts.

**Track B (Antigravity CLI): Implementation Foundation**
- **ADR Drafting**: Drafted ADR 0002 for Spell Casting (base).
- **UE C++ Setup**: Initialize the `Moon` project as a C++ project (currently Blueprint-only). Create `Source/` directory, `.Build.cs`, Target files, and initial GameMode/GAS base classes. (Completed)
- **Enhanced Input Setup**: Created base `InputMappingContext` and `InputAction` assets via MCP. Integrated into `MoonCharacterBase` C++. (Completed)
- **Spell Casting**: Implement `UMoonGameplayAbility_Spell` and bind input actions to ability activation. (Completed)
- **Dash/Evasion**: Implement Dash ability (`UMoonGameplayAbility_Dash`) and charge management system (in `UMoonAttributeSet` and `AMoonCharacterBase::Tick`). Added skeleton for Just-Dodge window. (Completed)
- **Combat HUD**: Implemented `UMoonCombatHUDWidget` (UMG base class) connecting GAS Attributes to Blueprint Implementable Events with visual lerping and low health warnings. (Completed)
- **Combo/Tension Gauge**: Implemented `AddTensionFromSpellHit`, `AddTensionFromJustDodge`, and `ApplyTensionDamagePenalty` in `MoonCharacterBase`. Added Tension Decay logic in `Tick` and `OnOverdriveTriggered` event emission. (Completed)
- **BP Integration & Testing (Current Session)**: 
  - Ran `FixAssets.py` to correctly assign `MoonCombatHUDWidget` parent class to `WBP_CombatHUD` and generated `GA_Dash` post-Live-Coding-restart.
  - Guided user through connecting UMG UI nodes (`Set Percent`, `Set Text`) and GAS Tag assignment (`Ability.Dash`).
  - Added UI creation sequence (`Create Widget` -> `Bind To Player` -> `Add To Viewport`) to `BP_MoonCharacter` `BeginPlay` along with test input (`Keyboard 1` -> `Add Tension(30)`).
  - Resolved pawn possession issue in `L_TestScene` by setting `Auto Possess Player = Player 0`.
  - **Identified Slate Crash Issue**: Hitting Play caused `bWasFound` assert in `SlateApplication.cpp` due to UMG Slate hierarchy corruption caused by Python `reparent_blueprint`. Guided user to manually recreate `WBP_CombatHUD` cleanly from scratch.
- **Next Steps**: Await user confirmation that manually recreating `WBP_CombatHUD` resolved the Slate crash and that the UI successfully responds to the test input. Track B is effectively complete once UI testing passes.

**Key decisions baked into Enemy AI (base) (carry forward when designing dependents):**
- MVP scope is 2 archetypes only: Grunt (melee, MaxHealth 30) and Ranged (MaxHealth 20, keeps a
  400–1200uu engage band and retreats if the player closes inside 400uu).
- Perception uses Sight (cone) + Hearing (`MakeNoise`-driven) — any system causing a loud event
  (destructible collapse, spell impact, player landing) should call `MakeNoise` with a
  0.0–1.0 loudness value; this doc doesn't enumerate all noise sources itself.
- Attack telegraphs are anim-driven: `OnAttackTelegraphed` (windup start AnimNotify) and
  `OnAttackCommitted` (strike-frame AnimNotify, this is when damage actually applies). The gap
  between them (`TelegraphWindow`) must stay >= 0.4s (`min_telegraph_window`, registered in
  entities.yaml) or the attack isn't fairly dodgeable — **Dash/Evasion must subscribe to these
  two delegates** for its just-dodge timing, not invent its own detection.
- This doc does NOT grant `State.Invulnerable`/`State.Executable` (Dash/Evasion and Enemy Elite
  Shield own that) — it only reads them, and confirmed those tags never block its own state
  transitions (they're overlays).
- Stagger is a slot only — Super Armor / CC Interrupt (undesigned) owns exactly 기when it triggers;
  this doc just exposes `TriggerStagger()`/`ClearStagger()`.
- Archetype identity exposed via GameplayTag (`Enemy.Archetype.Grunt`/`Enemy.Archetype.Ranged`)
  for Enemy Elite Shield (undesigned) to key off of.
- Registered in `design/registry/entities.yaml`: entities `grunt`/`ranged`, constants
  `max_health_grunt`(30)/`max_health_ranged`(20)/`min_engage_range`(400)/`max_engage_range`(1200)/
  `min_telegraph_window`(0.4), formulas `attack_telegraph_window`/`noise_detection_radius`/
  `ranged_engage_distance_check`.
- Open risk carried forward: large-swarm (dozens of enemies) AIPerception/BT tick performance is
  unmeasured — flag before Production if Environmental Chain-Destruction scale grows. Also
  `docs/engine-reference/unreal/modules/navigation.md` is only verified against UE5.7, not the
  pinned 5.8 — re-verify against real 5.8 headers at implementation time.

**Key decisions baked into Health/Damage Core (carry forward when designing dependents):**
- Single damage entry point (`ApplyDamage`/GameplayEffect) for all sources — no separate paths.
- No generic Defense/Armor stat in MVP scope — only a boolean shield/no-shield intercept hook
  (owned by future Enemy Elite Shield GDD) and a `bBypassDefense` true-damage flag.
- Two GameplayTags this doc owns and gates access to: `State.Invulnerable` (100% damage block —
  Dash/Evasion, Status Effect, Core Extraction Execution will grant it) and `State.Executable`
  (F-key execution eligibility — **only** Dash/Evasion just-dodge and Enemy Elite Shield break
  may grant it; Core Extraction Execution only consumes it via `TryExecute()`). Both tags are
  reference-counted (loose tag count), not plain booleans — two grantors won't stomp each other.
  `bBypassDefense` bypasses shields, never bypasses `State.Invulnerable` — kept those two
  concepts strictly separate to avoid overdrive-tier damage trivializing i-frames.
- `OnHealthPercentCrossed(threshold)` event exposed now specifically because Boss Phase (later
  in the design order) will need it for phase-transition triggers.
- No player HP regen; player death = instant checkpoint respawn (no game-over screen), exact
  checkpoint mechanism deferred to a not-yet-designed Persistence system.
- **Cross-doc gap found and fixed**: `systems-index.md`'s Dependency Map was missing
  Dash/Evasion → Health/Damage Core (Dash/Evasion consumes the two tags above). Added it to
  both the enumeration table and the Core Layer dependency list.
- Player Movement carryovers still open: `MovementLocked` flag ownership (Status Effect only),
  Camera System mutual dependency, and the missing Progression system for double-jump unlock —
  none of these blocked Health/Damage Core, still open for whoever designs those systems.

**To resume in a new session:**
1. **Sonnet**: Read `sonnet-prompt.md` and begin GDD review and tutorial design.
2. **Antigravity**: Initialize Unreal C++ project structure and write `0001-player-movement-and-gas-core.md` ADR.

## What changed this session (enemy-ai-base.md review pass)
- Reviewed and **APPROVED** `design/gdd/enemy-ai-base.md` end-to-end (independent review session).
- Generated complete design review artifact [design_review_enemy_ai_base.md](file:///C:/Users/qjqmf/.gemini/antigravity-cli/brain/9287b538-86da-45d5-a5b0-82928d6b1ac3/design_review_enemy_ai_base.md).
- Updated `enemy-ai-base.md` status to `Approved`.
- Updated `systems-index.md` row #4 to `Approved`, incremented progress metrics (reviewed 4, approved 4, MVP designed 4/9).

## What changed this session (enemy-ai-base.md drafting, /design-system enemy-ai-base)
- Completed the 8-section GDD for Enemy AI (base) in [enemy-ai-base.md](file:///D:/moon-fragment-hunt/design/gdd/enemy-ai-base.md), plus UI Requirements and Open Questions. Solo review mode — no specialist agents (systems-designer, ai-programmer, qa-lead, art-director) consulted; needs manual review before Production.
- MVP archetype scope: Grunt (melee, 30 HP) + Ranged (20 HP, 400–1200uu engage band).
- Defined the anim-driven attack telegraph contract (`OnAttackTelegraphed`/`OnAttackCommitted`, min 0.4s window) that Dash/Evasion (not yet designed) must consume for just-dodge timing — this is the key interface this GDD hands off to the next system in the design order.
- Updated [systems-index.md](file:///D:/moon-fragment-hunt/design/gdd/systems-index.md): row #4 → "Designed (pending review)", started/MVP-designed counts incremented to 4.
- Updated [entities.yaml](file:///D:/moon-fragment-hunt/design/registry/entities.yaml): added `grunt`/`ranged` entities, 5 new constants, 3 new formulas.
- Did not run `/design-review` in this session (by design — must run in a fresh session per protocol).

## What changed this session (camera-system-base.md review pass)
- Reviewed and **APPROVED** `design/gdd/camera-system-base.md` end-to-end (independent review session).
- Generated complete design review artifact [design_review_camera_system_base.md](file:///C:/Users/qjqmf/.gemini/antigravity-cli/brain/c423aba1-458b-4f03-b7a1-016fed13de9e/design_review_camera_system_base.md).
- Updated `camera-system-base.md` status to `Approved`.
- Updated `systems-index.md` row #2 to `Approved`, incremented progress metrics (reviewed 3, approved 3, MVP designed 3/9).

## What changed this session (camera-system-base GDD drafting)
- Completed the 8-section GDD for the Camera System (base) in [camera-system-base.md](file:///D:/moon-fragment-hunt/design/gdd/camera-system-base.md).
- Solved the mutual dependency with Player Movement (camera-relative input, controller rotation alignment).
- Enforced rules and edge cases for viewport collision, particularly ignoring destructible geometry fragments to avoid camera snapping jitter.
- Defined tuning knobs (spring arm length, lag speeds, FOV parameters) and QA acceptance criteria.
- Updated [systems-index.md](file:///D:/moon-fragment-hunt/design/gdd/systems-index.md) status to Draft and incremented started design docs count to 3.

## What changed this session (health-damage-core.md review pass)
- Reviewed and **APPROVED** `design/gdd/health-damage-core.md` end-to-end (independent review session).
- Generated complete design review artifact [design_review_health_damage_core.md](file:///C:/Users/qjqmf/.gemini/antigravity-cli/brain/2877398e-99d2-47a1-86e2-5facfecb8b31/design_review_health_damage_core.md).
- Updated `health-damage-core.md` status to `Approved`.
- Updated `systems-index.md` row #3 to `Approved`, incremented progress metrics (reviewed 2, approved 2).
- Resolved a dependency map gap: updated `Super Armor / CC Interrupt` in `systems-index.md` to show its dependency on `Health/Damage Core`.

## What changed this session (housekeeping, prior session)
- Completed `/design-review` for [player-movement.md](file:///D:/moon-fragment-hunt/design/gdd/player-movement.md). Created [design_review_player_movement.md](file:///C:/Users/qjqmf/.gemini/antigravity-cli/brain/b627c449-b2fc-46cd-87b3-a87ac137d6d7/design_review_player_movement.md) with Gemini (Antigravity Design Review Agent) reviewer stamp.
- Updated status headers in [player-movement.md](file:///D:/moon-fragment-hunt/design/gdd/player-movement.md) and [systems-index.md](file:///D:/moon-fragment-hunt/design/gdd/systems-index.md) to reflect `Approved` status.
- `production/session-state/plan.md` still holds an **unrelated, unfinished** checklist (Enemy BP tree: BP_Enemy_Base done, BP_Spider/BP_SpiderKing/player-extension/map-placement not done). Left untouched on purpose — not part of this thread, don't overwrite or treat as done.

## Progress so far (this thread)
- Reviewed the full DDD Expansion concept doc (Blood Moon Overdrive, Spell Weaving, environment chain-destruction, Core Extraction execution, Arena Morphing boss finale) — assumed **solo PC, no multiplayer**.
- Completed Player Movement GDD design review and marked it as APPROVED.
- UE5.8 feasibility pass on all 6 systems: 5 are standard/well-supported patterns (GAS, Chaos Destruction, Niagara, Enhanced Input). Arena Morphing was the one open risk (Nanite + Geometry Collection interaction, undocumented in our engine-reference snapshots).
- Ran a mid-production spike to close that risk (see below) — **CONFIRMED YES**, compatible.
- Set up `unreal-mcp` direct editor scripting this session: added `AllToolsets` plugin to
  `Moon.uproject` (was only `MCPClientToolset` + `ModelContextProtocol` before — those alone
  only exposed a trivial `AgentSkillToolset`). After an editor restart, ~20 toolsets became
  available (scene/actor/asset/static-mesh/Dataflow/etc.) — this unlocks scripted editor
  control for future sessions too, not just this spike.

## Spike result (2026-07-16, concluded)

**CONFIRMED YES**: UE5.8 Geometry Collection supports Nanite per-fragment — does NOT force
non-Nanite fallback. Verified via `unreal-mcp` scripting: created `/Game/_Spikes/GC_SpikeFloor`
from a Nanite-enabled cube using the `DF_GC_Template_StaticMesh` Dataflow template, fractured to
38 pieces (Voronoi), confirmed `enableNanite: true` + `bEnableNaniteFallback: false` +
`LogStaticMesh: NaniteBuild` in the output log at bake time. Visually confirmed fractured seams
in viewport screenshot.

Gotcha for future reference: the Dataflow graph's "Evaluate" toolbar button does NOT reliably
trigger the bake when driven headlessly — the bake actually committed when the asset editor tab
was **closed** (temp Dataflow cache → real asset). Not an issue for normal interactive use (Fracture
Mode UI still works normally); only came up because of headless MCP scripting.

**Still open**: 60fps perf budget under full production load (40+ pieces + 5 rising platforms +
player air-dash simultaneously) — not measured this session. Follow up before committing Arena
Morphing to production if scale grows significantly beyond this test.

Full writeup: `prototypes/arena-morphing-spike-2026-07-16/SPIKE-NOTE.md`

## Open questions
- Perf budget test (see above) — the only remaining unknown for Arena Morphing feasibility.

<!-- CONSISTENCY-CHECK: 2026-07-17 | GDDs checked: 7 | Conflicts found: 0 | Scope: combo-tension-gauge.md 추가 반영 -->

## Session Extract — /architecture-review 2026-07-18
- Verdict: FAIL
- Requirements: 74 total — 9 covered, 6 partial, 59 gaps
- New TR-IDs registered: 74 (tr-registry.yaml populated, version 2)
- GDD revision flags: None
- Top ADR gaps: Camera System (Core), Enemy AI (Core), Dash/Evasion (Core)
- Blocking: duplicate ADR-0002 number; all 3 ADRs still Proposed; checkpoint→ADR-0001 OnDeath forward-reference
- Report: docs/architecture/architecture-review-2026-07-18.md

## Session Extract — Blackhole GAS slice: compile + PIE verification (Claude Code, 2026-07-20, continued)

**Status: IN PROGRESS, mid-verification, PIE currently running.** Picks up directly from the
"GDD header sync + Blackhole GAS slice" extract below (same 10 files, now compiled).

**Compile — done.** UBT full build succeeded (editor closed, `Build.bat MoonEditor Win64
Development`). Editor relaunched clean.

**Real bug #1 found + fixed (code, not yet committed)**: `GE_SpellCooldown_Blackhole.cpp`
called `AddComponent<UTargetTagsGameplayEffectComponent>()` directly in its constructor — crashed
the editor on both Live Coding reload AND a cold boot with:
`Fatal error: NewObject with empty name can't be used to create default subobjects (inside of
UObject derived class constructor)... Use ObjectInitializer.CreateDefaultSubobject<> instead.`
Root cause: UE5.8 CoreUObject added a new guard (`FObjectInitializer::AssertIfInConstructor`,
`UObjectGlobals.h` ~line 1935/1962/1978) that fires on exactly this call shape — breaks the
standard 5.3-5.7 "AddComponent in the GE constructor" pattern that `UGameplayEffect::AddComponent<T>()`
itself still uses internally (`NewObject(this, NAME_None, ...)`, `GameplayEffect.h:2501`). **Fix**:
moved the `AddComponent` + tag-grant call out of the constructor into a `PostInitProperties()`
override — by the time `PostInitProperties()` runs, `FObjectInitializer`'s destructor has already
decremented `IsInConstructor`/reset `ConstructedObject` (`UObjectGlobals.cpp` ~line 4051), so the
guard no longer fires. Verified fix compiles and boots clean (both Live Coding and cold UBT boot).
This is a genuine UE5.8 engine-version gotcha worth writing into
`docs/engine-reference/unreal/deprecated-apis.md` or `breaking-changes.md` — **not yet done**,
flag as a follow-up task (separate from the other GAS deprecated-API doc task already queued to a
background task this session).

**Real bug #2 found + fixed (code, not yet committed)**: `TargetDummy.cpp`'s `CapsuleComponent`
never had its collision profile set. A plain `AActor`-owned capsule defaults to
`OverlapAllDynamic` (Pawn channel → Overlap), unlike `ACharacter`'s auto-configured `Pawn` profile
(Pawn channel → Block). `MoonGameplayAbility_Spell_Blackhole`'s hit trace uses
`SweepSingleByChannel(..., ECC_Pawn, ...)`, which **only reports blocking hits** — so the dummy
was structurally untargetable regardless of aim. **Fix**: added
`CapsuleComponent->SetCollisionProfileName(TEXT("Pawn"))` in `ATargetDummy`'s constructor.
Verified on a fresh PIE spawn (post full-editor-restart, not just Live Coding — Live Coding patches
function bodies but does not re-run constructors on already-spawned instances, so mid-PIE-session
property pokes via `unreal-mcp` ObjectTools.set_properties on the raw `bodyInstance.collisionResponses`
array do NOT reliably take effect either — same caveat, don't try that shortcut again, restart PIE
after any constructor change instead).

**Test rig set up in `L_CombatTest` (saved to the level asset)**:
- `BP_MoonCharacter_C_0` (a placed instance, editor default `BP_MoonCharacter_Test` label) at
  world origin (0,0,96), yaw 0, `AutoPossessPlayer=Player0` (no GameMode default-pawn-class
  reliance — matches this project's known fragile-but-working pattern, see the "GameMode note"
  in the Track B takeover extract above; a `GM_MoonCombat` GameMode blueprint does now exist and
  auto-spawns though, discovered mid-session, didn't investigate further).
- `TargetDummy_0` at (300,0,96), yaw 180, directly in front of the player along +X — within the
  ability's 500uu trace range with roomto spare.
- The old debug-only EventGraph node `Keyboard 1 → AddTension(self, 30)` on `BP_MoonCharacter`
  (leftover from a much earlier session, see Track A/B history above) was **deleted** — it was
  bound to the same key ("One") as the real `IA_Spell_Blackhole` input action and would have
  contaminated the Tension reading (0+30+70 clamped to 100 instead of the real 0→70).

**Verified so far via live PIE + `unreal-mcp`**: with `LogAbilitySystem` verbosity set to
`Verbose`, confirmed the full activation path works — `TryActivateAbilityByTag` → engine's own
`LogAbilitySystem` lines show `Activated [12] MoonGameplayAbility_Spell_Blackhole_0` on a clean
first press, `Ended` immediately after (single-shot, no dangling active instance), and correctly
`could not be activated due to Cooldown (Cooldown.Spell.Blackhole)` on every repeat while the key
auto-repeats/cooldown is up. Mana cost math checked out once already mid-session (100→30, then
partial regen visible at 8/s while investigating). **Not yet re-verified end-to-end with the
collision fix in place** — PIE was just restarted fresh (constructor fix needs a real actor
respawn, not a Live Coding patch) with clean baseline (Mana 100, Tension 0, Health 100/100 both
sides, no active tags) right as this session paused. **This is the very next thing to do on
resume**: ask user to press "1" once (mouse untouched — Enhanced Input mouse-look was observed to
silently re-rotate the player's ControlRotation away from facing the dummy between an aim-fix and
the next keypress in this same session, more than once; if that happens again, recompute the
dummy's position along the player's *current* actual forward vector rather than trying to force
the player's rotation, which doesn't stick), then check via
`GASToolsets.AbilitySystemInspectorToolset`: caster Mana (expect 100→30, allow for regen creep if
any delay before checking), `Cooldown.Spell.Blackhole` tag present, target Health (expect
100→75, PlaceholderDamage=25 is not a real balance number), caster TensionGauge (expect 0→70,
only fires on-hit per spell-casting-base.md semantics).

**Not committed yet**: both bug fixes above (`GE_SpellCooldown_Blackhole.{h,cpp}`,
`TargetDummy.cpp`) plus the two debug `UE_LOG` lines added to `MoonCharacterBase.cpp`
(`TryActivateAbilityByTag`) and `MoonGameplayAbility_Spell.cpp` (`CanActivateAbility`,
`CheckAndConsumeSpellCastLimit` rejection) for diagnosing the above — consider whether to keep
those logs (they're low-noise and match the file's existing `[MoonDebug]` convention) or strip them
before committing. The whole `Moon/Content/Moon/` and `Moon/Source/Moon/` tree is still fully
untracked in git (first time either would ever be committed) — still needs the user's commit-scope
confirmation flagged in the earlier Track B takeover extract before sweeping it in.

**Real bug #3 found (not a code bug — scene/ability-geometry mismatch, root cause of every whiff
this session)**: `MoonGameplayAbility_Spell_Blackhole`'s hit trace (`SweepSingleByChannel`,
`TraceRadius=100`) starts from `AvatarActor->GetActorLocation()` — the capsule's **center**, not
an eye-level offset. Both `Floor_0` and the arena platform `StaticMeshActor_2` (spans roughly
x:-1380..620, y:-1200..800, z:-50..50 — this is the actual ground surface characters stand on in
this map, `BlockAllDynamic`/`BlockAll` profiles, both block Pawn) sit close enough beneath any
standing character that a **100uu-radius** sweep centered on the capsule's origin dips ~12uu below
the character's own feet and self-hits the ground the caster is standing on, before the sweep ever
reaches a target a few hundred uu away. This is **not** fixable by repositioning the target or
fixing aim — every attempt this session whiffed for this reason (confirmed via `get_actor_bounds` +
`bodyInstance.collisionProfileName` on both floor objects), not misaim (rotation/position were
independently confirmed correct one of the times). Real fix belongs in the ability code (raise
`TraceStart`'s Z by roughly the caster's capsule half-height, or shrink `TraceRadius` below capsule
half-height) — **not done**, flag as a 4th follow-up alongside the engine-version doc task. Given
this doc's Smoke-test-only header caveat, whether to fix trace geometry at all is a design call, not
just a bug fix — ask the user before touching `MoonGameplayAbility_Spell_Blackhole.{h,cpp}` again.

**Workaround applied for this test session only (PIE-live only, not saved to the level asset,
will NOT survive a PIE restart)**: set `BP_MoonCharacter_C_0.CharMoveComp.GravityScale = 0` and
teleported both `BP_MoonCharacter_C_0` and `TargetDummy_0` to (0,0,1000)/(300,0,1000) — well clear
of all known level geometry (confirmed via `get_actor_bounds` on `Floor_0`/`StaticMeshActor_0`/
`StaticMeshActor_2`) so the sweep has nothing beneath it to self-hit. Player facing yaw 0 (both
actor rotation and `PlayerController_0`'s rotation set explicitly), dummy facing yaw 180, both at
same Z, 300uu apart (within the 500uu trace range). Cooldown confirmed clear
(`GetActiveTags` → `[]`) right before this save point.

**This IS the exact resume point** — the very next action on resume, before anything else: ask
user to press "1" once (mouse untouched — same caution as above, mouse-look was seen to silently
re-rotate `ControlRotation` between an aim-fix and the next keypress more than once this session),
then check via `GASToolsets.AbilitySystemInspectorToolset` on both actors (`BP_MoonCharacter_C_0`
Mana/TensionGauge, `TargetDummy_0` Health) — if this finally lands a hit, the acceptance-criteria
numbers to confirm are: Mana 100→30 (regen at 8/s eats into this fast, so check promptly — don't
worry if it's only partially recovered by the time you check, 100-70+regen*elapsed_seconds is
still a pass), `Cooldown.Spell.Blackhole` tag present via `GetActiveTags`, target Health 100→75
(`PlaceholderDamage`=25 is a smoke-test magnitude, not a real balance number — health-damage-core.md
doesn't own this number), caster TensionGauge 0→70 (`ManaCostForTension`, on-hit only per
spell-casting-base.md semantics — ability code path confirms it only calls
`AddTensionFromSpellHit` inside the `bHit` branch, never on whiff).

**User flagged mid-session**: approaching an 80%-of-5-hour usage-window limit; when hit, save
everything and end. This extract is that save point — if the session ends here, resume by reading
from "**This IS the exact resume point**" above.

## Session Extract — GDD header sync + Blackhole GAS slice (Claude Code, 2026-07-20)

**GDD header/index sync (done, committed+pushed as `dfaa6e6`)**
- Verified `systems-index.md` status column vs each GDD's own header — found 2 real mismatches
  (`combo-tension-gauge.md` said "In Design", `combat-hud.md` said "Designed (pending review)" even
  though the 2026-07-18 cross-review already found both NEEDS REVISION). Fixed both headers to state
  the NEEDS REVISION verdict + point at `gdd-cross-review-2026-07-18.md`.
- Removed stale `(미설계)` tags for Combo/Tension Gauge / Luna Overdrive left over in
  `spell-casting-base.md`, `health-damage-core.md`, `dash-evasion.md`, `combo-tension-gauge.md` (both
  systems have authored GDDs now — this was cross-review finding W1, previously unaddressed).
- `Core Extraction Execution (미설계)` mentions were left alone — that system genuinely isn't started.

**Minimal Blackhole spell GAS slice (code written, NOT yet compiled/wired/tested — uncommitted)**
- Goal: user wants a cheap/fast PIE-driven check via `unreal-mcp` that the just-reviewed GDD math
  (spell-casting-base.md, health-damage-core.md, combo-tension-gauge.md) actually holds up, before
  sinking more design time into them. Scoped to Blackhole only (not Fire/Lightning) to keep it small.
- `ue-gas-specialist` subagent wrote 10 new files + 1 config edit (git status confirms, not yet
  committed):
  - `Moon/Source/Moon/GAS/Effects/GE_SpellCost_Blackhole.{h,cpp}` — Instant, Mana -70
  - `Moon/Source/Moon/GAS/Effects/GE_SpellCooldown_Blackhole.{h,cpp}` — Duration 6.0s, grants
    `Cooldown.Spell.Blackhole`
  - `Moon/Source/Moon/GAS/Effects/GE_Damage_Instant.{h,cpp}` — Instant, SetByCaller `Data.Damage`,
    the shared single-entry-point damage GE per Health/Damage Core Rule 1 (reusable beyond Blackhole)
  - `Moon/Source/Moon/GAS/MoonGameplayAbility_Spell_Blackhole.{h,cpp}` — commits cost+cooldown
    (respecting the existing `CostBypass.Active`/Luna-Overdrive-bypass override in
    `MoonGameplayAbility_Spell`), forward sphere trace (500uu/100uu, smoke-test only), applies the
    damage GE + `AddTensionFromSpellHit(70)` on hit only (not on whiff/cast) per spell-casting-base.md
    OnSpellHit semantics, always `EndAbility`s. `PlaceholderDamage=25` — GDDs never define an actual
    spell damage number (that's unowned/Spell-Weaving territory), so this is a smoke-test magnitude only.
  - `Moon/Source/Moon/Character/TargetDummy.{h,cpp}` — minimal ASC+AttributeSet-only actor to receive
    the test hit, no mesh/AI.
  - `Moon/Config/DefaultGameplayTags.ini` — added `Cooldown.Spell.Blackhole`, `Data.Damage`.
- **UE5.8 API finding (verified against the actual installed engine, not training data)**: confirmed
  at `C:\Program Files\Epic Games\UE_5.8\Engine\Plugins\Runtime\GameplayAbilities\...` —
  `UGameplayEffect::InheritableOwnedTagsContainer` is deprecated since 5.3; granted tags now go through
  `UTargetTagsGameplayEffectComponent::SetAndApplyTargetTagChanges()` (used in
  `GE_SpellCooldown_Blackhole`'s constructor). Also `UGameplayAbility::AbilityTags` is deprecated
  since 5.5 in favor of constructor-only `SetAssetTags()` (used in the Blackhole ability). A background
  task (`task_a3fdf7fc`, running independently, started by user) is writing this up properly into
  `docs/engine-reference/unreal/deprecated-apis.md` — don't duplicate that.
- **NOT done yet (needs the Unreal Editor, which is why this can't be finished from a pure-code
  session)**: compile, add `MoonGameplayAbility_Spell_Blackhole` to `BP_MoonCharacter`'s
  `DefaultAbilities` array, place a `TargetDummy` (with `DefaultAttributesEffect` set to whatever GE
  `BP_MoonCharacter` already uses for Health=100 init) into `L_CombatTest`, then PIE-test via
  `unreal-mcp` — this project's prior sessions used
  `GASToolsets.AbilitySystemInspectorToolset.GetAttributeValues` on the live pawn/target to read
  attributes, and `StartPIE`/`BlueprintTools.write_graph_dsl`/`ObjectTools` for the editor-side setup
  (see the Track B takeover extract above for exact prior usage of these unreal-mcp toolset calls).
  Target numbers to confirm: Mana 100→30 on cast (spell-casting-base.md AC1), `Cooldown.Spell.Blackhole`
  present for 6s, target Health 100→75 (health-damage-core.md AC1 shape, damage magnitude is the
  placeholder not a real value), caster Tension 0→70 (combo-tension-gauge.md hit-accrual, only on hit).
- `unreal-mcp` (`.mcp.json`, `http://127.0.0.1:8000/mcp`) was not connected in the Claude Code session
  that did this implementation work — the editor wasn't running yet when that session started. User is
  opening a new conversation now that the editor + MCP server are up, to continue with the in-editor
  steps above.
