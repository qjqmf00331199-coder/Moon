# Tutorial Flow — Moon Fragment Hunt

> **Status**: Draft (initial authoring pass — not yet reviewed)
> **Author**: user + Claude Code (solo design mode)
> **Last Updated**: 2026-07-18
> **Source Docs**: design/gdd/game-concept.md, design/gdd/systems-index.md, design/gdd/player-movement.md,
> design/gdd/camera-system-base.md, design/gdd/spell-casting-base.md, design/gdd/dash-evasion.md,
> design/gdd/combo-tension-gauge.md, design/gdd/luna-overdrive.md, design/gdd/combat-hud.md,
> design/gdd/health-damage-core.md
> **Scope Note**: This document is not a system GDD (no single owning gameplay system produces
> "the tutorial") — it is a UX/onboarding flow that sequences existing, already-approved MVP
> systems into a first-time-player learning path. It does not invent new mechanics, rules, or
> tuning values; every number cited below is pulled directly from the cited GDD's Tuning Knobs.

## 1. Overview

The tutorial teaches the five MVP systems that make up the game's core Dopamine Driven Design
tension curve (game-concept.md §1) in the order the curve itself demands: you cannot feel the
payoff of Luna Overdrive without first feeling the mana/cooldown pressure it removes, and you
cannot feel that pressure without first being able to move and cast. The flow is one continuous
combat encounter (not a menu-driven series of checkboxes) split into 5 stages, each introducing
exactly one new system while the previous stage's system stays live and required. No stage is
skippable by walking past it — each stage's exit gate is a mechanical proof that the player used
the mechanic, not a scripted trigger volume the player can wander through unaware.

## 2. Design Principles

1. **Teach by making the mechanic necessary, not by showing text.** Every stage's "lesson" is a
   combat situation that is meaningfully harder to clear without the mechanic than with it —
   not a popup that gets dismissed.
2. **Never let a failed attempt become a dead end.** Every stage has an explicit rescue path
   (Section 4) — a player who cannot execute the mechanic within a generous window is still
   walked to competence, never soft-locked or forced to reset the tutorial.
3. **No new tuning values.** Every number in this document is the system's already-approved
   default value (cited inline) — the tutorial does not get an easier version of the mechanics;
   it gets the real ones, in a controlled space.
4. **HUD is taught implicitly.** Combat HUD (combat-hud.md) is 100% read-only (its Core Rule 1) —
   this flow never asks the player to "look at the HUD," it puts the player in situations where
   glancing at the relevant widget is the only way to make a good decision (e.g. Stage 3 forces a
   risk/reward call that only the tension gauge widget, W6, can inform).
5. **The capstone (Overdrive) must fire — skill should decide only *when*, not *whether*.**
   Section 3.5 details the accumulation-guarantee mechanism that prevents Stage 4 from becoming
   an unbounded grind if the player is under-performing.

## 3. Learning Sequence

### Stage 1 — Movement & Camera

**Systems introduced**: Player Movement (player-movement.md), Camera System (base)
(camera-system-base.md).

**Setup**: Player spawns in an enclosed arena segment with no enemies. A single destructible
prop (or lit waypoint) sits across a small gap that requires a jump, teaching camera-relative
movement (Player Movement Core Rule 1) and the single jump (Core Rule 4) without combat pressure.

**Teaching beat**: Player must walk (WASD/stick, camera-relative), rotate the camera to see
around the enclosed space, and jump the gap. `MaxWalkSpeed` = 600 uu/s, `JumpZVelocity` = 600
uu/s (player-movement.md Tuning Knobs) — both stock values, no tutorial-only tuning.

**Exit gate**: Player reaches a marked point on the far side of the gap. This requires an actual
jump input — the gap width is set so walking off the ledge (no jump) drops the player into a
non-lethal recovery pit that respawns them at the ledge (see Rescue, 4.1), not into failure state.

**Why this order**: Every later stage assumes the player can move and see — this is the only
stage with no failure-recovery *loop* needed because there is no time pressure and no enemy.

---

### Stage 2 — Spell Casting

**Systems introduced**: Spell Casting (base) (spell-casting-base.md).

**Setup**: Player enters a small arena with 2–3 stationary or slow-moving low-threat enemies
(reuses Enemy AI (base) grunts, no new enemy type). All three spell slots (Blackhole/Fire/
Lightning) are already unlocked — this game has no unlock-gated loadout (spell-casting-base.md
Core Rule 1: fixed 3-slot loadout, no swapping).

**Teaching beat**: Player must cast each of the 3 elements at least once to clear the enemies —
the encounter is tuned so that using only one element (e.g. spamming Fire) is *possible* but
slower, gently nudging toward trying all three without forbidding a single-element clear.
Mana pool is the real default (`MaxMana`=100, `ManaCost` Blackhole=70/Fire=25/Lightning=25,
`ManaRegenRate`=8/s — spell-casting-base.md Tuning Knobs), so the player also feels the real
"Blackhole commits ~70% of your mana" weight (Player Fantasy, spell-casting-base.md) for the
first time here.

**Exit gate**: All enemies in the sub-arena are defeated AND the tutorial has observed at least
one successful cast of each of the 3 elements (tracked via the same `OnSpellCast(Element)` event
combo-tension-gauge.md already subscribes to downstream — no new event needed).

**Why this order**: Placed before Dash/Evasion because Just-Dodge (Stage 3) is defined relative
to *enemy attacks*, and enemies only become a real threat once the player can already fight back;
introducing dash-as-pure-movement before dash-as-defense would teach the wrong mental model.

---

### Stage 3 — Dash / Just-Dodge

**Systems introduced**: Dash/Evasion (dash-evasion.md).

**Setup**: A single telegraphing enemy (or a scripted "practice dummy" reusing Enemy AI's
`OnAttackTelegraphed`/`OnAttackCommitted` delegates, dash-evasion.md Core Rule 6) repeatedly
performs one clearly-readable attack. The player is given free dash charges to experiment before
the graded rep.

**Teaching beat, two-part**:
- **3a — Dash as movement/defense**: Player must dash at least once (any timing) to reposition —
  teaches the charge system (`MaxCharges`=2, `RechargeRate`=2.0s, dash-evasion.md Tuning Knobs)
  and the invulnerability window (`DashInvulnDuration`=0.2s) without requiring precision.
- **3b — Just-Dodge**: Player must land at least one dash inside the real `JustDodgeWindow`
  (0.2s before the attack's commit time, dash-evasion.md Formulas) while in the attack's range,
  producing `State.Executable` on the dummy. This is the first time the player sees the F-key
  execute prompt (Combat HUD W7) light up — the tutorial does not explain the prompt in text; the
  prompt's own blink (combat-hud.md Tuning Knobs, `ExecutePromptBlinkRate`=2.0Hz) plus the
  `executable_duration`=3.0s window (dash-evasion.md) is what teaches "you have a few seconds to
  press F."
- **Execution**: Player presses F within the 3.0s window to complete the kill (Core Extraction
  Execution is not yet designed per systems-index.md — the tutorial's dummy execution uses the
  same assumed `State.Executable`-tag interface Combat HUD already assumes, Rule 6 of
  combat-hud.md; no new interface invented here).

**Exit gate**: One successful Just-Dodge + execution completes the stage.

**Why this order**: Just-Dodge is the entry point to the game's other core loop (execution →
mana refund, per game-concept.md §5) and is a precondition for the tension gauge's second income
source (combo-tension-gauge.md Core Rule 2 — Just-Dodge bonus via `State.Executable`), so it must
land before Stage 4 asks the player to *feel* the gauge.

---

### Stage 4 — Combo/Tension Gauge feel

**Systems introduced**: Combo/Tension Gauge (combo-tension-gauge.md), Combat HUD's gauge widget
(W6).

**Setup**: A mixed encounter (4–6 grunts) that cannot be cleared instantly — long enough for the
gauge to visibly rise, dip on a deliberate "bait" enemy attack, and rise again.

**Teaching beat**: This stage has no new input to learn — its entire purpose is to make the
player *look at* the tension gauge widget (W6) and connect their actions to it:
- Landing a Blackhole hit visibly jumps the gauge by 70 (`TensionGain` formula,
  combo-tension-gauge.md Formulas, `ManaCost[Blackhole]`=70 × `TensionGainCoefficient`=1.0).
- Taking a hit visibly *drops* the gauge by 20% (`DamagePenaltyPercent`=0.20) — the tutorial
  arena includes one attack pattern that is easy to avoid but tempting to tank, so the player
  feels the risk/reward tradeoff described in combo-tension-gauge.md Player Fantasy at least once
  before Stage 5, rather than discovering the penalty for the first time during the capstone.
- Standing still and not attacking for a few seconds shows the gauge visibly decay
  (`TensionDecayGracePeriod`=3.0s grace, then `TensionDecayRatePerSec`=10/s) — teaches "this is a
  pressure gauge, not a savings account."

**Exit gate**: Gauge reaches the `TensionChargedHighlightThreshold` (0.90 × 100 = 90,
combat-hud.md Formulas) at least once, i.e. the widget visibly enters its "Charged" pulse state.
This does **not** require reaching Max/100 — that is reserved for Stage 5's actual Overdrive
trigger, so Stage 4's gate is deliberately short of full to keep the stages mechanically distinct.

**Why this order**: The gauge is diegetically silent — it never tells the player what it's for
until they've seen Luna Overdrive at least once. This stage exists solely so Stage 5's payoff
lands as "oh, THAT'S what the gauge was building to," not as an unexplained number that suddenly
matters.

---

### Stage 5 — Luna Overdrive Awakening

**Systems introduced**: Luna Overdrive (luna-overdrive.md), full Combat HUD Overdrive state
(combat-hud.md Rule 7).

**Setup**: The tutorial's largest encounter (a small wave, or one durable "sub-boss" grunt with
inflated health) — deliberately sized so that a competent Stage 2–4 player crosses the
`TensionGaugeMax` (100) threshold organically within the fight, without needing to grind trash
mobs.

**Teaching beat**: The player fights normally; when the gauge hits 100, `OnOverdriveTriggered`
fires (combo-tension-gauge.md Core Rule 5) and the world flips to Blood Moon state
(luna-overdrive.md Overview) — crimson tint, HUD accent flip (combat-hud.md Rule 7), mana bar to
∞, all cooldown overlays cleared. The player is given explicit permission (via the encounter's
enemy density) to *empty every cooldown at once* — the tutorial's only goal here is for the
player to personally feel the "nothing to save, nothing to wait for" release described in
luna-overdrive.md Player Fantasy, for the full real `OverdriveDuration` = 10.0s.

**Exit gate**: `OnOverdriveEnded(Expired)` fires (natural 10-second expiry, not death) — the
stage does not require the encounter to be cleared *during* the window, only that the player
experienced a full, uninterrupted Overdrive window once. Tutorial completes on this event.

**Why this is last**: This is the top of the tension curve (game-concept.md §1, step 4) — every
prior stage exists to make this payoff legible. Ending the tutorial here (rather than tacking on
a "now defeat the boss" requirement) keeps the tutorial's own pacing honest to the "sharp rise,
sharp fall" curve it's teaching — it doesn't linger past the peak.

## 4. Failure / Rescue Logic

No stage can hard-lock the player. Each rescue below is a widening of opportunity, never a
difficulty reduction of the underlying mechanic (Design Principle 3).

| Stage | Failure Mode | Rescue Logic |
|---|---|---|
| 1. Movement | Player walks off the gap without jumping | Non-lethal recovery volume respawns player at the ledge with no health/progress loss — not treated as a "death," no restart of Stage 1 |
| 2. Spell Casting | Player clears enemies using only 1–2 elements | Stage does not fail — but the tutorial's next enemy wave (Stage 3 setup) will not spawn until all 3 elements have been cast at least once; a low-key on-screen prompt (icon glow on the un-used slot(s), no text) nudges without blocking exploration |
| 2. Spell Casting | Player runs out of mana mid-fight | No special-case needed — `ManaRegenRate`=8/s (spell-casting-base.md default) means the player simply waits a few seconds; this **is** the intended lesson (mana is a rhythm gate, not a failure state — spell-casting-base.md Player Fantasy) |
| 3. Just-Dodge | Player dashes too early/late repeatedly (no successful Just-Dodge within ~4 attack reps) | Dummy's `JustDodgeWindow` is never widened (Design Principle 3 — no secret easy-mode tuning), but the **attack telegraph repeats indefinitely** with generous dash-charge regen between reps (charges fully refill between each telegraph cycle) — the player gets unlimited attempts, never a "you failed, moving on" fallback |
| 3. Just-Dodge | Player successfully Just-Dodges but misses the F-key window (`executable_duration`=3.0s) | `State.Executable` simply expires (dash-evasion.md Edge Cases — normal, defined behavior, not a bug) and the dummy's attack cycle repeats, offering a fresh Just-Dodge opportunity — no penalty beyond the lost rep |
| 4. Tension Gauge | Player avoids all damage and the "bait" attack, never seeing the penalty | Non-blocking — the exit gate only requires reaching the Charged threshold (90), which is achievable through pure offense; the damage-penalty lesson is a bonus if triggered, not a hard requirement (documented gap, see Section 6 Open Questions) |
| 4. Tension Gauge | Player's playstyle is too passive/slow and gauge keeps fully decaying before reaching 90 | Encounter's enemy count/spacing is tuned so decay (10/s after 3.0s grace) cannot outpace a merely-competent Stage 2 clear rate under normal play; if the tutorial telemetry ever shows this failing in practice, the fix is enemy density in this stage's encounter — never a change to the gauge's own decay tuning (Design Principle 3) |
| 5. Overdrive | Player's tension gauge is climbing too slowly to hit 100 within the encounter (under-performing player) | **Accumulation guarantee, not a difficulty crutch**: the encounter includes a fixed number of guaranteed `State.Executable` grants (scripted "free" Just-Dodge windows woven into the wave, identical in kind to Stage 3's — not a new mechanic) timed so that even a player who lands zero optional Blackhole hits will cross 100 before the wave's last enemy dies. This uses the same `JustDodgeTensionBonus`=20 (combo-tension-gauge.md) already taught in Stage 4 — no hidden bonus tension value is introduced |
| 5. Overdrive | (Fallback of last resort) Player somehow still has not triggered Overdrive by the time all scripted enemies are dead | Tutorial-only debug path: publish `OnOverdriveTriggered` directly, exactly as the test-harness stub luna-overdrive.md Acceptance Criteria already uses for its own unit tests (luna-overdrive.md AC1 preamble) — this is not a new trigger path invented for the tutorial, it reuses the system's own documented test entry point. Logged as a tutorial-design smell if it ever fires in practice (see Open Questions) |
| 5. Overdrive | Player dies mid-encounter before reaching Overdrive | Standard respawn (Health/Damage Core death handling) at the start of Stage 5's encounter — tension gauge resets to 0 on death (combo-tension-gauge.md Edge Cases) and the wave restarts; not treated as tutorial failure, simply a retry of the stage |

## 5. Skip / Accessibility

- No stage gates on reflex-only precision without a rescue path (Section 4) — this satisfies
  technical-preferences.md's baseline but the tutorial does not currently offer a difficulty
  toggle; see Open Questions.
- All new-mechanic cues are audio+visual (icon glow, HUD pulse, prompt blink), consistent with
  combat-hud.md's accessibility minimum ("Building/Decaying 및 크림슨 전환은 색상만으로 구분하지
  말 것") — this flow does not introduce any new color-only signal.
- No stage requires a specific input device — all inputs (WASD/stick, dash button, F-key/pad
  button) already have keyboard+gamepad equivalents per their owning GDDs; the tutorial adds no
  new bindings.

## 6. Dependencies

| System | Nature of dependency |
|---|---|
| Player Movement | Stage 1 exit gate; camera-relative movement underlies every later stage |
| Camera System (base) | Stage 1 — camera rotation |
| Spell Casting (base) | Stage 2 exit gate; mana/cooldown pressure re-felt in Stage 5's release |
| Enemy AI (base) | All enemy encounters (Stages 2–5) reuse existing grunt archetypes and telegraph delegates — no new enemy type authored here |
| Dash/Evasion | Stage 3 exit gate; Just-Dodge is the entry point to Stage 4's second tension income source |
| Combo/Tension Gauge | Stage 4 exit gate; silent build-up that Stage 5 pays off |
| Luna Overdrive | Stage 5 exit gate — the capstone |
| Combat HUD | Read-only throughout — every stage's "teaching" leans on a specific widget (W4 cooldowns in Stage 2, W7 execute prompt in Stage 3, W6 gauge in Stage 4, full Overdrive state in Stage 5) already specified in combat-hud.md Rule 2's widget inventory. No new widget required. |
| Health/Damage Core | Player death/respawn handling (Stage 5 death rescue), `State.Executable` tag plumbing (Stage 3) |
| Core Extraction Execution (not yet designed) | Stage 3/5 executions use the same assumed interface Combat HUD already assumes (combat-hud.md Rule 6) — when Core Extraction Execution is designed, this document's execution beats should be re-checked against the real interface, not the assumed one |

## 7. Open Questions

- **Owner: level-designer, Target: encounter-block-out** — Exact enemy counts/spacing per stage
  (Stage 2's "2–3 enemies," Stage 4's "4–6 grunts," Stage 5's wave size) are placeholder
  quantities consistent with the cited tuning values' math, not yet spatially blocked out or
  playtested. Needs a real arena layout pass.
- **Owner: game-designer, Target: playtest** — Stage 4's damage-penalty lesson (Section 4) is
  optional/bonus rather than gated — decide after first playtest whether this under-teaches the
  risk/reward half of the gauge's Player Fantasy, and if so, add a scripted forced-hit beat
  (same pattern as Stage 5's accumulation guarantee) rather than tuning the penalty itself.
- **Owner: qa-lead, Target: pre-Production** — The Stage 5 "debug trigger fallback" (Section 4)
  should almost never fire if the encounter is tuned correctly; recommend instrumenting it
  (a counter/log) during playtesting so a high fallback-trigger rate becomes a visible signal
  that the wave's guaranteed-tension design (not the underlying gauge tuning) needs revision.
- **Owner: ux-designer, Target: Pre-Production `/ux-design`** — Section 5 notes no difficulty
  toggle exists yet for the Just-Dodge precision requirement (Stage 3b) — accessibility-specialist
  input needed on whether a settings-level timing-assist option should exist project-wide (would
  affect dash-evasion.md's `JustDodgeWindow`, not just the tutorial) versus tutorial-local design.
- **Owner: game-designer, Target: Core Extraction Execution GDD design** — once that system is
  designed, re-validate Stage 3/5's execution beats against its real trigger interface (currently
  using Combat HUD's assumed `State.Executable`-tag interface per Section 6).
