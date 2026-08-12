# Gate Check: Pre-Production → Production

**Date**: 2026-08-12
**Checked by**: gate-check skill
**Review mode**: solo (`production/review-mode.txt`)
**Previous verdicts**: FAIL (2026-07-18), FAIL (2026-07-20 recheck), PASS (2026-07-20 final pass — Technical Setup → Pre-Production)

> **Director Panel skipped — Solo mode. Gate verdict based on artifact and quality checks only.**

---

## Required Artifacts: 8/16 present

| # | Artifact | Status | Evidence |
|---|---|---|---|
| 1 | Vertical slice in `prototypes/` with REPORT.md | ❌ MISSING | 2 spikes exist (`arena-morphing-spike-2026-07-16`, `signature-combat-chain-spike-2026-07-21`), both SPIKE-NOTE only — no REPORT.md, no verdict |
| 2 | First sprint plan in `production/sprints/` | ❌ MISSING | Directory empty; confirmed not gitignored (`git check-ignore` returned nothing) |
| 3 | Art bible complete (9 sections) + AD-ART-BIBLE sign-off | ❌ PARTIAL | `design/art/art-bible.md` — Sections 1–4 written, "Sections 5–9: Pending" (line 144). Sign-off legitimately skipped per solo mode (line 7) |
| 4 | `design/assets/entity-inventory.md` | ❌ MISSING | `design/assets/` directory does not exist (recommended, not blocking) |
| 5 | All MVP-tier GDDs complete | ✅ PASS | 9/9 MVP GDDs present and Approved per `systems-index.md` rows 1–20 |
| 6 | `docs/architecture/architecture.md` | ✅ PASS | Present |
| 7 | ≥3 Foundation-layer ADRs | ✅ PASS | 11 ADRs (0001–0011) |
| 8 | All Foundation + Core ADRs status `Accepted` | ✅ PASS | 11/11 verified individually — none `Proposed` |
| 9 | `docs/architecture/control-manifest.md` | ✅ PASS | Present, Manifest Version 2026-07-27, covers ADR-0001…0011 |
| 10 | Epics with Foundation + Core layers | ⚠️ PARTIAL | 3 epics exist (1 Foundation, 2 Core). Only `player-movement-foundation-fixes` has stories (5). Camera System and Dash/Evasion epics have zero stories |
| 11 | Vertical Slice build playable | ❌ MISSING | Not built |
| 12 | VS playtested, ≥1 documented session | ❌ MISSING | `production/playtests/` empty, not gitignored |
| 13 | VS playtest report | ❌ MISSING | Same |
| 14 | UX specs: main menu, core HUD, pause menu | ❌ PARTIAL | HUD ✅ (`design/ux/combat-hud.md` — accepted as equivalent to the gate's `design/ux/hud.md`, governed by ADR-0010). **Main menu ❌. Pause menu ❌** |
| 15 | HUD design document | ✅ PASS | `design/ux/combat-hud.md` + `ue58-combat-hud-implementation.md` |
| 16 | Key screen UX specs passed `/ux-review` | ❌ MISSING | No `/ux-review` report found anywhere under `design/` |

**Additional artifacts present (not gate-required but relevant)**: `design/accessibility-requirements.md` ✅, `design/ux/interaction-patterns.md` ✅, `design/ux/tutorial-flow.md`, `docs/architecture/traceability-index.md`, `docs/architecture/tr-registry.yaml`.

---

## Quality Checks: 7/13 passing

| Check | Status | Evidence |
|---|---|---|
| Core loop fun validated by playtest | ❌ FAIL | No playtest data exists |
| Core fantasy delivered (playtester description) | ❌ FAIL | No playtest data exists |
| UX specs cover all UI Requirements from MVP GDDs | ⚠️ PARTIAL | Only Combat HUD covered; `combat-hud.md` GDD is the only UI-tier MVP system, so coverage of *GDD* UI requirements is arguably complete — but menu/pause screens named by the gate are unspecced |
| Interaction pattern library documents patterns used | ✅ PASS | `design/ux/interaction-patterns.md` present |
| Accessibility tier addressed in key screen specs | ⚠️ PARTIAL | `design/accessibility-requirements.md` exists with committed tier; cannot be addressed in the two missing specs |
| Sprint plan references real story file paths | ❌ FAIL | No sprint plan exists |
| Vertical Slice is COMPLETE end-to-end | ❌ N/A | Not built → downgraded to CONCERNS per skill's VS verdict rule |
| Architecture doc: no unresolved open questions in Foundation/Core | ❌ FAIL | `docs/consistency-failures.md` has **2 Open Core-layer conflicts**: (a) `dash-evasion.md` Rule 2 vs Rule 4 — position-step vs velocity-override motion model; (b) `camera-system-base.md` Formula 3 vs Rule/Tuning/AC — `CameraLagMaxDistance` 200uu vs 60.0uu |
| All ADRs have Engine Compatibility section | ✅ PASS | 11/11 verified by grep |
| All ADRs have ADR Dependencies section | ✅ PASS | 11/11 verified by grep |
| Traceability matrix: zero Foundation layer gaps | ✅ PASS | `traceability-index.md` line 145: "Foundation layer: no gaps." Remaining 13 gaps (17%) are Feature layer only |
| Test framework functional | ✅ PASS | `tests/unit/movement/` (4), `tests/static/` (4), `tests/integration/movement/` (1), `tests/smoke/critical-paths.md` |
| Smoke + QA gate | ✅ PASS | `smoke-2026-08-12.md` → PASS; `qa-signoff-player-movement-foundation-fixes-2026-08-12.md` → APPROVED WITH CONDITIONS |

---

## Blockers

1. **No sprint plan** — `production/sprints/` is empty. This is a hard required artifact for entering Production; there is no scoped, story-referencing plan for the first production sprint. → `/sprint-plan new`
2. **Art bible Sections 5–9 not written** — gate requires all 9 sections complete. Sections 1–4 (Visual Identity Foundation) satisfy the *Technical Setup* gate, not this one. Asset production entering Production without Sections 5–9 has no material/lighting/VFX/UI-art/pipeline standard to enforce against. → `/art-bible`
3. **Main menu and pause menu UX specs missing** — the gate names three key screens; only the core HUD is specced. → `/ux-design main-menu`, `/ux-design pause-menu`
4. **No `/ux-review` reports** — no key screen UX spec has been formally reviewed, including the one that exists (`combat-hud.md`). → `/ux-review all`
5. **Two Open Core-layer design conflicts** — both sit in epics that have no stories yet, so they will be hit immediately on the first Camera or Dash story:
   - `dash-evasion.md` Rule 2 vs Rule 4 — motion model contradiction (position step vs velocity override). ADR-0007 is Accepted against an ambiguous GDD.
   - `camera-system-base.md` — `CameraLagMaxDistance` specified as both 200uu and 60.0uu.
   → resolve and mark Resolved in `docs/consistency-failures.md`, then re-run `/consistency-check`

---

## Concerns (non-blocking)

- **No Vertical Slice built, playtested, or reported.** Per this skill's explicit rule, a *skipped* slice downgrades to CONCERNS rather than FAIL — a valid solo-dev call. But the risk stands: **advancing without a validated Vertical Slice increases the risk of late-stage design pivots.** Both quality checks that depend on it ("core loop fun is validated", "core fantasy is delivered") are currently unanswerable. The two existing spikes (`arena-morphing`, `signature-combat-chain`) are narrow technical spikes, not a full-loop slice.
- **2 of 3 epics have zero stories.** Camera System and Dash/Evasion epics are scoped but not broken down. → `/create-stories camera-system-foundation-fixes`, `/create-stories dash-evasion-foundation-fixes`
- **`design/assets/entity-inventory.md` missing** (recommended artifact). → `/asset-spec` with no arguments
- **`production/stage.txt` does not exist.** The 2026-07-20 Pre-Production gate PASSed but Phase 6 (write stage.txt) was never executed, so the status line has had no stage breadcrumb since. Independent of this gate's verdict, this should be created.
- **Both prototypes are undocumented** (no README/CONCEPT) — flagged by the session-start hook. → `/reverse-document concept prototypes/[name]`

---

## Minimal path to PASS

Three things, in this order:

1. `/sprint-plan new` — scope the first production sprint against real story paths in `production/epics/`. Requires stories to exist first, so run `/create-stories camera-system-foundation-fixes` and `/create-stories dash-evasion-foundation-fixes` before it.
2. `/art-bible` — complete Sections 5–9.
3. `/ux-design main-menu`, `/ux-design pause-menu`, then `/ux-review all`.

Resolve the two Open consistency failures alongside (2) and (3) — they are cheap (a rule rewrite and a number reconciliation) but they gate the Camera and Dash stories.

If the user chooses to advance despite the missing Vertical Slice, that is an accepted risk, documented above — it is not what causes this FAIL.

---

## Chain-of-Verification

5 questions checked — verdict **unchanged (FAIL)**.

1. *Have I accurately separated hard blockers from strong recommendations?* — Yes. The Vertical Slice cluster (items 1, 11, 12, 13) is explicitly downgraded to CONCERNS per the skill's own rule. The five blockers are all items the gate lists as required with no skip provision.
2. *Are there PASS items I was too lenient about?* **[TOOL ACTION]** — Re-grepped all 11 ADR Status lines individually (ADR-0005…0011 use a `## Status` heading rather than a `**Status:**` line; the first grep pass returned empty and would have produced a false FAIL). All 11 read `Accepted`. Also accepted `design/ux/combat-hud.md` as the gate's `design/ux/hud.md` — justified: it is the HUD design doc, governed by ADR-0010, and the gate's filename is illustrative.
3. *Am I missing additional blockers?* **[TOOL ACTION]** — Read `docs/consistency-failures.md` (skill Phase 3 preamble) and found 2 **Open** entries in Core-layer domains (Dash/Evasion, Camera System) that no other check surfaced. Added as Blocker 5. Also re-read `design/art/art-bible.md` section headers directly rather than inferring from file existence — confirmed "Sections 5–9: Pending".
4. *Can I provide a minimal path to PASS?* — Yes, three ordered actions above.
5. *Is the fail condition resolvable, or does it indicate a deeper design problem?* — Resolvable. Architecture is in good shape (11/11 ADRs Accepted with full Engine Compatibility and Dependencies sections, zero Foundation traceability gaps, control manifest current, smoke + QA both green). The gaps are planning and design-doc artifacts, not architectural rot. The one genuine risk — no validated Vertical Slice — is a deliberate scope call, not a defect.

---

## Verdict: ❌ FAIL

Five hard blockers. Architecture and test infrastructure are production-ready; **planning and UX artifacts are not**. Per the collaborative protocol this verdict is advisory — the user may advance despite it, accepting the documented risks.

`production/stage.txt` is **not** written by this run.
