# Cross-GDD Review Report

**Date**: 2026-07-18
**GDDs Reviewed**: 9 system GDDs (+ game-concept.md, systems-index.md)
**Systems Covered**: Player Movement, Camera System (base), Health/Damage Core, Enemy AI (base), Spell Casting (base), Dash/Evasion, Combo/Tension Gauge, Luna Overdrive, Combat HUD
**Registry baseline**: `design/registry/entities.yaml`

---

## Consistency Issues

### Blocking (must resolve before architecture begins)

🔴 **B1 — systems-index.md approval status is stale/false (3 of 9 GDDs are NOT actually approved)**

`systems-index.md`'s Progress Tracker claims "Design docs approved: 9" and "MVP systems designed: 9/9", and marks rows #9 (Combo/Tension Gauge), #11 (Luna Overdrive), #20 (Combat HUD) as `Status = Approved`. The GDDs' own headers say otherwise:
- `combo-tension-gauge.md` L3: **"Status: In Design"**
- `luna-overdrive.md` L3: **"Status: Designed (pending re-review — 2026-07-17 full design-review: NEEDS REVISION, 5 blocking items revised in-session)"**
- `combat-hud.md` L3: **"Status: Designed (pending review)"**

Only 6/9 (player-movement, camera-system-base, enemy-ai-base, dash-evasion, health-damage-core, spell-casting-base) actually self-report Approved. The index overstates MVP completion — a `/gate-check` or `/create-architecture` reading the tracker at face value would proceed on a false premise. **Fix the three rows + Progress Tracker (approved = 6, not 9) before treating Systems Design as closed.**

### Warnings (should resolve, but won't block)

⚠️ **W1 — Stale "미설계" (undesigned) markers for Combo/Tension Gauge and Luna Overdrive in 3 upstream GDDs**
`spell-casting-base.md` (Interactions, Dependencies table), `health-damage-core.md` (Interactions), `dash-evasion.md` (Interactions) all still label Combo/Tension Gauge and/or Luna Overdrive as "하류, 미설계" (downstream, undesigned) — both now have authored GDDs. Same update-lag pattern as B1.

⚠️ **W2 — Camera System ↔ Luna Overdrive (FOV) dependency undeclared on both sides + in the index**
Camera's Rule 7 / Formula 2 / AC8 consume Luna's `OnOverdriveStarted`/`OnOverdriveEnded` events (FOV 90°→100° swing) — a real data dependency. Neither Camera's nor Luna's Dependencies table lists the other; `systems-index.md` row #2 shows Camera depending only on Player Movement. Violates the dependency-bidirectionality rule.

⚠️ **W3 — Combat HUD ↔ Health/Damage Core & Spell Casting reciprocity asymmetry**
Combat HUD lists both as hard upstream dependencies; neither lists Combat HUD back in their own Dependencies tables (only inside UI Requirements prose) — inconsistent with the convention Dash/Evasion, Combo/Tension Gauge, and Luna Overdrive all follow (they do list Combat HUD as a downstream dependent).

⚠️ **W4 — Player Movement Rule 11 vs Camera Edge Case 5/AC9: contradictory look-input claim during execution cutscene**
Player Movement Core Rule 11: "플레이어 입력은 연출 중에도 죽지 않음" (input never dies during the cutscene). Camera Edge Case 5 / AC9.2: look input is explicitly blocked ("Ignore Look Input") during the execution sequence. Movement input claim survives; the blanket "input doesn't die" phrasing directly conflicts with Camera's look-input block. One-line fix: narrow Player Movement Rule 11 to *movement* input only.

⚠️ **W5 — Luna Overdrive Dependencies table still presents the cast-rate cap as an unresolved BLOCKING precondition**
Luna's Dependencies table still reads the cast-rate floor as an outstanding blocking cross-dependency on Spell Casting, but Luna's own Open Questions section marks it RESOLVED (spell-casting Rule 11, `max_casts_per_second`=20, applies regardless of `CostBypass.Active`). The interaction is now consistent — only the Dependencies-table wording is stale.

⚠️ **W6 — Combo/Tension Gauge conflates two grantors of `State.Executable` (latent)**
Combo Rule 2 awards the just-dodge tension bonus by listening to `OnTagAdded(State.Executable)`, framed as detecting just-dodge specifically. Health/Damage Core Rule 9 states `State.Executable` has **two** grantors: Dash/Evasion (just-dodge) **and** Enemy Elite Shield (shield break). The tension bonus will also fire on Elite Shield breaks once that system exists. Latent (Elite Shield is Alpha/undesigned) but the two-grantor contract is already committed.

⚠️ **W7 — Combo/Tension Gauge Rule 4 depends on an HDC interface HDC only tentatively exposes**
Combo Rule 4 (damage-taken tension penalty) hard-depends on `OnDamageApplied`, which Health/Damage Core only mentions as "추후" (future/unspecified) in its Interactions section — it isn't in HDC's formal UI Requirements delegate list. HDC should formally commit this interface or Combo should rebind to an existing exposed signal.

⚠️ **W8 — Registry `jump_air_time` output_range is stale vs player-movement.md**
Registry records `output_range: [0.8, 1.5]`; current player-movement.md gives 0.63s–2.04s plus a 0.5s–3.0s Joint Bound tuning knob (post-2026-07-16 review). Registry wasn't updated at revision time — same drift pattern the registry's own `air_control` comment already admits for a different constant.

---

## Game Design Issues

### Blocking

None found.

### Warnings

⚠️ **W9 — Refresh-chain damping lever defaults to a no-op, risking the core tension-curve pillar**
`OverdriveTensionGainMultiplier` (combo-tension-gauge Core Rule 7) defaults to **1.0 = zero damping**. Luna's own analysis shows the worst case: at `TensionGainCoefficient` Safe-Range top (1.5), Blackhole = 105 tension → every single cast refreshes Overdrive → `OverdriveDuration` becomes functionally meaningless. Even at default coefficient, dense packs can sustain Overdrive indefinitely via 2-3 hits. This directly undermines the concept's signature tension curve's step 5 ("급격한 하락" / sharp drop) — sustained Overdrive means the drop never lands. **Recommend**: default `OverdriveTensionGainMultiplier` below 1.0 (e.g. 0.3-0.5), or formally designate the refresh-chain the #1 playtest priority before shipping 1.0 unexamined.

⚠️ **W10 — Player attention budget exceeds ~4 concurrent trackers during core loop**
HP, dash charges + recharge, tension gauge state, mana + 3 cooldowns, enemy telegraphs/Executable prompts, and (when active) Overdrive remaining-time — exceeds the comfortable 3-4 limit. Mitigations are strong and deliberate (HUD designed for 0.2s peripheral glances; mana/cooldown trackers collapse to ∞/cleared during Overdrive, removing load exactly when spectacle peaks). **Recommend**: advisory only — validate in first playtest whether the tension risk-reward decision survives alongside dodge timing, or whether players tunnel on one and ignore the gauge.

⚠️ **W11 — Base spell RawDamage (assumed 25) has no owning GDD**
Enemy HP tuning (Grunt 30 / Ranged 20 → "dies in 1-2 hits") and Health/Damage Core's own examples silently assume base spell RawDamage=25, but spell-casting-base.md explicitly disclaims owning individual-spell damage balance. The difficulty-curve numbers currently "work" only because of an unstated assumed value. **Recommend**: designate an owning GDD for the three base spells' RawDamage before treating enemy HP tuning as validated.

**Checks confirmed clean**: Progression Loop Competition (3a — Mana/Cooldown and Tension Gauge operate on separate axes, don't compete), Economic Loop Analysis (3d — mana refund is skill-gated behind the full execution chain, not a runaway loop, sinks are adequate), Pillar Alignment (3f — all 9 systems explicitly declare and serve "Dopamine Driven Design"), Player Fantasy Coherence (3g — one consistent aggressive risk-taking spellcaster identity across all 9 docs, the one near-conflict with mana gating is explicitly reconciled in spell-casting-base's own text). Difficulty Curve (3e) is premature to fully assess (Boss Phase/Elite Shield not designed yet) but the concrete numbers that do exist are internally coherent aside from W11.

---

## Cross-System Scenario Issues

Scenarios walked: 4
1. Spell-weave → Tension fill → Overdrive trigger in dense pack (spell-casting-base → combo-tension-gauge → luna-overdrive)
2. Just-dodge → Executable → F-key execution → mana refund (dash-evasion → health-damage-core → spell-casting-base)
3. F-key execution cutscene camera lock (player-movement → camera-system-base)
4. Player death during active Overdrive (health-damage-core → luna-overdrive → combo-tension-gauge)

#### Warnings
⚠️ Scenario 1 — Combo/Tension Gauge, Luna Overdrive, Spell Casting (base)
Same root cause as W9: the refresh-chain surfaces exactly at this interaction boundary, not just in isolated GDD text — chaining weave→tension→trigger repeatedly with the default 1.0 multiplier can keep re-triggering Overdrive without the intended sharp drop ever landing.

⚠️ Scenario 3 — Player Movement, Camera System (base)
Same root cause as W4: at the exact moment the execution cutscene starts, Player Movement's "input never dies" framing and Camera's explicit look-input block read as contradictory messaging to an implementer. Confirmed this is a real interaction-boundary issue, not just a wording nit in isolation.

#### Info
ℹ️ Scenario 2 — Dash/Evasion, Health/Damage Core, Spell Casting (base)
Walked clean: just-dodge → `State.Executable` grant → proximity + F-key → `TryExecute()` → mana snapped to MaxMana. Skill-gated, no double-dipping, no undefined state. (Caveat inherited from W6: `State.Executable` can also arrive via Elite Shield break once that system exists, but that doesn't break this scenario today.)

ℹ️ Scenario 4 — Health/Damage Core, Luna Overdrive, Combo/Tension Gauge
Player death simultaneously forces Luna's immediate end (no carry-over, `EndReason: PlayerDeath`) and Combo's reset-to-0. Both are idempotent terminal operations — no contradiction found — but neither GDD explicitly sequences them relative to each other. Minor ordering ambiguity, unlikely to be player-visible; not verified against exact GDD text this session (worth a quick explicit note in either GDD, not blocking).

---

## GDDs Flagged for Revision

| GDD | Reason | Type | Priority |
|-----|--------|------|----------|
| `systems-index.md` | False Approved status for 3 rows + Progress Tracker overcounts approvals | Consistency | Blocking |
| `combo-tension-gauge.md` | Still "In Design" — needs to actually complete review; also owns the W9 tuning-default risk and the W6 grantor-conflation gap | Consistency + Design Theory | Blocking (status) / Warning (content) |
| `luna-overdrive.md` | Pending re-review (5 blockers already fixed in-session, re-review itself never run); stale Dependencies-table wording (W5) | Consistency | Blocking (status) / Warning (content) |
| `combat-hud.md` | Pending review; reciprocity asymmetry (W3) | Consistency | Blocking (status) / Warning (content) |
| `spell-casting-base.md` | Stale "미설계" markers (W1); undeclared RawDamage ownership (W11) | Consistency | Warning |
| `health-damage-core.md` | Stale "미설계" marker (W1); tentative `OnDamageApplied` interface (W7) | Consistency | Warning |
| `dash-evasion.md` | Stale "미설계" marker (W1) | Consistency | Warning |
| `player-movement.md` | Rule 11 look-input contradiction (W4) | Consistency | Warning |
| `camera-system-base.md` | Undeclared Luna Overdrive dependency (W2); Rule 11 conflict counterpart (W4) | Consistency | Warning |

---

## Verdict: FAIL

One blocking issue (B1) — the systems index's approval claims are false, and the underlying re-reviews for 3 GDDs genuinely have not happened. This isn't optional cleanup: `/gate-check pre-production` and `/create-architecture` both trust this tracker, and both would proceed on a false "MVP 9/9 approved" premise if run now.

### Required actions before re-running:
1. Run `/design-review` (re-review) on `luna-overdrive.md` — its 5 prior blockers were fixed in-session but re-review itself was never executed.
2. Run `/design-review` on `combo-tension-gauge.md` (currently "In Design", never formally reviewed) and `combat-hud.md` (currently "pending review").
3. Correct `systems-index.md`'s Progress Tracker and the 3 stale "Approved" rows to match actual status once each real review completes.
4. Address W9 (refresh-chain default) as part of `combo-tension-gauge.md`'s review — this is a design-theory finding a fresh reviewer should weigh in on, not just a docs fix.
5. Sweep the W1 stale "미설계" markers and W2/W3/W4/W5 dependency-table fixes — all are small, mechanical edits once the three re-reviews land.

Chain-of-Verification: 5 questions checked (2 [TOOL ACTION] — grepped GDD Status headers directly, re-read systems-index.md Progress Tracker table) — verdict unchanged from initial draft; B1's severity was confirmed rather than softened.
