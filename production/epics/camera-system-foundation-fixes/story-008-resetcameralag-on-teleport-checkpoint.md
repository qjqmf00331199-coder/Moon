# Story 008: ResetCameraLag on Teleport/Checkpoint

> **Epic**: Camera System Foundation Fixes
> **Status**: Ready
> **Layer**: Core
> **Type**: Integration
> **Estimate**: 1.5h
> **Manifest Version**: 2026-07-27
> **Last Updated**: (set by /dev-story when implementation begins)

## Context

**GDD**: `design/gdd/camera-system-base.md`
**Requirement**: `TR-cam-007`
*(Requirement text lives in `docs/architecture/tr-registry.yaml` — read fresh at review time)*

**ADR Governing Implementation**: ADR-0005: Camera System (base) — SpringArm + Data-Driven Config
**ADR Decision Summary**: Respawn/teleport hook (Edge Case 2) is a cross-referenced obligation on ADR-0002 (`UMoonCheckpointSubsystem`) — whichever system handles checkpoint restore must call `CameraBoom->ResetCameraLag()`. This ADR does not own the checkpoint system itself.

**Engine**: Unreal Engine 5.8 | **Risk**: LOW
**Engine Notes**: `USpringArmComponent::ResetCameraLag()` is a stable pre-cutoff API.

**Control Manifest Rules (this layer)**:
- Required: `ResetCameraLag()` must be called on teleport/checkpoint respawn paths — source: ADR-0005.

---

## Acceptance Criteria

*From GDD `design/gdd/camera-system-base.md`, scoped to this story (QA-TEST-06):*

- [ ] `CameraBoom->ResetCameraLag()` is called on the character's checkpoint-respawn / teleport code path
- [ ] After a teleport of `≥1000uu`, the camera is synchronized to the new position with zero frames of sweep artifact — no visible slide through world geometry from the old position to the new one
- [ ] This story confirms (does not re-implement) the checkpoint subsystem's call site — check `UMoonCheckpointSubsystem` (ADR-0002) first for an existing hook before adding a new one

---

## Implementation Notes

*Derived from ADR-0005 Key Interfaces section:*

"Respawn/teleport hook (Edge Case 2): whichever system handles checkpoint restore (ADR-0002 `UMoonCheckpointSubsystem`) must call `CameraBoom->ResetCameraLag()` — cross-referenced obligation on that existing ADR, not a new one."

Read `UMoonCheckpointSubsystem`'s current restore path first — if the Player Movement epic or an earlier pass already added this call, this story is a verification-only story (confirm + QA-TEST-06 evidence), not new code.

---

## Out of Scope

*Handled by neighbouring stories — do not implement here:*

- Story 004: `CameraLagMaxDistance` hard-follow (different scenario — sustained pursuit, not instantaneous teleport)
- `UMoonCheckpointSubsystem`'s own restore logic (ADR-0002, not this epic)

---

## QA Test Cases

*Written by /qa-plan sprint (2026-08-14). The developer implements against these — do not invent new test cases during implementation.*

- **AC-1**: `ResetCameraLag()` call site exists on respawn path
  - Given: `UMoonCheckpointSubsystem`'s restore path (ADR-0002) executes
  - When: the character's Transform is restored to the checkpoint
  - Then: `CameraBoom->ResetCameraLag()` is called on the same frame
  - Edge cases: if the call site already exists from an earlier pass, this test only needs to confirm presence — do not add a duplicate call

- **AC-2 (QA-TEST-06)**: no sweep artifact on ≥1000uu teleport
  - Given: character teleports/respawns `≥1000uu` from its prior position
  - When: the first frame after `ResetCameraLag()` renders
  - Then: camera is synchronized to the new position immediately — zero frames of visible sweep/slide through world geometry
  - Edge cases: teleport happening while the character was mid-dash (camera lag cache already perturbed) must still reset cleanly

- **AC-3**: reset does not affect non-teleport lag behavior
  - Given: normal gameplay movement (no teleport) is occurring
  - When: `ResetCameraLag()` is NOT called
  - Then: camera lag behaves per Story 004 (normal `CameraLagMaxDistance` hard-follow), unaffected by this story's addition

---

## Test Evidence

**Story Type**: Integration
**Required evidence**:
- Integration: `tests/integration/camera/reset-camera-lag_test.cpp` OR documented playtest

**Status**: [ ] Not yet created

---

## Dependencies

- Depends on: Story 001 (`CameraBoom` exists with lag enabled), existing `UMoonCheckpointSubsystem` (ADR-0002)
- Unlocks: None
