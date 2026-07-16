# Unreal Engine 5.8 — Breaking Changes

**Last verified:** 2026-07-16

This document tracks breaking API changes and behavioral differences between Unreal Engine 5.3
(likely in model training) and Unreal Engine 5.8 (current project version). Organized by risk level.
Original 5.3→5.7 content below is unchanged; see the **5.7 → 5.8 Delta** section at the end for
what's new since this project's prior pin.

## HIGH RISK — Will Break Existing Code

### Substrate Material System (Production-Ready in 5.7)
**Versions:** UE 5.5+ (experimental), 5.7 (production-ready)

Substrate replaces the legacy material system with a modular, physically accurate framework.

```cpp
// ❌ OLD: Legacy material nodes (still work but deprecated)
// Standard material graph with Base Color, Metallic, Roughness, etc.

// ✅ NEW: Substrate material layers
// Use Substrate nodes: Substrate Slab, Substrate Blend, etc.
// Modular material authoring with true physical accuracy
```

**Migration:** Enable Substrate in `Project Settings > Engine > Substrate` and rebuild materials using Substrate nodes.

---

### PCG (Procedural Content Generation) API Overhaul
**Versions:** UE 5.7 (production-ready)

PCG framework reached production-ready status with major API changes.

```cpp
// ❌ OLD: Experimental PCG API (pre-5.7)
// Old node types, unstable API

// ✅ NEW: Production PCG API (5.7+)
// Use FPCGContext, IPCGElement, new node types
// Stable API, production-ready workflow
```

**Migration:** Follow PCG migration guide in 5.7 docs. Expect significant refactoring for experimental PCG code.

---

### Megalights Rendering System
**Versions:** UE 5.5+

New lighting system supports millions of dynamic lights.

```cpp
// ❌ OLD: Limited dynamic lights (clustered forward shading)
// Max ~100-200 dynamic lights before performance degrades

// ✅ NEW: Megalights (5.5+)
// Millions of dynamic lights with minimal performance cost
// Enable: Project Settings > Engine > Rendering > Megalights
```

**Migration:** No code changes needed, but lighting behavior may differ. Test scenes after enabling.

---

## MEDIUM RISK — Behavioral Changes

### Enhanced Input System (Now Default)
**Versions:** UE 5.1+ (recommended), 5.7 (default)

Enhanced Input is now the default input system.

```cpp
// ❌ OLD: Legacy input bindings (deprecated)
InputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);

// ✅ NEW: Enhanced Input
SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) {
    UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
    EIC->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
}
```

**Migration:** Replace legacy input bindings with Enhanced Input actions.

---

### Nanite Default Enabled
**Versions:** UE 5.0+ (optional), 5.7 (encouraged)

Nanite virtualized geometry is now the recommended workflow for static meshes.

```cpp
// Enable Nanite on static mesh:
// Static Mesh Editor > Details > Nanite Settings > Enable Nanite Support
```

**Migration:** Convert high-poly meshes to Nanite. Test performance on target platforms.

---

## LOW RISK — Deprecations (Still Functional)

### Legacy Material System
**Status:** Deprecated but supported
**Replacement:** Substrate Material System

Legacy materials still work, but Substrate is recommended for new projects.

---

### Old World Partition (UE4 Style)
**Status:** Deprecated
**Replacement:** World Partition (UE5+)

Use UE5's World Partition system for large worlds.

---

## Platform-Specific Breaking Changes

### Windows
- **UE 5.7**: DirectX 12 is now default (was DX11 in older versions)
- Update shaders for DX12 compatibility

### macOS
- **UE 5.5+**: Metal 3 required (minimum macOS 13)

### Mobile
- **UE 5.7**: Minimum Android API level raised to 26 (Android 8.0)
- Minimum iOS deployment target raised to iOS 14

---

## Migration Checklist

When upgrading from UE 5.3 to UE 5.7:

- [ ] Review Substrate materials (convert if ready for new system)
- [ ] Audit PCG usage (update to production API if using experimental)
- [ ] Test Megalights performance (enable and benchmark)
- [ ] Migrate legacy input to Enhanced Input
- [ ] Convert high-poly meshes to Nanite
- [ ] Update shaders for DX12 (Windows) or Metal 3 (macOS)
- [ ] Verify minimum platform versions (Android 8.0, iOS 14)
- [ ] Test Lumen and Nanite performance on target hardware

---

**Sources:**
- https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-5-7-release-notes
- https://dev.epicgames.com/documentation/en-us/unreal-engine/upgrading-projects-to-newer-versions-of-unreal-engine

---

## 5.7 → 5.8 Delta (added 2026-07-16)

Epic's official 5.8 release notes report **no breaking changes for typical gameplay C++ projects**.
The items below are config/architecture changes worth knowing, plus one unverified community claim.

### MEDIUM RISK — Config/Architecture Changes

- **Zen Server remote networking**: `AllowRemoteNetworkService` project setting renamed to
  `RemoteNetworkService`, new enum values (`None`, `Unsecured`, `GeneratedStaticKey`). Update any
  project config referencing the old key.
- **Mass Framework split**: Core entity system moved into new `MassCore` module, separated from
  `MassGameplay`. Projects with heavy Mass usage may need updated module dependencies in `Build.cs`.
- **Input system consolidation**: Enhanced Input and Common Input/UI unified — removes duplicate
  data asset requirements. Review custom Input Mapping Context setup for now-redundant assets.
- **IK Retargeter operation stack restructured**: Blend to Source, Scale Goals, Offset Goals now
  split as separate ops. Existing retargeter assets should still load; custom op-stack tooling may
  need updates.
- **Windows audio backend**: Default switched XAudio2 → WASAPI (`AudioMixerWasapi`, auto-selected).
  No code changes required. Revert via `AudioMixerModuleName=AudioMixerXAudio2` in `WindowsEngine.ini`.

### LOW RISK — Now Production-Ready (was experimental)

- **Iris Replication**: production-ready for licensees in 5.8 (was experimental) — see
  `deprecated-apis.md` Networking section for the Iris vs. legacy Replication Graph note.
- **Megalights**: production-ready in 5.8 (was newer in 5.5-5.7).

### UNVERIFIED — Build Toolchain (community-reported, confirm locally)

Community migration notes (not confirmed against official Epic docs) claim 5.8 prefers
**Visual Studio 2026 / MSVC v145 toolset** over VS 2022 (v143), producing an error like:
`"Visual Studio compiler version 14.38.33145 is not a preferred version. Please use the latest
preferred version 14.50.35717"` if the older toolset is used.

If this appears: set `DefaultBuildSettings = V7` and `IncludeOrderVersion = Unreal5_8` in
`Target.cs`, update VC++ Redistributables via the engine's bundled installer, and clear `.vs/`,
`Binaries/`, `DerivedDataCache/`, `Intermediate/` before rebuilding.

**Source:** https://jakubpradeniak.com/posts/dev-notes/migrating-unreal-engine-5-8-cpp-project/
(community blog, not official — treat as a first troubleshooting step, not a guarantee)
