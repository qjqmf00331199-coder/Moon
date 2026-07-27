# Architecture Review — 2026-07-27 (v4, post-ADR-0010/0011)

- **Date:** 2026-07-27
- **Engine:** Unreal Engine 5.8 (pinned 2026-07-16, post-LLM-cutoff — HIGH risk)
- **Mode:** `full` delta review — fresh reviewer pass against the latest pushed commit
  `9838a1b` after ADR-0010 and ADR-0011 landed.
- **Baseline:** `architecture-review-2026-07-27-v3.md` — CONCERNS, 52 covered / 11 partial / 13 gaps of 76.
- **ADRs Reviewed:** 11 (0001-0011). ADR-0001 through ADR-0009 were already Accepted at v3;
  ADR-0010 and ADR-0011 are reviewed in this pass and accepted by this change.

---

## Verdict: PASS

ADR-0010 closes the remaining Presentation-layer gap group: TR-hud-001..007 and
TR-dash-008. ADR-0011 closes the remaining Feature-layer gap group:
TR-tension-002/003/004/005/007. No new blocking cross-ADR conflict, unresolved
dependency, or deprecated-API contradiction was introduced by either ADR.

The project now has zero uncovered architecture requirements. Existing
non-blocking concerns remain: GDD revision flags C-2/C-3, `AMoonCharacterBase`
responsibility accretion, implementation-time UE5.8 verification items, and
stale `docs/architecture/architecture.md` bookkeeping.

---

## Traceability Summary

| | v3 | v4 | Delta |
|---|---:|---:|---:|
| Total active requirements | 76 | 76 | 0 |
| Covered | 52 | 65 | +13 |
| Partial | 11 | 11 | 0 |
| Gaps | 13 | 0 | -13 |

### Gap Closures

| Requirement group | v3 status | v4 coverage | Governing ADR |
|---|---|---|---|
| Combat HUD: TR-hud-001..007 | 7 gaps | Covered | ADR-0010 |
| Dash HUD surface: TR-dash-008 | 1 gap | Covered | ADR-0010 |
| Combo/Tension Gauge: TR-tension-002/003/004/005/007 | 5 gaps | Covered | ADR-0011 |

### Coverage by System

| System | Covered | Partial | Gaps | Governing ADR |
|---|---:|---:|---:|---|
| Health/Damage Core | 9 | 0 | 0 | ADR-0001, ADR-0002, ADR-0008 |
| Player Movement | 8 | 2 | 0 | ADR-0001, ADR-0005, ADR-0009 |
| Camera System (base) | 7 | 2 | 0 | ADR-0005 |
| Luna Overdrive | 7 | 1 | 0 | ADR-0004 |
| Enemy AI (base) | 6 | 2 | 0 | ADR-0006 |
| Spell Casting (base) | 7 | 3 | 0 | ADR-0003, ADR-0004 |
| Dash/Evasion | 6 | 2 | 0 | ADR-0007, ADR-0010 |
| Combo/Tension Gauge | 6 | 1 | 0 | ADR-0001, ADR-0004, ADR-0011 |
| Combat HUD | 7 | 0 | 0 | ADR-0010 |

---

## ADR Dependency Order

All 11 ADRs are now Accepted. No unresolved dependencies or cycles were found.

```
Foundation:
  1. ADR-0001  Player Movement and GAS Core                 [Accepted]
  2. ADR-0002  Checkpoint Persistence                       [Accepted]
  3. ADR-0003  Spell Casting GAS Implementation             [Accepted]
  4. ADR-0008  Health/Damage Core Death Contract            [Accepted]
  5. ADR-0009  Player Movement Runtime Contract             [Accepted]

Depends on Foundation:
  6. ADR-0004  Luna Overdrive Fixed Window                  [Accepted]
  7. ADR-0006  Enemy AI Behavior Tree                       [Accepted]
  8. ADR-0007  Dash/Evasion Just-Dodge                      [Accepted]
  9. ADR-0005  Camera System SpringArm                      [Accepted]

Feature / Presentation:
 10. ADR-0011  Combo/Tension Gauge Ordering and Wiring      [Accepted]
 11. ADR-0010  Combat HUD Widget Architecture               [Accepted]
```

---

## Cross-ADR Conflicts

- **C-1:** Resolved since v2 by ADR-0008; still clean. ADR-0010 and ADR-0011 both
  consume ADR-0008's `OnDeath`/Health delegate contracts without redefining them.
- **C-2:** Still open, non-blocking GDD issue — `dash-evasion.md` Rule 4 velocity
  wording conflicts with Rule 2/ADR-0007's position-step model.
- **C-3:** Still open, non-blocking GDD issue — `camera-system-base.md`
  `CameraLagMaxDistance` 60uu vs 200uu contradiction.
- **C-4:** Still a non-blocking architecture concern — more state remains on
  `AMoonCharacterBase`; ADR-0011 adds a second tick entry point and names this risk.

No new blocking conflict was introduced by ADR-0010 or ADR-0011.

---

## Engine Compatibility

No new deprecated API reference was introduced.

- ADR-0010 has one implementation-time UE5.8 verification item:
  `UCommonInputSubsystem` input-method signal and glyph asset source of truth after
  Enhanced Input/Common Input unification.
- ADR-0011 has two implementation-time verification items: no project gain trigger
  runs later than `TG_PostUpdateWork` in the same frame, and
  `FOnAttributeChangeData::GEModData` null/non-null semantics match the proposed
  damage-filter usage.

These are non-blocking for Acceptance under existing project precedent
(ADR-0004/0006/0008/0009), but they must be verified before implementation stories
using those exact APIs are marked Ready.

---

## GDD Revision Flags

Unchanged from v3 and still non-blocking for architecture PASS:

| GDD | Issue | Status |
|---|---|---|
| `dash-evasion.md` | Rule 4 velocity wording vs Rule 2/ADR-0007 | Still unrevised |
| `camera-system-base.md` | Formula 3 / Arena Morphing 200uu vs 60uu | Still unrevised |
| `dash-evasion.md` | Missing `JustDodgeQueryRadius` / `AirDashZImpulse` knobs | Still unrevised |
| `enemy-ai-base.md` | Missing `MeleeAttackRange` knob | Still unrevised |
| `dash-evasion.md` | Stale Last-Updated / re-review status | Still unrevised |

---

## Pre-Gate Checklist

| Item | Status |
|---|---|
| `tests/unit/` | Partial — `.gitkeep` only |
| `tests/integration/` | Partial — `.gitkeep` only |
| `.github/workflows/tests.yml` | Exists |
| `design/accessibility-requirements.md` | Exists |
| `design/ux/interaction-patterns.md` | Exists |

No missing pre-gate checklist file blocks the next architecture gate action.

---

## Blocking Issues

None for architecture coverage. Requirement gaps are now 0.

## Required ADRs

None currently required for MVP architecture coverage. Next work is follow-up
cleanup and implementation readiness, not gap-closing ADR authoring.

## Recommended Next Actions

1. Refresh `docs/architecture/architecture.md` so the master architecture document
   reflects ADR-0001 through ADR-0011 and the PASS state.
2. Run a focused GDD cleanup pass for C-2/C-3 and the missing tuning knobs.
3. Verify ADR-0010/0011 implementation-time UE5.8 items before marking dependent
   implementation stories Ready.
4. Run `/gate-check pre-production` when ready for the next formal phase gate.
