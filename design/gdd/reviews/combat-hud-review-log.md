# Review Log — combat-hud.md

## Review — 2026-07-18 — Verdict: APPROVED
Scope signal: L
Specialists: (lean review — solo pass, no specialist delegation)
Blocking items: 0 | Recommended: 0
Summary: 8/8 sections complete. All 7 widgets bind only to interfaces upstream GDDs already expose
via their own UI Requirements sections — no new interface invented. W7 execute-prompt binding to
Core Extraction Execution (not yet designed) is explicitly documented as an assumed interface with
a handoff note (Rule 6, Open Questions) — correctly non-blocking. Coalescing (Rule 4) and lazy
real-value gating for gameplay-meaningful signals (Rule 5) are unambiguous and consistent with the
Health/Damage Core "no judgment uncertainty" philosophy this doc cites. Dependency graph confirmed
bidirectional for all 5 designed upstream systems. Layout/style is correctly deferred to
/ux-design + Art Bible (Rule 10) — not this doc's scope.
Prior verdict resolved: First review

## Review — 2026-07-20 — Verdict: APPROVED (full depth re-read)
Scope signal: M
Specialists: Antigravity (game-designer, systems-designer, ux-designer, qa-lead multi-perspective;
solo pass following AGENTS.md design-review guidelines)
Blocking items: 0 | Recommended: 2
Summary: Full GDD re-read against W3 cross-review finding. No blocking items. Status header was
"Needs Revision" due to the 2026-07-18 cross-review W3 finding (reciprocity asymmetry).

W3 assessment: Combat HUD's own Dependencies table correctly declares all upstream systems as hard
dependencies. The reciprocity gap (HDC and Spell Casting not listing Combat HUD as downstream in
their Dependencies tables) is a problem in THOSE GDDs, not here. This GDD is correctly constructed
and cannot self-fix a gap in upstream documents. Deferred to a follow-up minor fix pass on HDC and
spell-casting-base.md. Non-blocking for Combat HUD Approved status.

Non-blocking findings (deferred):
1. No AC for HUD initialization hiding (Edge Cases: show nothing until first delegate received).
   Implementation guidance is in Edge Cases — AC would strengthen QA coverage but not blocking.
2. No AC for LowHealthWarning threshold up/down crossing in same frame. Non-blocking.

All 12 existing ACs verified against GDD rules — all consistent.
Prior verdict resolved: Yes — status updated from "Needs Revision" to "Approved"

