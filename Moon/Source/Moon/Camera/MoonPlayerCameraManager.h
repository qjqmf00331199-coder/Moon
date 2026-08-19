#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "MoonPlayerCameraManager.generated.h"

class UMoonCameraSettings;

// AMoonPlayerCameraManager (Story 002 / TR-cam-002 / ADR-0005 Decision 3, Alternative 3): owns the
// vertical (Pitch) view rotation clamp. GDD Formula 4 (design/gdd/camera-system-base.md:145-151) is
// a direct FMath::Clamp(Pitch, CameraPitchMin, CameraPitchMax) — this class sources those two bounds
// from UMoonCameraSettings and applies them to the engine's own ViewPitchMin/ViewPitchMax, which
// APlayerController's per-frame rotation update clamps against every tick via LimitViewPitch(). No
// custom Tick clamp is implemented here; the engine does that work once these two properties are set.
//
// ADR-0005 Alternative 3 explicitly rejects owning this clamp in CharacterMovementComponent or the
// SpringArm — do not add pitch logic to either of those from this story or any future one.
//
// This class is also where Story 009's camera shake dispatch will live (ADR-0005 Decision 6) — kept
// open for that addition, not sealed as pitch-only.
UCLASS()
class MOON_API AMoonPlayerCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()

public:
	AMoonPlayerCameraManager();

	// Camera tuning source (ADR-0005 Key Interfaces: "it reads UMoonCameraSettings"). Assign the
	// same DA_MoonCameraSettings asset here as on AMoonCharacterBase — Story 001 and this story each
	// read the asset independently, there is no shared-reference plumbing between them.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UMoonCameraSettings> CameraSettings;

protected:
	// Called once by the owning APlayerController when this manager is spawned (before the first
	// UpdateCamera), well before any possession — the correct one-time hook for a static config
	// value, not a per-Tick concern (Story 002 instruction: "not a Tick hack").
	virtual void InitializeFor(APlayerController* PC) override;

protected:
	// AC-3 edge case: safe fallback when no CameraSettings asset is assigned, so the clamp is never
	// left at the engine's own uninitialized/unclamped default. Matches the GDD Rule 2 / Formula 4
	// default range and ADR-0005 Decision 2's CDO-fallback convention already used on
	// AMoonCharacterBase's CameraSettings-literal pattern.
	//
	// Protected, not private (pre-existing build-blocking bug found and fixed during Story 005's
	// /dev-story pass, unrelated to Story 005 itself): a derived class's `using Base::Member`
	// test accessor cannot re-expose a truly private base member — MoonPlayerCameraManagerTests.cpp
	// already relied on that pattern and failed to compile until this was protected.
	void ApplyPitchClamp();
};
