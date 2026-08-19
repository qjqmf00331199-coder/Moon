# Sprint 1 — 2026-08-14 to 2026-08-21

## Sprint Goal
Camera System Foundation Fixes epic story 생성 + 구현, ADR-0005 데이터 기반 계약으로 전환.

## Capacity
- Total days: 5
- Buffer (20%): 1 day (unplanned work)
- Available: 4 days

## Tasks

### Must Have (Critical Path)
| ID | Task | Agent/Owner | Est. Days | Dependencies | Acceptance Criteria |
|----|------|-------------|-----------|-------------|-------------------|
| S1-01 | Run `/create-stories camera-system-foundation-fixes` | self / producer | 0.5 | - | Story files created under `production/epics/camera-system-foundation-fixes/`, each passes story-readiness READY |
| S1-02 | Implement camera-001..009 via `/dev-story` (real breakdown, see `production/sprint-status.yaml`) | ue-blueprint-specialist / unreal-specialist | ~2.25 (sum of 9 story estimates) | S1-01 | Camera QA-TEST-01 through QA-TEST-10 executable in PIE; all runtime camera tuning sourced from `UMoonCameraSettings` |

### Should Have
None — dash-evasion epic requires camera-facing validation complete first (out of scope this sprint per `production/epics/dash-evasion-foundation-fixes/EPIC.md`).

### Nice to Have
None

## Carryover from Previous Sprint
None — Player Movement Foundation Fixes epic (5 stories) fully Complete.

## Risks
| Risk | Probability | Impact | Mitigation |
|------|------------|--------|------------|
| TR-cam-003/TR-cam-004 have partial (⚠️) ADR-0005 coverage | Medium | Medium | Surface gap explicitly in story acceptance criteria during `/create-stories`; amend ADR-0005 if needed |
| Story count/estimates unknown until `/create-stories` runs | Medium | Low | Re-run `/sprint-plan update` after S1-01 to reflect real per-story estimates |

## Dependencies on External Factors
- Player Movement story 001 complete — satisfied ✅
- Camera-facing validation is itself the prerequisite for the dash-evasion epic (next sprint), not an external blocker for this sprint

## Definition of Done for this Sprint
- [ ] All Must Have tasks completed
- [ ] All tasks pass acceptance criteria
- [ ] QA plan exists (`production/qa/qa-plan-sprint-1.md`)
- [ ] All Logic/Integration stories have passing unit/integration tests
- [ ] Smoke check passed (`/smoke-check sprint`)
- [ ] QA sign-off report: APPROVED or APPROVED WITH CONDITIONS (`/team-qa sprint`)
- [ ] No S1 or S2 bugs in delivered features
- [ ] Design documents updated for any deviations
- [ ] Code reviewed and merged

> **Scope check:** If this sprint includes stories added beyond the original epic scope, run `/scope-check camera-system-foundation-fixes` to detect scope creep before implementation begins.
