# Gate Check: Pre-Production → Production

**Date**: 2026-08-14
**Checked by**: gate-check skill
**Review mode**: solo (`production/review-mode.txt`)
**Previous verdicts**: FAIL (2026-07-18), FAIL (2026-07-20 recheck), PASS (2026-07-20 — Technical Setup → Pre-Production), **FAIL (2026-08-12 — same gate as this one)**

> **Director Panel skipped — Solo mode. Gate verdict based on artifact and quality checks only.**

---

## What changed since the 2026-08-12 FAIL

Real work landed, but **none of it touched the five blockers**. For the record:

- Player Movement Foundation Fixes epic **fully closed** — all 5 stories Complete, all AC passing (Story 001 AC-4 closed via binary asset-registry inspection; Story 004 AC-1/3/4 closed via human PIE playtest; Story 005 AC-4 closed via a 100-actor `unreal-mcp`-driven PIE run + `stat unit` baseline of Frame 26.50ms / Game 26.98ms).
- Repo hygiene: 713 untracked files classified and resolved (987MB of marketplace packs gitignored, 223 real artifacts committed). Working tree clean.
- **A bad "fix" was caught and reverted** — see Blocker 6 below, which is new to this gate.

The five blockers from 2026-08-12 are **unchanged**, verified individually below.

---

## Required Artifacts: 8/16 present

| # | Artifact | Status | Evidence (re-verified 2026-08-14) |
|---|---|---|---|
| 1 | Vertical slice in `prototypes/` with REPORT.md | ❌ MISSING | 2 spikes only; `find prototypes -iname REPORT.md` → zero hits. Recommended, not blocking → CONCERNS |
| 2 | First sprint plan in `production/sprints/` | ❌ MISSING | Directory **does not exist at all** (`ls` → No such file or directory). Worse than 2026-08-12, when it existed but was empty |
| 3 | Art bible complete (9 sections) + AD-ART-BIBLE sign-off | ❌ PARTIAL | `design/art/art-bible.md` — headers confirm Sections 1–4 only; line 144 still literally reads `## Sections 5–9: Pending`. Sign-off legitimately N/A per solo mode |
| 4 | `design/assets/entity-inventory.md` | ❌ MISSING | Recommended, not blocking → CONCERNS |
| 5 | All MVP-tier GDDs complete | ✅ PASS | `systems-index.md` — 11 Approved entries |
| 6 | `docs/architecture/architecture.md` | ✅ PASS | Present, 19.3KB (real content, not a header stub) |
| 7 | ≥3 Foundation-layer ADRs | ✅ PASS | 11 ADRs (0001–0011) |
| 8 | All Foundation + Core ADRs status `Accepted` | ✅ PASS | **11/11 re-verified.** Note: my first grep returned 7 because ADRs 0001–0004 use inline `**Status:** Accepted` while 0005–0011 use a `## Status` header. Zero `Proposed`. The 2026-08-12 report was correct |
| 9 | `docs/architecture/control-manifest.md` | ✅ PASS | Present, 9.0KB |
| 10 | Epics with Foundation + Core layers | ⚠️ PARTIAL | 3 epics. `player-movement-foundation-fixes` = 5 stories (all Complete). `camera-system-foundation-fixes` = **0 stories**. `dash-evasion-foundation-fixes` = **0 stories** |
| 11 | Vertical Slice build playable | ❌ MISSING | Not built → CONCERNS per skill rule |
| 12 | VS playtested, ≥1 documented session | ❌ MISSING | `production/playtests/` **does not exist** → CONCERNS |
| 13 | VS playtest report | ❌ MISSING | Same → CONCERNS |
| 14 | UX specs: main menu, core HUD, pause menu | ❌ PARTIAL | `design/ux/` contains only `combat-hud.md`, `interaction-patterns.md`, `tutorial-flow.md`, `ue58-combat-hud-implementation.md`. **Main menu ❌, pause menu ❌** |
| 15 | HUD design document | ✅ PASS | `design/ux/combat-hud.md` (+ implementation doc) |
| 16 | Key screen UX specs passed `/ux-review` | ❌ MISSING | `find` across `design/`, `docs/`, `production/` for `*ux-review*` → zero hits |

---

## Quality Checks: 8/13 passing

| Check | Status | Evidence |
|---|---|---|
| Core loop fun validated by playtest | ❌ FAIL | No playtest data exists |
| Core fantasy delivered (playtester description) | ❌ FAIL | No playtest data exists |
| UX specs cover all UI Requirements from MVP GDDs | ⚠️ PARTIAL | Combat HUD is the only UI-tier MVP system, so *GDD* UI coverage is arguably complete — but the gate names menu/pause screens, which are unspecced |
| Interaction pattern library documents patterns used | ✅ PASS | `design/ux/interaction-patterns.md` present |
| Accessibility tier addressed in key screen specs | ⚠️ PARTIAL | `design/accessibility-requirements.md` present (7.2KB, tier committed); cannot be addressed in two specs that don't exist |
| Sprint plan references real story file paths | ❌ FAIL | No sprint plan, no sprints directory |
| Vertical Slice is COMPLETE end-to-end | ❌ N/A | Not built → CONCERNS per skill's VS rule, not FAIL |
| Architecture doc: no unresolved open questions in Foundation/Core | ❌ FAIL | `docs/consistency-failures.md` lines 12–13 — **2 Open Core-layer conflicts, unchanged**: (a) `dash-evasion.md` Rule 2 vs Rule 4, position-step vs velocity-override; (b) `camera-system-base.md` `CameraLagMaxDistance` 200uu vs 60.0uu |
| All ADRs have Engine Compatibility section | ✅ PASS | 11/11 |
| All ADRs have ADR Dependencies section | ✅ PASS | 11/11 |
| Traceability matrix: zero Foundation layer gaps | ✅ PASS | `traceability-index.md:145` — "Foundation layer: no gaps." 13 remaining gaps (17%) are Feature layer only |
| Test framework functional + tests passing | ✅ PASS | **All 9 test scripts re-run live this session, all PASS**: 4 static, 4 unit, 1 integration |
| Smoke + QA gate | ✅ PASS | `smoke-2026-08-12.md` → PASS; `qa-signoff-...-2026-08-12.md` → APPROVED WITH CONDITIONS |

---

## Blockers

1. **No sprint plan** — `production/sprints/` does not exist. Hard required artifact; there is no scoped, story-referencing plan for the first production sprint. → `/sprint-plan new`
2. **Art bible Sections 5–9 not written** — gate requires all 9. Sections 1–4 satisfied the *Technical Setup* gate, not this one. Entering Production means starting asset production with no material/lighting/VFX/UI-art/pipeline standard. → `/art-bible`
3. **Main menu and pause menu UX specs missing** — gate names three key screens; only the core HUD is specced. → `/ux-design main-menu`, `/ux-design pause-menu`
4. **No `/ux-review` reports** — no UX spec has been formally reviewed, including the one that exists. → `/ux-review all`
5. **Two Open Core-layer design conflicts** — both sit in the two epics that have zero stories, so the first Camera or Dash story written will hit them immediately:
   - `dash-evasion.md` Rule 2 vs Rule 4 — motion model contradiction. ADR-0007 is Accepted against an ambiguous GDD.
   - `camera-system-base.md` — `CameraLagMaxDistance` specified as both 200uu and 60.0uu.
   → resolve, mark Resolved in `docs/consistency-failures.md`, re-run `/consistency-check`

6. **NEW — the engine-reference library is demonstrably unreliable, and 10 of 11 ADRs rest on it.**
   `docs/engine-reference/unreal/` is this project's designated defense against its own documented risk ("Engine version is past both models' training cutoff — cross-check before trusting either model's native UE knowledge"). Two of its claims are now disproven:
   - **`SetMovementModeWithCustomMode()` — proven false by the compiler.** `deprecated-apis.md` claimed `SetMovementMode()` was deprecated in 5.8 and must migrate to this function. A real UBT build failed with `error C2039: not a member`. The function does not exist; `SetMovementMode()` is the only overload and is not deprecated. This false claim had already propagated into ADR-0007's Engine Compatibility table *and* its Migration Plan item 6, and into shipped code via commit `f54a85f` — which was "verified" by cross-checking against the same wrong doc. Reverted and corrected in `cb7a626`.
   - **GAS attribute-set init — unsubstantiated, checked this gate.** `deprecated-apis.md` lists "Legacy GAS attribute set initialization functions" as deprecated. ADR-0001's still-open Verification Required item cites it, instructing implementers to avoid `InitFromMetaDataTable`. Reading the real installed header: `AttributeSet.h:243` declares `UE_API virtual void InitFromMetaDataTable(const UDataTable* DataTable);` with **no `UE_DEPRECATED` marker**. The claim does not hold for the specific API the ADR names.

   The pattern, not the two instances, is the blocker: doc-vs-doc agreement was repeatedly mistaken for verification, and it already shipped one wrong code change. Production means writing substantially more code against these ADRs' Engine Compatibility sections. → audit every row of `deprecated-apis.md` and `breaking-changes.md` against the installed engine headers, mark each row verified/unverified, then re-check the Verification Required items in all 11 ADRs.

---

## Concerns (non-blocking)

- **No Vertical Slice built, playtested, or reported.** Per the skill's explicit rule a *skipped* slice is CONCERNS, not FAIL — a valid solo-dev call. The risk stands: advancing without one increases the risk of late-stage design pivots, and the two quality checks that depend on it (core loop fun, core fantasy delivered) remain unanswerable. The two existing spikes are narrow technical spikes, not a full-loop slice.
- **2 of 3 epics have zero stories** — Camera System and Dash/Evasion are scoped but not broken down. → `/create-stories camera-system-foundation-fixes`, `/create-stories dash-evasion-foundation-fixes`
- **Build compile status is currently UNVERIFIABLE.** A UBT build was attempted twice this session and both times returned `Unable to build while Live Coding is active` (editor open). The working tree contains the `SetMovementMode` revert plus a new `Moon_BenchmarkSpawnMovers` exec function that have **never had a verified clean full build** — only a UHT pass and a partial compile. Not a gate-listed item for this transition, but it should be resolved before relying on the branch: close the editor and re-run `Build.bat MoonEditor Win64 Development`.
- `design/assets/entity-inventory.md` missing (recommended). → `/asset-spec`
- Both prototypes undocumented (no README/CONCEPT) — flagged by the session-start hook. → `/reverse-document concept prototypes/[name]`

---

## Minimal path to PASS

1. `/sprint-plan new` — produce the first production sprint plan referencing real story paths.
2. `/art-bible` — write Sections 5–9.
3. `/ux-design main-menu` + `/ux-design pause-menu`, then `/ux-review all`.
4. Resolve the two Open Core conflicts in `docs/consistency-failures.md`, re-run `/consistency-check`.
5. Audit `deprecated-apis.md` / `breaking-changes.md` against installed engine headers; re-check the Verification Required items in all 11 ADRs.

Items 1–4 are documentation/planning work. Item 5 is verification work. None indicate a design dead-end — this is an artifact-completeness failure, not a broken project.

---

## Chain-of-Verification

5 questions checked — **verdict unchanged (FAIL)**.

1. *Blockers vs. strong recommendations separated correctly?* — Yes. All four Vertical-Slice-dependent artifacts (#1, #11, #12, #13) are downgraded to CONCERNS per the skill's explicit VS rule, not counted as blockers.
2. *Any PASS item I was too lenient about?* **[TOOL ACTION]** — Re-verified rather than inherited: ADR status (caught and corrected my own format-blind grep: 11/11 Accepted, not 7), traceability Foundation gaps (zero, `traceability-index.md:145`), `architecture.md`/`control-manifest.md` byte sizes (19.3KB/9.0KB — real content), and all 9 test scripts re-run live (all PASS). No leniency found.
3. *Missing any additional blockers?* **[TOOL ACTION]** — Yes, one, found by reading the installed UE 5.8 `AttributeSet.h` directly: Blocker 6. Also surfaced the unverifiable-build concern.
4. *Minimal path to PASS?* — Five specific items above.
5. *Resolvable, or deeper design problem?* — Resolvable. The only items requiring genuine design decisions are the two Open Core conflicts (Blocker 5); the rest is authoring and verification.

---

## Verdict: **FAIL**

Same verdict as 2026-08-12, with one additional blocker. `production/stage.txt` remains **Pre-Production** — not modified by this gate.

The epic that closed since the last gate (Player Movement Foundation Fixes, 5/5 stories, all AC passing, all tests green) is real progress and materially de-risks the Foundation layer. It does not move this gate, because every blocker here is a Pre-Production *planning and validation* artifact rather than implementation work.
