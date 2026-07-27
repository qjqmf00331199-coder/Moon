# Architecture Review — C-1 Recheck — 2026-07-27

- **Date:** 2026-07-27
- **Mode:** `consistency` (cross-ADR conflict detection only, targeted at C-1 — not a full requirements rescan)
- **Trigger:** ADR-0002 amended (`ResetDeathState()` call added to `RestoreCheckpoint`) and ADR-0008 authored earlier this session; user asked to confirm C-1 (from [architecture-review-2026-07-27.md](architecture-review-2026-07-27.md)) is resolved.
- **Scope note:** This is a scoped recheck, not a substitute for the full review. GDD requirement rescan and engine-specialist consultation were not re-run.

---

## C-1 — `OnDeath` consumed by three ADRs, defined by none

### Verdict: 🟠 PARTIALLY RESOLVED

The design gap is closed — a real producer now exists and is GDD-consistent. The paper trail was not fully closed until this pass: two of the three consuming ADRs still described the old "no producer" world.

| Sub-issue | Status before this pass | Status after this pass |
|---|---|---|
| Producer ADR exists | ❌ none | ✅ [ADR-0008](0008-health-damage-core-death-event-contract.md) defines `OnDeath`, `IMoonHealthEventInterface`, `ResetDeathState()` |
| ADR-0002 amendment (its Migration Plan item 5 requirement) | ❌ not applied | ✅ Decision §3, Key Interfaces, Migration Plan, Related Decisions all updated to call `ResetDeathState()` before Health-restore GameplayEffect |
| ADR-0004 cites ADR-0008 | ❌ `Depends On` listed only ADR-0001/0003; Risks said "canonical death delegate" doesn't exist | ✅ `Depends On` now lists ADR-0008; Risks line rewritten — no longer describes an undefined producer |
| ADR-0006 cites ADR-0008 | ❌ `Depends On` literally read "Health/Damage Core (Approved, no ADR yet...)" | ✅ `Depends On` and Related Decisions now cite ADR-0008 directly |
| ADR-0008 itself Accepted | ❌ Proposed | ❌ **still Proposed** — not changed by this pass |

### Remaining blocker

**ADR-0008 has not been Accepted.** Per `docs/CLAUDE.md`'s status lifecycle, stories referencing a `Proposed` ADR are auto-blocked, and ADR-0008's own Ordering Note states ADR-0004/0006 should not be Accepted until ADR-0008 is. All three ADRs (0004, 0006, 0008) remain Proposed — none can currently back a story.

C-1 as originally framed ("defined by none") is closed. The residual gate is ordinary status-lifecycle progression, not a design conflict.

---

## Recommended Next Actions

1. Review and Accept ADR-0008 (similar Accept pass to ADR-0005 — confirm no unresolved dependency, requirements TR-hp-006/007/008 addressed)
2. Once ADR-0008 is Accepted, ADR-0004 and ADR-0006 become eligible for their own Accept review
3. Re-run full `/architecture-review` after ADR-0008 (and ideally 0004/0006) reach Accepted — to flip TR-hp-006/007/008 to ✅ in the traceability matrix and confirm no new conflicts were introduced by the citation changes made in this pass

---

## Files touched this session (for this recheck)

- `docs/architecture/0002-checkpoint-persistence.md` — `ResetDeathState()` integration (prior turn)
- `docs/architecture/0004-luna-overdrive-fixed-window.md` — `Depends On` + Risks updated to cite ADR-0008
- `docs/architecture/0006-enemy-ai-behavior-tree.md` — `Depends On` + Related Decisions updated to cite ADR-0008
- `docs/architecture/architecture-review-2026-07-27-c1-recheck.md` — this report
