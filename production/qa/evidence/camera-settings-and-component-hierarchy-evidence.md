# Test Evidence: Camera Settings and Component Hierarchy (Story 001)

> **Epic**: Camera System Foundation Fixes
> **Story**: `production/epics/camera-system-foundation-fixes/story-001-camera-settings-and-component-hierarchy.md`
> **Requirements**: TR-cam-001, TR-cam-009 / ADR-0005 Decisions 1, 2, and 5
> **Engine**: Unreal Engine 5.8 (project pin; local header verification described below)

## Implementation evidence

- `UMoonCameraSettings : UDataAsset` owns exactly the 11 tuning fields enumerated by Story 001.
  Scalar properties carry the GDD `ClampMin`/`ClampMax` and UI metadata. The vector's asymmetric
  component contract (`X=0`, separate Y/Z ranges) is enforced in `IsWithinSafeRanges()` because a
  single Unreal `FVector` property metadata range cannot express different per-axis limits.
- `AMoonCharacterBase` still owns the existing `USpringArmComponent -> UCameraComponent` pair.
  The boom attaches directly to the Capsule root at relative `(0,0,60)`, while the follow camera
  attaches to `USpringArmComponent::SocketName`.
- `BeginPlay()` calls `ApplyCameraSettings()` exactly once. A valid asset supplies the arm length,
  socket offset, both stored lag speeds, maximum lag distance, collision probe size, and base FOV.
  Pitch limits, Overdrive FOV, and execution arm length are deliberately only stored here; their
  runtime consumers belong to later stories.
- Missing/stale object references and out-of-safe-range assets emit one initialization-time
  `Error` diagnostic and apply the `UMoonCameraSettings` class defaults. No warning runs in Tick,
  and the camera hierarchy remains usable without a null dereference.
- Constructor values remain editor/CDO preview and missing-asset safety fallbacks. Once a valid
  asset is assigned, `BeginPlay()` replaces every Story-001-owned runtime camera property from the
  asset, so constructor literals are not the valid runtime source of truth.
- The native character constructor assigns `/Game/Moon/Camera/DA_CameraSettings` as the production
  default reference. Blueprint subclasses may override or clear that reference without making the
  Blueprint binary the canonical owner of the production default.

## Approved default preservation

The settings class defaults and constructor preview fallbacks both preserve the approved shoulder
view and collision tuning:

| Property | Value |
|---|---:|
| `TargetArmLength` | `450.0` |
| `CameraSocketOffset` | `(0,45,20)` |
| `CameraLagSpeed` | `18.0` |
| `CameraLagMaxDistance` | `60.0` |
| `BaseFOV` | `90.0` |
| `CameraProbeSize` | `12.0` |

Rotation lag remains disabled as required by Camera GDD Rule 4; its designer value (`15.0`) is
stored/applied without enabling the behavior. Collision stays enabled on `ECC_Camera`, and the
boom inherits controller Pitch/Yaw but never Roll.

## UE 5.8 header verification

Verified directly against the installed header:
`C:/Program Files/Epic Games/UE_5.8/Engine/Source/Runtime/Engine/Classes/GameFramework/SpringArmComponent.h`.
It declares the exact public properties used by this story: `TargetArmLength`, `SocketOffset`,
`TargetOffset`, `ProbeSize`, `ProbeChannel`, `bDoCollisionTest`, `bUsePawnControlRotation`,
`bInheritPitch`, `bInheritYaw`, `bInheritRoll`, `bEnableCameraLag`,
`bEnableCameraRotationLag`, `bUseCameraLagSubstepping`, `CameraLagSpeed`,
`CameraRotationLagSpeed`, `CameraLagMaxTimeStep`, and `CameraLagMaxDistance`. This closes the
ADR's explicit uncertainty around `bUseCameraLagSubstepping` and `CameraLagMaxTimeStep` for the
pinned engine version. Neither property is listed in this project's UE 5.8 breaking-change or
deprecated-API notes.

## Automated evidence

Run:

```powershell
& tests/static/camera/camera_settings_contract_check.ps1
```

The script contains five deterministic test functions covering:

1. Capsule -> SpringArm -> Camera hierarchy and the `Z=60` torso pivot.
2. Exact 11-field schema, defaults, scalar metadata, and vector validation.
3. One-time `BeginPlay` DataAsset application and required property mappings.
4. Actionable missing/invalid-asset diagnostics plus safe-default fallback.
5. Absence of Story 002/004/006/007 behavior (pitch manager, lag hard-limit additions, dynamic
   FOV, and execution blends).

**Result (2026-08-19): PASS** —
`camera settings and component hierarchy contract checks passed (5 test functions)`.

Related same-file regression scripts also pass:

- `tests/static/movement_foundation_contract.ps1`
- `tests/static/movement_independence_check.ps1`
- `tests/static/movement_regression_checks.ps1`

The final UE 5.8 `MoonEditor Win64 Development` build also passes. After review moved the default
asset reference out of the partially loaded Blueprint and into the native constructor, the final
`-NoUBA` verification processed UHT, compiled all three changed C++ units, linked the module, and
completed all 6 actions with `Result: Succeeded` in 12.58 seconds.

## Production Data Asset / PIE status

`DA_CameraSettings` was created through an Unreal Editor asset write after the new reflected C++
class compiled. The native constructor establishes the production default reference without
rewriting the Blueprint binary. Remaining runtime checks are recorded below:

- [x] `/Game/Moon/Camera/DA_CameraSettings` exists and uses `MoonCameraSettings` as its class.
- [x] `AMoonCharacterBase` assigns that asset as the production default; `BP_MoonCharacter`
  inherits the reference without a binary override.
- [ ] PIE default view confirms 450 arm length, `(0,45,20)` socket offset, lag 18/max 60, FOV 90.
- [ ] PIE with the reference cleared emits one actionable diagnostic and retains a usable view.
- [ ] PIE or an automation fixture with an out-of-range asset emits one diagnostic and falls back.

Until those checks are completed in an editor session, the binary production-asset and runtime
PIE portion remains an explicit blocker to fully closing the story; the C++/static contract can
still be reviewed independently.

The unattended asset-write commandlet created the Data Asset successfully on 2026-08-19. Its
attempt to save a Blueprint-level override was discarded during review because this worktree does
not contain the ignored ParagonAurora dependencies: saving the partially loaded Blueprint would
have removed its existing mesh/animation references. The original Blueprint binary was restored
byte-for-byte from its verified Git LFS object, and the production reference moved to the native
constructor instead.

A subsequent unattended, save-free editor launch exited successfully. The log contains no failed
lookup for `DA_CameraSettings`, confirming that the native `FObjectFinder` path resolves the new
asset. It does still report this worktree's already-known missing ignored ParagonAurora packages;
because no Blueprint was saved during this verification, those references remain intact.
