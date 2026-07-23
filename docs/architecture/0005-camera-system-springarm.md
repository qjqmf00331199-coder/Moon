# ADR-0005: Camera System (base) — SpringArm + Data-Driven Config

## Status
Proposed

## Date
2026-07-23

## Engine Compatibility

| Field | Value |
|-------|-------|
| **Engine** | Unreal Engine 5.8 |
| **Domain** | Camera / Input |
| **Knowledge Risk** | LOW — `USpringArmComponent`/`UCameraComponent`/`APlayerCameraManager` are stable pre-cutoff GameFramework APIs, not flagged in `deprecated-apis.md` or `breaking-changes.md` |
| **References Consulted** | `docs/engine-reference/unreal/VERSION.md`, `docs/engine-reference/unreal/breaking-changes.md`, `docs/engine-reference/unreal/deprecated-apis.md`, `docs/engine-reference/unreal/plugins/gameplay-camera-system.md`, `docs/engine-reference/unreal/PLUGINS.md` |
| **Post-Cutoff APIs Used** | None. The project's own engine reference flags a newer post-cutoff alternative (`GameplayCameras` plugin, UE5.5+, node-based Camera Rig system) — deliberately not adopted here, see Alternatives. |
| **Verification Required** | `bUseCameraLagSubstepping`/`CameraLagMaxTimeStep` (already in use in `MoonCharacterBase.cpp`) are not documented in this project's engine-reference library — low risk (long-stable SpringArm properties) but not formally verified against 5.8 headers. |

## ADR Dependencies

| Field | Value |
|-------|-------|
| **Depends On** | None (ADR-0001, Accepted, only decouples facing from camera yaw at the character-rotation level — does not touch SpringArm/Camera internals) |
| **Enables** | Future Core Extraction Execution GDD (execution cutscene blend interface, Rule 8) and Overdrive Visual State GDD (FOV state as one input) |
| **Blocks** | None — closes an architecture-review gap (TR-cam-001..009, 9/9 previously uncovered), does not block any epic from starting |
| **Ordering Note** | Should land before `/create-epics` per the 2026-07-18 architecture-review's Core-layer gap closure recommendation |

## Context

### Problem Statement
`design/gdd/camera-system-base.md` has been Approved since 2026-07-16 with 9 Core Rules, 5 Formulas, and an explicit Tuning Knobs mandate ("하드코딩되지 않고 외부 데이터 에셋... 통해 튜닝 가능해야 한다"), but has **zero ADR coverage** — `docs/architecture/architecture-review-2026-07-18.md` flagged all 9 technical requirements (TR-cam-001..009) as gaps.

The current implementation (`Moon/Source/Moon/Character/MoonCharacterBase.cpp` constructor) only covers a bare subset, hardcoded directly in C++:
- `TargetArmLength`, `SocketOffset`, `TargetOffset`, `bEnableCameraLag`, `CameraLagSpeed`, `CameraLagMaxDistance`, substepping, initial boom rotation, `bUsePawnControlRotation`.

Not implemented anywhere: pitch clamp ownership (Rule 2/Formula 4), collision + Destructible-ignore config (Rule 5), Overdrive FOV interpolation (Rule 7/Formula 2), execution cutscene blend (Rule 8/Formula 5), camera shake budget/dedup (Rule 9/Edge Case 6), and all 6 documented edge cases (corner dithering, `ResetCameraLag`, debris jitter, off-screen launch, execution look-input lock, shake amplitude cap). None of this is designer-tunable — it's all C++ constructor literals, contradicting the GDD's explicit data-asset mandate.

**Contract deviation found during this ADR's authoring** (documented here, not fixed here): `MoonCharacterBase.cpp`'s character rotation flags (`bUseControllerRotationYaw = false`, `bOrientRotationToMovement = true`) contradict `camera-system-base.md` Rule 6 and `player-movement.md` Core Rule 2, which both mandate the opposite (instant snap-to-camera-yaw facing, no orient-to-movement) so strafe-aiming stays screen-locked. `git blame` confirms these two lines are unmodified since the initial scaffolding commit (`9724f44`) — stock UE5 third-person-template defaults, never touched by the later camera/dash tuning pass (`2e6ba9f`) or any other commit. This ADR ratifies the **GDD values** as the binding architectural contract; the C++ fix is tracked as a separate implementation task, not part of this decision.

### Constraints
- Must remain designer-tunable without a C++ recompile (GDD Tuning Knobs mandate).
- Must preserve the already-shipped, hand-tuned feel (`TargetArmLength=450`, `SocketOffset=(0,45,20)`, `CameraLagSpeed=18.0`, `CameraLagMaxDistance=60.0`) as the data asset's default values — this tuning was iterated on across real commits and should not be silently reset.
- Must not require rewriting the GDD's Formulas (2/3/5), which are written in SpringArm's own exponential-decay lag math and `FInterpTo`/`VInterpTo` — not compatible with a node-based rig system's blend model.

### Requirements
- Data-driven Tuning Knobs (Rule/Tuning Knobs table, 12 parameters).
- Pitch clamp owned by a `PlayerCameraManager` subclass (Rule 2), not `CharacterMovementComponent`.
- Overdrive FOV interpolation hook (Rule 7) triggerable by Luna Overdrive's `OnOverdriveStarted`/`OnOverdriveEnded`.
- Execution cutscene blend hook (Rule 8) — forward interface only, Core Extraction Execution GDD not yet designed.
- Collision config that ignores future Destructible Geometry debris (Rule 5, Edge Case 3) — forward interface, that GDD not yet designed either.

## Decision

1. **Keep `USpringArmComponent` + `UCameraComponent`** — do not migrate to the UE5.5+ `GameplayCameras` plugin (Camera Rig system). See Alternative 1 for rejection reasoning.
2. **Introduce `UMoonCameraSettings : UDataAsset`** holding every Tuning Knobs row (`TargetArmLength`, `CameraSocketOffset`, `CameraPitchMin/Max`, `CameraLagSpeed`, `CameraRotationLagSpeed`, `CameraLagMaxDistance`, `BaseFOV`, `OverdriveFOV`, `ExecutionArmLength`, `CameraProbeSize`). `AMoonCharacterBase` loads a reference to this asset and applies its values in `BeginPlay` (constructor keeps safe literal fallbacks purely for the CDO preview in-editor, never used at runtime once the asset loads).
3. **New `AMoonPlayerCameraManager : APlayerCameraManager` subclass** owns the pitch clamp (Rule 2/Formula 4) via a `ViewPitchMin`/`ViewPitchMax` override sourced from `UMoonCameraSettings` — matches the GDD's explicit assignment of this ownership to a PlayerCameraManager subclass rather than CMC or the SpringArm itself.
4. **FOV interpolation (Rule 7) and execution cutscene blend (Rule 8)** both live as `Tick`-driven `FInterpTo`/`VInterpTo` state on `AMoonCharacterBase` — consistent with the existing one-shot-anim/jump-state-machine Tick pattern already established in this file, rather than introducing a second, unrelated animation/blend framework. Character exposes `SetOverdriveFOVActive(bool)` (wired to Luna Overdrive's start/end events) and forward-declares `BeginExecutionCameraBlend()`/`EndExecutionCameraBlend()` for Core Extraction Execution to call once that GDD exists.
5. **Collision**: `bDoCollisionTest=true`, `ProbeSize` from `UMoonCameraSettings` (default 12.0), query channel `ECC_Camera`. Destructible Geometry (not yet designed) is pre-obligated by this ADR to classify its debris under a channel the `CameraBoom`'s collision response ignores — this ADR does not invent that channel, it only registers the constraint for whoever designs Destructible Geometry next.
6. **Camera shake** (Rule 9, Edge Case 6): `UCameraShakeBase` subclasses per trigger (damage/Supernova/Just-Dodge), owned by `AMoonPlayerCameraManager`, with a hard cap on concurrent shakes and same-class-restart-not-stack policy to prevent unbounded amplitude compounding.
7. **Rotation contract** (Rule 6 / `player-movement.md` Core Rule 2): this ADR ratifies `bUseControllerRotationYaw=true`, `bOrientRotationToMovement=false`, `bUseControllerRotationPitch=false`, `bUseControllerRotationRoll=false` as the canonical architectural contract. The current C++ deviation is a tracked bug (see Risks), not a decision this ADR is changing.

### Architecture Diagram
```
AMoonCharacterBase
 ├─ CameraBoom (USpringArmComponent)          ← config from UMoonCameraSettings@BeginPlay
 │   └─ FollowCamera (UCameraComponent)
 ├─ Tick: FOV FInterpTo (Rule 7) ←── SetOverdriveFOVActive(bool) ←── Luna Overdrive events
 ├─ Tick: Execution blend FInterpTo/VInterpTo (Rule 8) ←── BeginExecutionCameraBlend() [future: Core Extraction Execution]
 └─ (Player)Controller
     └─ AMoonPlayerCameraManager (APlayerCameraManager)
         ├─ Pitch clamp (Rule 2/Formula 4)
         └─ Camera shake dispatch + amplitude cap (Rule 9/Edge Case 6)

UMoonCameraSettings (UDataAsset) — single source of truth for all 12 Tuning Knobs values
```

### Key Interfaces
- `UMoonCameraSettings` (DataAsset): read-only property bag, one instance referenced by `AMoonCharacterBase`.
- `AMoonCharacterBase::SetOverdriveFOVActive(bool bActive)` — Luna Overdrive's `OnOverdriveStarted`/`OnOverdriveEnded` call this.
- `AMoonCharacterBase::BeginExecutionCameraBlend()` / `EndExecutionCameraBlend()` — forward interface, unimplemented caller until Core Extraction Execution GDD lands.
- `AMoonPlayerCameraManager` — no other system calls into it directly; it reads `UMoonCameraSettings` and reacts to shake-trigger delegates (damage, Supernova, Just-Dodge) it subscribes to from Health/Damage Core and Dash/Evasion.
- Respawn/teleport hook (Edge Case 2): whichever system handles checkpoint restore (ADR-0002 `UMoonCheckpointSubsystem`) must call `CameraBoom->ResetCameraLag()` — cross-referenced obligation on that existing ADR, not a new one.

## Alternatives Considered

### Alternative 1: Migrate to the UE5.5+ Gameplay Camera System (`GameplayCameras` plugin)
- **Description**: Replace SpringArm+Camera with `UGameplayCameraComponent` + node-based Camera Rig assets, per this project's own engine-reference recommendation for "dynamic camera behavior."
- **Pros**: Engine-recommended modern path; built-in rig blending could simplify future camera-mode additions (e.g. boss-phase cameras).
- **Cons**: GDD Formulas 2/3/5 are written in SpringArm's own math (`FInterpTo`, exponential-decay lag, `VInterpTo` for the execution blend) — would require rewriting the Approved GDD's Formulas section, not just the implementation. Zero existing project code uses this plugin. Higher post-cutoff engine risk (this is itself a newer, less battle-tested-in-this-project system) for a benefit (rig blending) the MVP scope doesn't need — the only "mode switch" in scope is the execution cutscene blend, which the GDD already specifies as a simple two-parameter lerp, not a rig graph.
- **Rejection Reason**: No requirement in the Approved GDD needs rig-graph blending; the cost (GDD rewrite + net-new unproven plugin usage) outweighs the benefit for MVP scope.

### Alternative 2: Keep everything hardcoded in the C++ constructor (status quo)
- **Description**: Leave all 12 tuning values as C++ literals, as currently shipped.
- **Pros**: Zero new code.
- **Cons**: Directly contradicts the GDD's explicit, non-negotiable Tuning Knobs mandate ("하드코딩되지 않고 외부 데이터 에셋... 통해 튜닝 가능해야 한다"); blocks any designer iteration on feel without a recompile+relink cycle.
- **Rejection Reason**: Violates an explicit Approved-GDD requirement, not just a nice-to-have.

### Alternative 3: Own the pitch clamp in `CharacterMovementComponent` instead of a `PlayerCameraManager` subclass
- **Description**: Clamp view pitch inside CMC's rotation-update path.
- **Pros**: One fewer new class.
- **Cons**: GDD Rule 2 explicitly assigns this ownership to "`APlayerCameraManager` 서브클래스" — CMC doesn't own camera/view-target rotation cleanly, and mixing movement-rotation and view-rotation concerns risks the same kind of silent contract drift this ADR just found (Rule 6/rotation-flags).
- **Rejection Reason**: Contradicts explicit GDD ownership assignment; introduces exactly the kind of cross-system rotation-ownership ambiguity that caused the Rule 6 deviation.

## Consequences

### Positive
- Designers can retune all 12 camera parameters via a DataAsset without recompiling — satisfies the GDD's explicit mandate.
- GDD Formulas map 1:1 onto SpringArm's native lag/interpolation model — no translation risk.
- Reuses the already-tuned, already-shipped feel as the DataAsset's defaults — no regression to the proven values from commits `2e6ba9f`/`7f7e2e3`.
- Low engine risk — no post-cutoff API dependency.

### Negative
- Adds a new DataAsset class + a new `PlayerCameraManager` subclass (moderate new code surface vs. the current bare-minimum constructor).
- Execution cutscene blend (`BeginExecutionCameraBlend`/`EndExecutionCameraBlend`) has no caller until Core Extraction Execution is designed — this interface is speculative and may need revision once that GDD lands.

### Risks
- **The rotation-flag deviation (Rule 6) means the currently shipped build's actual strafe-aim feel does NOT match this ADR's ratified contract until the separately-tracked C++ fix lands.** Any story or playtest run before that fix must not assume aim-while-strafe is currently screen-locked — it isn't. Mitigation: the fix task is already flagged (spawned as a background task); `/create-stories` for this ADR's epic should include a PIE strafe-aim acceptance check, not rely on code review alone.
- `bUseCameraLagSubstepping`/`CameraLagMaxTimeStep` semantics are unverified against real UE5.8 headers (not in this project's engine-reference library). Mitigation: verify at implementation time; low risk given these are long-stable SpringArm properties.
- Destructible Geometry's collision channel doesn't exist yet — this ADR's collision-ignore obligation on that future system is a forward constraint that could be missed if that GDD's author doesn't cross-reference this ADR. Mitigation: this ADR's existence + registry entry (see below) is the cross-reference; `/architecture-decision` for Destructible Geometry must check the registry first (already the tool's Step 3a behavior).

## GDD Requirements Addressed

| GDD System | Requirement | How This ADR Addresses It |
|------------|-------------|---------------------------|
| camera-system-base.md | Rule 1 (component hierarchy) | Standard SpringArm/Camera attachment, no change needed |
| camera-system-base.md | Rule 2 (Enhanced Input rotation) + Formula 4 (pitch clamp) | `AMoonPlayerCameraManager` owns pitch clamp from `UMoonCameraSettings` |
| camera-system-base.md | Rule 3 (camera-relative movement) + Formula 1 | Already implemented (Enhanced Input + camera-relative vector) — unaffected by this ADR |
| camera-system-base.md | Rule 4 (SpringArm lag properties) + Formula 3 | `UMoonCameraSettings`-driven SpringArm config, values preserved from current shipped tuning |
| camera-system-base.md | Rule 5 (collision + debris exception) | `bDoCollisionTest`/`ProbeSize`/`ECC_Camera` config + forward obligation on Destructible Geometry's future channel |
| camera-system-base.md | Rule 6 (rotation snap facing) | Ratified as canonical; current C++ deviation tracked as a separate bug fix, not this ADR's scope |
| camera-system-base.md | Rule 7 (Overdrive FOV) + Formula 2 | Character-side `SetOverdriveFOVActive`/Tick `FInterpTo`, wired to Luna Overdrive events |
| camera-system-base.md | Rule 8 (execution cutscene) + Formula 5 | Character-side `BeginExecutionCameraBlend`/`EndExecutionCameraBlend` forward interface |
| camera-system-base.md | Rule 9 (camera shake) + Edge Case 6 | `AMoonPlayerCameraManager`-owned shake dispatch with amplitude cap + restart-not-stack |
| camera-system-base.md | Edge Cases 1-2, 4-5 | Dither fade (implementation detail on `AMoonPlayerCameraManager`/material), `ResetCameraLag()` obligation on checkpoint restore (ADR-0002), lag-max-distance hard-follow (already in `UMoonCameraSettings`), look-input lock during execution (character-side flag) |
| player-movement.md | Core Rule 2 (facing decouple) | Same Rule-6 ratification above — both GDDs share this contract |

## Performance Implications
- **CPU**: Negligible — SpringArm/CameraShake are cheap per-frame operations already used at scale in UE projects generally; one `UDataAsset` load at `BeginPlay` is a one-time cost.
- **Memory**: One small `UMoonCameraSettings` asset instance, negligible.
- **Load Time**: Negligible (single small DataAsset).
- **Network**: N/A — camera is client-side presentation only, no replication implications.

## Migration Plan
1. Create `UMoonCameraSettings` DataAsset class with all 12 fields, defaulted to the currently shipped values (no feel change).
2. Move `MoonCharacterBase.cpp`'s hardcoded SpringArm setup to read from the asset in `BeginPlay` (constructor keeps literal fallbacks for CDO preview only).
3. Implement `AMoonPlayerCameraManager`, wire pitch clamp, assign as the player's camera manager class.
4. Add collision config (`bDoCollisionTest`, `ProbeSize`, `ECC_Camera`) to the SpringArm setup.
5. Add FOV/Overdrive interpolation state + `SetOverdriveFOVActive` to `AMoonCharacterBase::Tick`, wire to Luna Overdrive's start/end events.
6. Add execution blend forward interface (no caller yet — stub only until Core Extraction Execution exists).
7. Add camera shake classes + dispatch on `AMoonPlayerCameraManager`.
8. Separately (tracked task, not this migration): fix the Rule 6 rotation-flag deviation in the constructor.

## Validation Criteria
- `camera-system-base.md` QA-TEST-01 through QA-TEST-10, executed in PIE.
- New explicit check tied to the Rule 6 fix: strafe in all 4 diagonal directions while continuously look-input rotating; character facing must snap to camera yaw every tick with zero lag, confirming aim stays screen-locked.
- `overdrive_duration`/`overdrive_recovery_duration` FOV transitions timed against Formula 2's 0.5s/0.8s targets (±0.05s/±0.08s per QA-TEST-08).

## Related Decisions
- ADR-0001 (Accepted) — character rotation decoupling at the GAS-foundation level; this ADR owns the camera-specific pitch/FOV/shake layer on top of it.
- ADR-0002 (checkpoint-persistence) — owns the respawn/teleport event this ADR's `ResetCameraLag()` obligation attaches to.
- `design/gdd/camera-system-base.md`, `design/gdd/player-movement.md` — GDDs this ADR implements.
