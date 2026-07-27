# Architecture Review — 2026-07-27 (v2, post-ADR-0008/0009)

- **Date:** 2026-07-27
- **Engine:** Unreal Engine 5.8 (pinned 2026-07-16, post-LLM-cutoff — HIGH risk)
- **Mode:** `full` — delta-verified against [architecture-review-2026-07-27.md](architecture-review-2026-07-27.md)
  (the same-day baseline). GDD text is unchanged since that pass except the two still-open internal
  contradictions noted below (C-2, C-3) — this run re-reads the 2 new/changed ADRs (0008, 0009) plus
  the 5 amended ADRs (0002, 0004, 0006, 0007, and 0007's E-1 fix) in full, and spot-verifies every
  other artifact the baseline flagged (registry, systems-index, consistency-failures, architecture.md,
  pre-gate files) rather than re-deriving all 76 requirements from a blank slate.
- **ADRs Reviewed:** 9 (0001–0009), all now checked
- **Previous review:** [architecture-review-2026-07-27.md](architecture-review-2026-07-27.md) — verdict FAIL, 23 gaps / 76
- **Also see:** [architecture-review-2026-07-27-c1-recheck.md](architecture-review-2026-07-27-c1-recheck.md) — the intermediate consistency-only pass done before ADR-0008 was Accepted

---

## Verdict: 🟡 CONCERNS (up from 🔴 FAIL)

Foundation layer is now **100% covered** — the cause of the FAIL verdict is gone. C-1, the recurring
blocking conflict, is resolved: producer exists, is Accepted, and all three consumers (ADR-0002/0004/0006)
correctly cite it. Remaining gaps are entirely Feature/Presentation layer (Combo/Tension Gauge, Combat HUD)
— real gaps, but not blocking-grade, and not new.

| Criterion | Result |
|---|---|
| All requirements covered | ❌ 13 gaps / 76 (down from 23) |
| No blocking cross-ADR conflicts | ✅ C-1 resolved. C-2/C-3 remain, both non-blocking (GDD self-contradictions, not ADR conflicts) |
| Foundation layer complete | ✅ Player Movement 10/10, Health/Damage Core 9/9 — both fully covered |
| Engine consistency | ✅ E-1 fixed this session. E-2/E-3 remain, both non-blocking (documented risks, not contradictions) |
| Pre-gate checklist | ✅ no ❌ items remain (was 2 ❌ at the 07-27 baseline) |

`/gate-check pre-production` **can now be offered** — see Pre-Gate Checklist below. `/create-epics` is
no longer blocked by Foundation gaps, though the two uncovered Feature/Presentation systems should get
ADRs before their epics are created.

---

## Traceability Summary

| | 2026-07-27 (baseline) | 2026-07-27 (v2, this pass) | Δ |
|---|---|---|---|
| Total active requirements | 76 | 76 | 0 |
| ✅ Covered | 38 (50%) | **52 (68%)** | +14 |
| ⚠️ Partial | 15 (20%) | **11 (15%)** | −4 |
| ❌ Gap | 23 (30%) | **13 (17%)** | −10 |

### Where the −10 gaps / −4 partials came from

| Source | TRs flipped | New status |
|---|---|---|
| ADR-0008 (Health/Damage Core death contract, Accepted) | TR-hp-006, TR-hp-007, TR-hp-008 | ❌ → ✅ |
| ADR-0008 (same ADR, closes ambiguity ADR-0001 left open) | TR-hp-004 (ref-counted tag auto-clear on death), TR-hp-005 (TryExecute was a stub, now fully wired via Decision 4) | ⚠️ → ✅ |
| ADR-0009 (Player Movement runtime contract, Proposed but content-complete) | TR-mov-002, 003, 006, 007, 008, 009, 010 | ❌ → ✅ |
| C-1 resolution (ADR-0006/0004 now cite an Accepted producer instead of an assumed one) | TR-ai-004, TR-overdrive-006 | ⚠️ → ✅ |

**Note on ADR-0009:** it is scored as covering its 7 TRs even though its own Status field is still
`Proposed` — consistent with how this project's prior reviews scored ADR-0004/0006/0007's requirements
while those were still Proposed (coverage = "an ADR's Decision text addresses this," not "and is
Accepted"). ADR-0009 has no unresolved dependency (`Depends On`: ADR-0001, Accepted) and is not gating
on anything else — it is a candidate for its own Accept pass whenever you want it.

### Coverage by system

| System | ✅ | ⚠️ | ❌ | Governing ADR | Δ since 07-27 baseline |
|---|---|---|---|---|---|
| Health/Damage Core | **9** | 0 | 0 | ADR-0001, ADR-0002, **ADR-0008** | 4→9 ✅, 2→0 ⚠️, 3→0 ❌ |
| Player Movement | **8** | 2 | 0 | ADR-0001, ADR-0005, **ADR-0009** | 1→8 ✅, 7→0 ❌ |
| Camera System (base) | 7 | 2 | 0 | ADR-0005 | unchanged |
| Luna Overdrive | 7 | 1 | 0 | ADR-0004 | unchanged (TR-overdrive-006 counted here: was already 7✅/1⚠️ at baseline table level — see note¹) |
| Enemy AI (base) | 6 | 2 | 0 | ADR-0006 | unchanged at system-table level (TR-ai-004 flip already reflected in prior count — see note¹) |
| Spell Casting (base) | 7 | 3 | 0 | ADR-0003, ADR-0004 | unchanged |
| Dash/Evasion | 5 | 2 | 1 | ADR-0007 | unchanged |
| Combo/Tension Gauge | 1 | 1 | 5 | ADR-0001 (attribute only), ADR-0004 (lock only) | unchanged — **no ADR exists for this system's own scope** |
| Combat HUD | 0 | 0 | 7 | **none** | unchanged |

¹ The 07-27 baseline's per-system table had already folded TR-ai-004 and TR-overdrive-006 into each
system's ✅/⚠️ split inconsistently with its own per-requirement matrix (which marked them ⚠️ "depends
on C-1"). This pass corrects both to ✅ per-requirement; the system table above reflects the corrected
count.

---

## Remaining Coverage Gaps (13) — all Feature/Presentation layer

### 🟠 Feature layer — Combo/Tension Gauge (5, unchanged, no ADR exists)

| TR-ID | Requirement |
|---|---|
| TR-tension-002 | Read-only subscription to `OnSpellHit`/`OnTagAdded(State.Executable)`/`OnDamageApplied`/Death |
| TR-tension-003 | Decay timer with grace gating, keeps ticking during `State.Invulnerable` |
| TR-tension-004 | Deterministic same-frame Gain→Penalty→Decay ordering, clamp [0,Max] |
| TR-tension-005 | `OnOverdriveTriggered` exactly once at Max — **ADR-0004 consumes this event but no ADR defines its emission** |
| TR-tension-007 | Expose read-only value + Building/Decaying state downstream |

→ `/architecture-decision Combo/Tension Gauge` — same recommendation as the 07-27 baseline, still open.

### 🟡 Presentation layer — Combat HUD (7) + Dash HUD surface (1), unchanged, no ADR exists

TR-hud-001 through TR-hud-007, plus TR-dash-008 (dash charge/cooldown → HUD). `WBP_CombatHUD` is
already substantially built in-engine with no governing ADR — same reverse-documentation situation
as the baseline. **Note:** `design/ux/combat-hud.md` now exists (new since the 07-27 baseline,
presumably from a `/ux-design` pass) — this is a UX spec, not an ADR; it does not close these gaps
but may accelerate writing the Combat HUD ADR since the UX-level contract is now written down.

→ `/architecture-decision Combat HUD` — still open.

---

## Cross-ADR Conflicts

### ✅ C-1 — RESOLVED (was 🔴 BLOCKING)

`OnDeath` now has a real, Accepted producer (ADR-0008). All three original consumers correctly cite it:
- ADR-0002 (Accepted) — `RestoreCheckpoint` calls `ResetDeathState()` before Health-restore, per ADR-0008 Migration Plan item 5. Done.
- ADR-0004 (Accepted) — `Depends On` lists ADR-0008; Risks section rewritten to reflect a real, Accepted delegate instead of an assumed future one.
- ADR-0006 (Accepted) — `Depends On` and Related Decisions both cite ADR-0008 directly, replacing the old "Health/Damage Core (Approved, no ADR yet)" placeholder text.

No further action needed on C-1.

### 🟠 C-2 — Dash motion model: GDD Rule 4 vs ADR-0007 Decision 1 — STILL OPEN

Unchanged from baseline. `dash-evasion.md` Rule 4 still reads "velocity를 대쉬 벡터로 완전히 덮어쓴다"
(velocity override) while Rule 2 and ADR-0007 both describe/ratify the swept `SetActorLocation`
position-step model. `TR-dash-002`'s registry text was corrected to the ADR-ratified reading
(per `tr-registry.yaml` v3 notes), but **the GDD itself was not revised** — this is a design-doc fix,
not an ADR fix, and remains the recommended action: `/quick-design` or `/design-review
design/gdd/dash-evasion.md` to restate Rule 4 as override *semantics* delivered by Rule 2's position step.

### 🟡 C-3 — `CameraLagMaxDistance`: 60 uu vs 200 uu inside one GDD — STILL OPEN

Unchanged. `camera-system-base.md:138` (Formula 3) and `:204` (Arena Morphing dependency row) still say
200.0 uu against 4 other sites in the same document (and ADR-0005) saying 60.0 uu. Same fix recommended
as baseline: correct the two 200-uu sites in the GDD.

### 🟡 C-4 — `AMoonCharacterBase` responsibility accretion — STILL A CONCERN, GREW

ADR-0009 adds more surface to the same class flagged at baseline: the hitstop capture-and-blend
rewrite, two new float timer fields (`JumpInputBufferTimer`/`CoyoteTimeTimer`), and the
`bMovementLocked`/`SetMovementLocked()` reservation. Five ADRs (0001, 0004, 0005, 0007, 0009) now place
state on this one class, up from four at baseline. Still not blocking, but the aggregate keeps growing
un-tracked. Same recommendation as baseline: an explicit component-decomposition rule via
`/create-control-manifest` or a small dedicated ADR, before Production.

---

## ADR Dependency Order (refreshed)

```
Foundation (no unresolved deps) — ALL ACCEPTED:
  1. ADR-0001  Player Movement and GAS Core          [Accepted]
  2. ADR-0002  Checkpoint Persistence                [Accepted]  — ResetDeathState() wired, C-1 resolved
  3. ADR-0003  Spell Casting GAS Implementation      [Accepted]
  4. ADR-0008  Health/Damage Core Death Contract     [Accepted]  — closes C-1, unblocks 0004/0006

Ready now (no unresolved deps, content-complete):
  5. ADR-0009  Player Movement Runtime Contract      [Proposed]  — Depends On: ADR-0001 (Accepted) only

Also Accepted, previously gated on ADR-0008:
  6. ADR-0004  Luna Overdrive Fixed Window            [Accepted]
  7. ADR-0006  Enemy AI Behavior Tree                 [Accepted]
  8. ADR-0007  Dash/Evasion Just-Dodge                [Accepted]  — Depends On: ADR-0006 (Accepted)
  9. ADR-0005  Camera System (SpringArm)               [Accepted]  (accepted independently, 07-27 baseline)
```

**Unresolved dependency flags:** none. Every ADR's `Depends On` field now names either nothing or an
Accepted ADR. **Cycles:** none. This is the first review pass where the dependency graph is fully
clean — the entire 07-27 baseline's "Unresolved dependency flags" section is now empty.

**Only ADR-0009 remains Proposed.** It has zero blockers; it simply hasn't had its own Accept pass yet.

---

## GDD Revision Flags — status check

| GDD | Flagged issue (07-27 baseline) | Status now |
|---|---|---|
| `dash-evasion.md` Rule 4 | velocity-override wording contradicts Rule 2 / ADR-0007 | ❌ **still unrevised** (C-2) |
| `camera-system-base.md` Formula 3 / Arena Morphing row | 200 uu vs 60 uu | ❌ **still unrevised** (C-3) |
| `dash-evasion.md` Tuning Knobs | missing `JustDodgeQueryRadius`/`AirDashZImpulse` rows | ❌ **still not added** — ADR-0007 Migration Plan item 5 still pending |
| `enemy-ai-base.md` Tuning Knobs | missing `MeleeAttackRange` | ❌ **still not added** — ADR-0007/0006 Migration Plan pending |
| `dash-evasion.md` header re-review | stale Last-Updated after 2026-07-20 Rule 2 edit | ❌ **still not re-reviewed** |

None of these block Acceptance of any ADR (all five ADRs that reference them are already Accepted with
the gap explicitly flagged in-ADR) — but they are real outstanding GDD-authoring debt, unchanged since
baseline. Recommend a `/quick-design` pass across `dash-evasion.md` and `camera-system-base.md` to clear
all five in one go.

---

## Engine Compatibility Audit — refreshed

| Finding | Baseline (07-27) | Status now |
|---|---|---|
| **E-1** — ADR-0007 ratified deprecated `SetMovementMode()` while claiming "Post-Cutoff APIs Used: None" | 🔴 open | ✅ **fixed this session** — table now accurately states the deprecated call and cites the migration; Migration Plan item 6 added |
| **E-2** — Engine-reference library missing `SetAssetTags()`/`AbilityTags` deprecation entries the project already worked around | 🟠 open | ❌ **still open** — `deprecated-apis.md` has no `SetAssetTags`/`AbilityTags`/`task_a3fdf7fc` entries; the background task noted in the baseline has not landed |
| **E-3** — ADR-0004 (`SetLooseGameplayTagCount`) and ADR-0006 (AIPerception signatures, no `modules/ai.md`) carry unverifiable-by-reference claims | 🟡 open, self-flagged | ⚠️ **unchanged** — both ADRs are now Accepted with these risks still explicitly flagged as implementation-time verification items, same treatment as every other Accepted ADR's open Verification Required items |
| New this pass — ADR-0008/0009 both flag **GAS callback signature drift** (`PostGameplayEffectExecute`/`PreAttributeChange` signature assumptions) as unverified against real 5.8 headers | — | 🟡 new, non-blocking — same class of risk as E-3, explicitly flagged in both ADRs' own Risks sections |

Recommend: land the E-2 background task (`task_a3fdf7fc`) before the next full review — it's the one
engine-reference gap with a concrete, already-scoped fix sitting idle.

---

## Architecture Document Coverage (`architecture.md`)

**Still stale** — unchanged since baseline: Version 1.0, `Last Updated: 2026-07-18`, "ADRs Referenced:
ADR-0001" only. It is now stale against **8 additional ADRs** (0002–0009), not 6. The baseline's
recommendation to refresh "after the ★ ADR lands" — ADR-0008 has now landed and been Accepted, along
with 0009's authoring and 0004/0006/0007's Acceptance. This is the right time for that refresh.

Structurally still correct per the baseline's own assessment (layer map, module ownership, data flow,
principles, API boundary) — only the bookkeeping sections (ADR Audit, Traceability Coverage, Required
ADRs list, 2026-07-18-era Open Questions) need updating.

---

## Registry and Consistency-Log Bookkeeping

- **`tr-registry.yaml`**: no changes needed this pass. No new, superseded, or reworded requirements
  were introduced by ADR-0008/0009 (both address existing TR-IDs verbatim). Registry stays at v3.
- **`docs/consistency-failures.md`**: the C-1 entries (rows for 2026-07-18 and 2026-07-27, plus the
  full write-up) are now stale — they still read "Open — RECURRED" / "Open — BLOCKING". Updated in
  this pass (see file diff) to reflect Resolved, with the resolution and pattern recorded per the
  skill's Reflexion Log convention. C-2/C-3 rows are untouched — both still genuinely open.
- **`design/gdd/systems-index.md`**: no drift found — already corrected to 9/9 Approved in a prior
  session, still accurate.

---

## Pre-Gate Checklist

| Item | Baseline (07-27) | Status now |
|---|---|---|
| `tests/unit/` | ⚠️ `.gitkeep` only | ⚠️ unchanged — still no test files |
| `tests/integration/` | ⚠️ `.gitkeep` only | ⚠️ unchanged — still no test files |
| `.github/workflows/tests.yml` | ❌ missing | ✅ **now exists** |
| `design/accessibility-requirements.md` | ❌ missing | ✅ **now exists** |
| `design/ux/interaction-patterns.md` | ❌ missing | ✅ **now exists** |

No ❌ items remain. `/gate-check pre-production` can now be offered — the two ⚠️ items (empty test
dirs) are a test-content gap, not a missing-infrastructure gap, and are advisory per this project's
test-evidence-by-story-type table, not a hard pre-gate blocker.

---

## Blocking Issues (must resolve before PASS)

1. **Combo/Tension Gauge** — 5 of 7 uncovered, including TR-tension-005 (ADR-0004's upstream event
   producer, still unspecified by any ADR).
2. **Combat HUD** — 0 of 7 covered, widget already built in-engine, now has a UX spec
   (`design/ux/combat-hud.md`) but no ADR.
3. **ADR-0009 is Proposed** — content-complete, zero blockers, just needs an Accept pass.

## Non-blocking but should be fixed in the same pass

4. **C-2 / C-3** — two Approved GDDs each still contradict themselves internally (unchanged from baseline).
5. **`dash-evasion.md` / `enemy-ai-base.md` Tuning Knobs additions** — still not added (unchanged).
6. **`architecture.md`** — bookkeeping stale by 8 ADRs now (was 6 at baseline).
7. **E-2** — engine-reference GAS deprecation entries still missing; background task still not landed.

---

## Required ADRs (prioritised)

| # | ADR | Layer | Closes | Why now |
|---|---|---|---|---|
| 1 | **Combo/Tension Gauge** — attribute, decay, ordering, trigger emission | Feature | TR-tension-002/003/004/005/007 | ADR-0004's upstream producer is still unspecified; this is now the only Feature-layer gap |
| 2 | **Combat HUD** — UMG/CommonUI widget architecture | Presentation | TR-hud-001..007, TR-dash-008 | Partly reverse-documentation; `design/ux/combat-hud.md` now gives it a UX-level head start |

Foundation layer requires **no new ADRs** — both items that drove the FAIL verdict (Health/Damage Core,
Player Movement) are closed.

---

## Recommended Next Actions

1. Accept ADR-0009 — zero blockers, same pattern as the other five Accept passes this session
2. `/architecture-decision Combo/Tension Gauge` — the last Feature-layer gap
3. `/architecture-decision Combat HUD` — the last Presentation-layer gap (leverage `design/ux/combat-hud.md`)
4. `/quick-design` pass on `dash-evasion.md` (C-2, Tuning Knobs additions) and `camera-system-base.md` (C-3)
5. Refresh `docs/architecture/architecture.md` — bookkeeping only, structure still holds
6. Land the E-2 background task (`deprecated-apis.md` GAS entries)
7. Once items 2–3 land, re-run `/architecture-review` — should be the first PASS-eligible state
