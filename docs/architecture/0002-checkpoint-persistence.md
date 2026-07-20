# ADR 0002: Runtime Checkpoint Persistence Strategy

**Status:** Accepted  
**Date:** 2026-07-18  
**Accepted:** 2026-07-20 (design verified consistent with ADR-0001 implementation; `UMoonCheckpointSubsystem` not yet implemented — accepted as the agreed design, implementation pending)  
**Context:** Moon Fragment Hunt (UE 5.8)

## Scope Note

This ADR covers **in-session runtime checkpoint state only** — the snapshot/restore
needed for `health-damage-core.md`'s "death = instant checkpoint respawn" rule.
It does **not** cover disk-persisted SaveGame (profile progression, unlocks,
settings, cross-session saves). That category is explicitly out of scope for
the current combat-system-expansion index (`systems-index.md`: "Progression/
Economy/Persistence/Narrative/Audio 카테고리는 이 확장 범위 밖"). A full SaveGame
ADR should be authored separately once a Persistence GDD exists.

## Engine Compatibility

| Field | Value |
|-------|-------|
| **Engine** | Unreal Engine 5.8 |
| **Domain** | Core |
| **Knowledge Risk** | HIGH (project pinned post-LLM-cutoff per `docs/engine-reference/unreal/VERSION.md`) — no SaveGame/Subsystem-specific breaking changes found in `breaking-changes.md` or `deprecated-apis.md` at time of writing |
| **References Consulted** | `docs/engine-reference/unreal/VERSION.md`, `breaking-changes.md`, `deprecated-apis.md` (grepped for Save/Serialize/Subsystem — no hits) |
| **Post-Cutoff APIs Used** | None — `UGameInstanceSubsystem` and GAS attribute snapshotting are pre-5.4 stable APIs |
| **Verification Required** | (1) Confirm which mechanism Arena Morphing actually uses for boss-arena transitions — true level travel (`OpenLevel`, subsystem persists by design) vs. World Partition streaming/Data Layers within one persistent level (subsystem question moot, but in-place actor destruction/GC during streaming is a different risk) — before this ADR is relied on for boss-fight checkpoints. (2) Confirm `UMoonAttributeSet`'s initialization pattern (from ADR-0001) does not use the legacy GAS attribute-set init functions flagged as deprecated in UE 5.8 (`deprecated-apis.md`, "5.8 Additions") — this ADR's restore path assumes the updated 5.8 init pattern. |

## ADR Dependencies

| Field | Value |
|-------|-------|
| **Depends On** | ADR-0001 (Player Movement and GAS Core Foundation) — needs `UMoonAttributeSet` and `AMoonCharacterBase` to exist |
| **Enables** | Core Extraction Execution, Boss Phase, Arena Morphing (all rely on checkpoint/respawn continuity) |
| **Blocks** | None — Health/Damage Core respawn rule can currently rely on simple level restart; this ADR upgrades that to mid-level checkpointing |
| **Ordering Note** | Should land before Boss Phase / Arena Morphing implementation, since those introduce multi-stage encounters where a naive "restart level" respawn breaks pacing |

## Context

### Problem Statement
`health-damage-core.md` mandates: on player death, the game performs an **instant
checkpoint respawn** — not a full level reload. This requires the engine to hold
a snapshot of player state (position, HP/Mana/Tension via `UMoonAttributeSet`,
active status effects) captured at the last checkpoint trigger, and to restore
it on death without any disk I/O or loading screen.

### Constraints
- Must be instant (no loading screen) — respawn breaks combat flow/tension curve if slow
- Must not require a SaveGame/disk write — this is transient, in-run state only
- Must survive within a single play session, including boss-arena level transitions (Arena Morphing)
- Must not leak progression/persistence scope creep into this ADR

### Requirements
- Capture checkpoint snapshot on designer-placed checkpoint triggers (and boss-phase transitions)
- Restore player Transform + `UMoonAttributeSet` values (Health, Mana, TensionGauge) on death
- Clear/replace snapshot on next checkpoint (only one active checkpoint at a time — no checkpoint history/undo)

## Decision

**Store the checkpoint snapshot in a `UGameInstanceSubsystem` (`UMoonCheckpointSubsystem`), in memory only, no disk serialization.**

1. **`UMoonCheckpointSubsystem : public UGameInstanceSubsystem`**
   - Owns a single `FMoonCheckpointSnapshot` struct (not a `USaveGame` object — no
     `UGameplayStatics::SaveGameToSlot` call anywhere in this system).
   - `FMoonCheckpointSnapshot` fields: `FTransform PlayerTransform`, a copy of the
     relevant `UMoonAttributeSet` base values (Health, Mana, TensionGauge), and an
     array of active `FGameplayTag` status effects to reapply.

2. **Capture**: A `CheckpointTrigger` actor (Blueprint-placeable) calls
   `UMoonCheckpointSubsystem::CaptureCheckpoint(APawn* Player)` on overlap. Boss
   Phase transitions (per `systems-index.md` dependency chain) call the same
   entry point at phase-start.

3. **Restore**: `AMoonCharacterBase`'s death handling (already gated through the
   `UGameplayEffectExecutionCalculation` damage pipeline per ADR-0001) calls
   `UMoonCheckpointSubsystem::RestoreCheckpoint(APawn* Player)` instead of
   triggering a level reload. Restore re-applies Transform and re-initializes
   `UMoonAttributeSet` from the snapshot via `GameplayEffect` (never a direct
   attribute set-value, per GAS convention).

4. **Subsystem lifetime**: `UGameInstanceSubsystem` persists across level travel
   within the same game instance, which covers Arena Morphing's boss-arena
   transitions without extra plumbing. It is destroyed on full game-instance
   teardown (return to main menu / app close) — by design, since this is not
   meant to survive process restart.

### Architecture Diagram
```
CheckpointTrigger (BP actor, overlap event)
        │  CaptureCheckpoint(Player)
        ▼
UMoonCheckpointSubsystem (GameInstanceSubsystem, memory-only)
        │  FMoonCheckpointSnapshot { Transform, AttributeValues, StatusTags }
        ▼
AMoonCharacterBase::OnDeath()
        │  RestoreCheckpoint(Player)
        ▼
Player respawned in-place, no level reload, no disk I/O
```

### Key Interfaces
- `UMoonCheckpointSubsystem::CaptureCheckpoint(APawn* Player)` — void, called by CheckpointTrigger / Boss Phase transition
- `UMoonCheckpointSubsystem::RestoreCheckpoint(APawn* Player)` — void, called by death handling in `AMoonCharacterBase`
- `UMoonCheckpointSubsystem::HasActiveCheckpoint()` — bool, guards restore calls before any checkpoint has fired

## Alternatives Considered

### Alternative A: SaveGame object reused as memory-only container
- **Description**: Use a `USaveGame` subclass but never call `SaveGameToSlot`/`LoadGameFromSlot` — just hold the object in memory, structured so a later full-SaveGame ADR could add disk persistence with minimal rework.
- **Pros**: Forward-compatible shape if full SaveGame work starts later.
- **Cons**: `USaveGame` carries serialization overhead/API surface (`UPROPERTY(SaveGame)` tagging, version tracking) irrelevant to a pure in-memory snapshot — adds conceptual weight for zero current benefit.
- **Rejection Reason**: YAGNI — this ADR is scoped to runtime-only; a future Persistence GDD should design the SaveGame shape once actual save requirements (what to save, when, cloud sync) are known, not guess now.

### Alternative B: Level-owned checkpoint component + GameMode-driven respawn
- **Description**: Each level keeps its own checkpoint state on the `GameMode`/`GameState`, respawn logic lives per-level.
- **Pros**: No cross-level lifetime concerns.
- **Cons**: `GameMode`/`GameState` are destroyed and recreated on level travel — breaks the Arena Morphing requirement (checkpoint must survive boss-arena transitions within one encounter). Would require duplicating checkpoint logic per level/GameMode.
- **Rejection Reason**: Fails the "survive level transitions" requirement outright.

### Alternative C: PlayerState-owned checkpoint data
- **Description**: Store snapshot fields directly on `AMoonPlayerState`.
- **Pros**: Naturally tied to the player, simple access.
- **Cons**: ADR-0001 already decided the player character owns its `AbilitySystemComponent` locally (not via PlayerState) specifically because "the player character does not persist across matches/levels in a way that requires PlayerState ownership." Routing checkpoint data through PlayerState would contradict that established stance without superseding it.
- **Rejection Reason**: Conflicts with ADR-0001's PlayerState-ownership decision; would require re-litigating ADR-0001 for no clear benefit.

## Consequences

### Positive
- Instant respawn — no loading screen, preserves combat tension curve
- Clean separation from future disk-SaveGame work — no premature persistence-format decisions
- Survives Arena Morphing's level transitions for free via `GameInstanceSubsystem` lifetime

### Negative
- No checkpoint history — only one active checkpoint; cannot roll back further than the most recent trigger (acceptable per requirements — no undo requested)
- If a future Persistence GDD adds disk-save, this snapshot struct will need to be re-expressed as a `USaveGame` — some rework expected, accepted as reasonable cost of not over-building now

### Risks
- **Risk**: Seamless level travel edge cases could reset `GameInstanceSubsystem` state unexpectedly on some travel types.
  **Mitigation**: Verification Required item above — test specifically against Arena Morphing's boss-arena transition method before relying on this for boss checkpoints.
- **Risk**: Restoring `UMoonAttributeSet` values via direct snapshot could bypass GAS's effect-based attribute mutation conventions.
  **Mitigation**: Decision explicitly requires restore via `GameplayEffect` application, not direct attribute writes — enforced in Control Manifest when this ADR is implemented.
- **Risk** (engine-specialist review, 2026-07-18): UE 5.8 deprecated the legacy GAS attribute-set initialization functions. If ADR-0001's `UMoonAttributeSet` init still uses the legacy pattern, this ADR's GameplayEffect-based restore may interact with it incorrectly.
  **Mitigation**: Verify `UMoonAttributeSet` init uses the updated 5.8 pattern before implementing this ADR's restore path — see Verification Required above.

## GDD Requirements Addressed

| GDD System | Requirement | How This ADR Addresses It |
|------------|-------------|--------------------------|
| health-damage-core.md | "사망 = 즉시 체크포인트 리스폰" (death = instant checkpoint respawn) | `UMoonCheckpointSubsystem` captures/restores state with no disk I/O or reload, satisfying the "instant" requirement |
| player-movement.md | Player transform/position continuity | `FMoonCheckpointSnapshot.PlayerTransform` restore on death |

## Performance Implications
- **CPU**: Negligible — snapshot capture/restore is a struct copy + GameplayEffect application, not a per-frame cost
- **Memory**: One `FMoonCheckpointSnapshot` instance resident for subsystem lifetime — trivial footprint (Transform + few floats + small tag array)
- **Load Time**: None — explicitly no disk I/O
- **Network**: N/A (single-player scope per current GDDs; if co-op/multiplayer is added later, this ADR would need revisiting for replication)

## Migration Plan
New system, no existing code to migrate. Implementation replaces whatever placeholder "restart level on death" behavior currently exists (if any) in the Health/Damage Core prototype.

## Validation Criteria
- Death during normal combat restores player to last checkpoint transform + attribute values within one frame (no loading screen observed)
- Checkpoint captured before entering a Boss Phase transition restores correctly after death mid-boss-fight, including across the Arena Morphing level-transition case
- No `SaveGameToSlot`/`LoadGameFromSlot` calls present anywhere in this system (verifies scope boundary held)

## Related Decisions
- ADR-0001 (Player Movement and GAS Core Foundation) — dependency
- `design/gdd/health-damage-core.md` — source requirement
- `design/gdd/systems-index.md` — scope boundary (Persistence category excluded from current index)
