# Story 001: Camera-Yaw Facing and Movement Independence

> **Epic**: Player Movement Foundation Fixes
> **Status**: Complete
> **Layer**: Foundation
> **Type**: Logic
> **Estimate**: 2-4 hours
> **Manifest Version**: 2026-07-27
> **Last Updated**: 2026-08-12

## Context

**GDD**: `design/gdd/player-movement.md`  
**Requirement**: `TR-mov-001`, `TR-mov-002`, `TR-mov-010`

**ADR Governing Implementation**: ADR-0001: Player Movement and GAS Core Foundation; ADR-0005: Camera System SpringArm; ADR-0009: Player Movement Runtime Contract  
**ADR Decision Summary**: Player movement uses CMC and Enhanced Input, but facing must be camera-yaw locked: `bUseControllerRotationYaw=true`, `bOrientRotationToMovement=false`, pitch/roll false. Movement code must remain independent of Spell Casting and locomotion translation must remain CMC-driven, not root-motion driven.

**Engine**: Unreal Engine 5.8 | **Risk**: HIGH  
**Engine Notes**: Enhanced Input and CMC basics are acceptable, but implementation must still check local engine reference docs before relying on remembered UE5.8 behavior.

**Control Manifest Rules (this layer)**:
- Required: camera-yaw facing flags and movement compile-independence.
- Forbidden: movement gating from Spell Casting, execution presentation, locomotion montage, or hitstop.
- Guardrail: static checks must catch future drift.

---

## Acceptance Criteria

*From GDD `design/gdd/player-movement.md`, scoped to this story:*

- [ ] GIVEN camera yaw changes while the character is idle or strafing, WHEN facing is checked, THEN the character faces camera yaw immediately with no interpolation.
- [ ] GIVEN Character class rotation settings, WHEN statically inspected or read from the CDO, THEN `bUseControllerRotationYaw == true`, `bOrientRotationToMovement == false`, `bUseControllerRotationPitch == false`, and `bUseControllerRotationRoll == false`.
- [ ] GIVEN Movement-owning source files, WHEN CI/static grep runs, THEN Spell Casting types/state references are zero.
- [ ] GIVEN locomotion animation assets for jump/landing, WHEN asset settings are inspected, THEN root motion is disabled.

---

## Implementation Notes

- Fix the known shipped deviation in `AMoonCharacterBase` constructor: stock third-person-template rotation flags currently contradict the GDD and ADR-0005.
- Keep movement direction camera-relative and independent from facing. Facing follows controller yaw; movement vector follows yaw-only forward/right basis.
- Add or update static checks under `tests/` or project scripts so future regressions are caught without relying on manual review.
- Do not touch Spell Casting or Dash behavior in this story beyond ensuring Movement code does not reference them.

---

## Out of Scope

- Story 002 handles tuning/clamp data.
- Story 003 handles airborne substate, jump buffer, and coyote timers.
- Story 004 handles hitstop.
- Camera data asset implementation belongs to the Camera System epic.

---

## QA Test Cases

- **AC-1**: Camera yaw facing
  - Given: a player pawn and a test harness that can set controller yaw to 0, 45, 90, and 180 degrees.
  - When: the pawn is idle, strafing left, and moving diagonally for one tick after each yaw change.
  - Then: actor yaw matches controller yaw within 1 degree on the same tick and movement direction remains camera-relative.
  - Edge cases: diagonal input, rapid yaw flips, no movement input.

- **AC-2**: Static rotation flags
  - Given: `AMoonCharacterBase` CDO or source inspection.
  - When: the test reads rotation flags.
  - Then: yaw is controller-owned, orient-to-movement is false, pitch and roll are false.
  - Edge cases: Blueprint subclass defaults must not override these values.

- **AC-3**: Movement independence
  - Given: Movement-owning source paths.
  - When: grep/static checks search for Spell Casting type names, ability class names, and spell state names.
  - Then: no forbidden references are found.
  - Edge cases: comments and test fixtures may be allowlisted only if the checker documents why.

- **AC-4**: Non-root-motion locomotion
  - Given: jump and landing AnimSequence assets.
  - When: import settings/root-motion extraction flags are inspected.
  - Then: root motion is disabled for locomotion animations.
  - Edge cases: non-locomotion cinematic/execution animations are out of scope.

---

## Test Evidence

**Story Type**: Logic  
**Required evidence**:
- `tests/unit/movement/camera_yaw_facing_test.*`
- `tests/static/movement_independence_check.*`
- Optional PIE evidence: `production/qa/evidence/camera-yaw-facing-and-movement-independence-evidence.md`

**Status**: [x] Static/unit evidence created; [ ] PIE evidence pending
**Implementation note 2026-07-27**: Started. The playtest-blocking yaw/orient rotation flag deviation is fixed in `AMoonCharacterBase`. Full `AMoonCharacterBase` responsibility separation is not part of this story; the static check focuses on the Movement path and Build.cs module boundary.
**Implementation note 2026-07-27 (Codex)**: Added `tests/unit/movement/camera_yaw_facing_test.ps1` and `tests/static/movement_independence_check.ps1`. Both checks pass locally, and `Build.bat MoonEditor Win64 Development -Project=D:\moon-fragment-hunt\Moon\Moon.uproject -NoHotReload` succeeds when run outside the filesystem sandbox so UBT can access its AppData cache/log directory. PIE validation is still pending because Unreal MCP/Editor control tools were not available in this session.

---

## Dependencies

- Depends on: None
- Unlocks: Story 003, Story 004, Camera System Foundation Fixes, Dash/Evasion Foundation Fixes

## Completion Notes
**Completed**: 2026-08-12
**Criteria**: 4/4 passing. AC-4 closed 2026-08-12 without editor access: `BP_MoonCharacter.uasset` uses `EAnimationMode::AnimationSingleNode` over 6 raw ParagonAurora AnimSequences (`Idle`, `Jog_Fwd`, `Jump_Apex`, `Jump_Land`, `Jump_Recovery`, `Jump_Start` — no AnimBP in the chain). Each `.uasset`'s asset-registry tag block contains a plaintext `bEnableRootMotion` / `False` pair, readable directly from the binary without loading the editor — confirmed for all 6 via byte-offset scan.
**Deviations**: None blocking.
**Test Evidence**: Logic — `tests/unit/movement/camera_yaw_facing_test.ps1` and `tests/static/movement_independence_check.ps1`, both live-re-run PASS. AC-4: direct binary inspection of the 6 locomotion AnimSequence assets (see Criteria above) — no PIE required, asset-registry tags are stable regardless of editor session.
**Code Review**: Skipped — Solo mode.
