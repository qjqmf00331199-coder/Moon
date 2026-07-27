# Architecture Review — 2026-07-27 (v3, post-ADR-0009-Accept)

- **Date:** 2026-07-27
- **Engine:** Unreal Engine 5.8 (pinned 2026-07-16, post-LLM-cutoff — HIGH risk)
- **Mode:** `full` — delta-verified against [architecture-review-2026-07-27-v2.md](architecture-review-2026-07-27-v2.md).
  Diffed the repo between the v2 commit (`7288213`) and HEAD: the only change is
  `docs/architecture/0009-player-movement-runtime-contract.md` Status field
  `Proposed` → `Accepted`. No GDD text, no other ADR, no registry, no test
  infrastructure changed. This pass verifies that one flip and re-derives
  only what it affects — not a blank-slate re-run.
- **ADRs Reviewed:** 9 (0001–0009), all now Accepted
- **Previous review:** [architecture-review-2026-07-27-v2.md](architecture-review-2026-07-27-v2.md) — verdict CONCERNS, 13 gaps / 76

---

## Verdict: 🟡 CONCERNS (unchanged from v2)

Foundation layer remains **100% covered**. The only thing this pass changes is
that ADR-0009 — already scored as covering its 7 TRs at v2 despite being
Proposed — is now formally Accepted, so the last non-gap blocking item from v2
("ADR-0009 is Proposed") is resolved. Requirement coverage counts do not move:
v2 already counted ADR-0009's Decision text as coverage regardless of status.
Remaining 13 gaps are still entirely Feature/Presentation layer (Combo/Tension
Gauge, Combat HUD) — unaffected by this change, so the verdict stays CONCERNS,
not PASS.

| Criterion | v2 | v3 (this pass) |
|---|---|---|
| All requirements covered | ❌ 13 gaps / 76 | ❌ 13 gaps / 76 (unchanged) |
| No blocking cross-ADR conflicts | ✅ C-1 resolved; C-2/C-3 non-blocking | ✅ unchanged |
| Foundation layer complete | ✅ Player Movement 8/10✅+2⚠️, Health/Damage Core 9/9 | ✅ unchanged |
| Engine consistency | ✅ E-1 fixed; E-2/E-3 non-blocking | ✅ unchanged |
| Pre-gate checklist | ✅ no ❌ items | ✅ unchanged |
| **All 9 ADRs Accepted** | ❌ ADR-0009 Proposed | ✅ **now true — 9/9 Accepted** |

`/gate-check pre-production` remains offerable, now with zero ADRs left in
`Proposed` state anywhere in the project.

---

## Traceability Summary

| | v2 (2026-07-27) | v3 (this pass) | Δ |
|---|---|---|---|
| Total active requirements | 76 | 76 | 0 |
| ✅ Covered | 52 (68%) | 52 (68%) | 0 |
| ⚠️ Partial | 11 (15%) | 11 (15%) | 0 |
| ❌ Gap | 13 (17%) | 13 (17%) | 0 |

No TR flips this pass — ADR-0009's Accept did not change what its Decision
text covers, only its status field. (v2's own note: "it is scored as covering
its 7 TRs even though its own Status field is still Proposed... coverage =
'an ADR's Decision text addresses this,' not 'and is Accepted.'" That caveat
is now moot — the ADR is Accepted.)

### Coverage by system (unchanged from v2)

| System | ✅ | ⚠️ | ❌ | Governing ADR |
|---|---|---|---|---|
| Health/Damage Core | 9 | 0 | 0 | ADR-0001, ADR-0002, ADR-0008 |
| Player Movement | 8 | 2 | 0 | ADR-0001, ADR-0005, ADR-0009 |
| Camera System (base) | 7 | 2 | 0 | ADR-0005 |
| Luna Overdrive | 7 | 1 | 0 | ADR-0004 |
| Enemy AI (base) | 6 | 2 | 0 | ADR-0006 |
| Spell Casting (base) | 7 | 3 | 0 | ADR-0003, ADR-0004 |
| Dash/Evasion | 5 | 2 | 1 | ADR-0007 |
| Combo/Tension Gauge | 1 | 1 | 5 | ADR-0001 (attribute only), ADR-0004 (lock only) — **no ADR for this system's own scope** |
| Combat HUD | 0 | 0 | 7 | **none** |

---

## Remaining Coverage Gaps (13) — all Feature/Presentation layer, unchanged from v2

### Combo/Tension Gauge (5, no ADR exists)
TR-tension-002, 003, 004, 005 (ADR-0004's upstream `OnOverdriveTriggered` producer — still unspecified by any ADR), 007.
→ `/architecture-decision Combo/Tension Gauge`

### Combat HUD (7) + Dash HUD surface (1), no ADR exists
TR-hud-001..007, plus TR-dash-008. `design/ux/combat-hud.md` exists as a UX spec head-start; no ADR yet.
→ `/architecture-decision Combat HUD`

---

## Cross-ADR Conflicts — unchanged from v2

- ✅ **C-1** — resolved (ADR-0008 producer, all consumers cite it correctly). No further action.
- 🟠 **C-2** — `dash-evasion.md` Rule 4 vs Rule 2/ADR-0007 (velocity-override wording vs position-step model) — still open, GDD-fix not ADR-fix.
- 🟡 **C-3** — `camera-system-base.md` 60uu vs 200uu self-contradiction — still open, GDD-fix not ADR-fix.
- 🟡 **C-4** — `AMoonCharacterBase` responsibility accretion (5 ADRs now: 0001/0004/0005/0007/0009) — still a non-blocking concern, unchanged by this pass (ADR-0009's surface was already counted at v2).

---

## ADR Dependency Order — refreshed, now fully Accepted

```
Foundation (no unresolved deps) — ALL ACCEPTED:
  1. ADR-0001  Player Movement and GAS Core          [Accepted]
  2. ADR-0002  Checkpoint Persistence                [Accepted]
  3. ADR-0003  Spell Casting GAS Implementation      [Accepted]
  4. ADR-0008  Health/Damage Core Death Contract     [Accepted]
  5. ADR-0009  Player Movement Runtime Contract      [Accepted]  ← flipped this pass (was Proposed at v2)

Depends on Foundation — ALL ACCEPTED:
  6. ADR-0004  Luna Overdrive Fixed Window            [Accepted]
  7. ADR-0006  Enemy AI Behavior Tree                 [Accepted]
  8. ADR-0007  Dash/Evasion Just-Dodge                [Accepted]
  9. ADR-0005  Camera System (SpringArm)               [Accepted]
```

**Unresolved dependency flags:** none. **Cycles:** none. **Proposed ADRs remaining: 0** (was 1 at v2).
Stories may now reference any of the 9 ADRs without being status-blocked.

---

## GDD Revision Flags — unchanged from v2 (still open, non-blocking)

| GDD | Issue | Status |
|---|---|---|
| `dash-evasion.md` Rule 4 | velocity-override wording vs Rule 2/ADR-0007 | ❌ still unrevised (C-2) |
| `camera-system-base.md` Formula 3 / Arena Morphing row | 200uu vs 60uu | ❌ still unrevised (C-3) |
| `dash-evasion.md` Tuning Knobs | missing `JustDodgeQueryRadius`/`AirDashZImpulse` | ❌ still not added |
| `enemy-ai-base.md` Tuning Knobs | missing `MeleeAttackRange` | ❌ still not added |
| `dash-evasion.md` header | stale Last-Updated | ❌ still not re-reviewed |

None block Acceptance of any ADR (unchanged reasoning from v2).

---

## Engine Compatibility Audit — unchanged from v2

E-1 fixed (v2 session). E-2 (missing `deprecated-apis.md` GAS entries) and E-3 (unverifiable-by-reference
claims in ADR-0004/0006, both self-flagged) remain open, both non-blocking. No new engine-relevant
content was introduced by this pass — ADR-0009's Engine Compatibility section is unchanged from v2.

---

## Pre-Gate Checklist — unchanged from v2

| Item | Status |
|---|---|
| `tests/unit/` | ⚠️ `.gitkeep` only |
| `tests/integration/` | ⚠️ `.gitkeep` only |
| `.github/workflows/tests.yml` | ✅ exists |
| `design/accessibility-requirements.md` | ✅ exists |
| `design/ux/interaction-patterns.md` | ✅ exists |

No ❌ items. `/gate-check pre-production` remains offerable.

---

## Blocking Issues (must resolve before PASS)

1. **Combo/Tension Gauge** — 5 of 7 uncovered, including TR-tension-005 (ADR-0004's upstream event producer, still unspecified).
2. **Combat HUD** — 0 of 7 covered, no ADR (UX spec exists).

~~3. ADR-0009 is Proposed~~ — **RESOLVED this pass.**

## Non-blocking but should be fixed in the same pass

3. C-2 / C-3 — two Approved GDDs each still self-contradict (unchanged).
4. `dash-evasion.md` / `enemy-ai-base.md` Tuning Knobs additions — still not added (unchanged).
5. `architecture.md` — bookkeeping stale by 8 ADRs (unchanged from v2, still needs its own refresh pass).
6. E-2 — engine-reference GAS deprecation entries still missing (unchanged).

---

## Required ADRs (prioritised, unchanged from v2)

| # | ADR | Layer | Closes |
|---|---|---|---|
| 1 | **Combo/Tension Gauge** | Feature | TR-tension-002/003/004/005/007 |
| 2 | **Combat HUD** | Presentation | TR-hud-001..007, TR-dash-008 |

Foundation layer requires no new ADRs — fully covered and fully Accepted.

---

## Recommended Next Actions

1. `/architecture-decision Combo/Tension Gauge` — last Feature-layer gap
2. `/architecture-decision Combat HUD` — last Presentation-layer gap (leverage `design/ux/combat-hud.md`)
3. `/quick-design` pass on `dash-evasion.md` (C-2, Tuning Knobs) and `camera-system-base.md` (C-3)
4. Refresh `docs/architecture/architecture.md` — bookkeeping only
5. Land the E-2 background task (`deprecated-apis.md` GAS entries)
6. Once items 1–2 land, re-run `/architecture-review` — first PASS-eligible state
