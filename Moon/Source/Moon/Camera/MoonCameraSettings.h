#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MoonCameraSettings.generated.h"

// UMoonCameraSettings (ADR-0005 Decision 2 / TR-cam-009): single source of truth for the camera
// system's Tuning Knobs (design/gdd/camera-system-base.md section 7). AMoonCharacterBase loads a
// reference to an instance of this asset and applies its values in BeginPlay; the constructor's
// literal SpringArm/Camera setup remains only as a CDO-preview fallback, never re-read at runtime
// once an asset is assigned (ADR-0005 Decision 2, Story 001 AC-6).
//
// Field count note (Story 001 implementation-time finding): the GDD's own Tuning Knobs table
// (section 7) and this ADR's Decision 2 enumerate exactly 11 parameters, not the "12" the ADR's
// Requirements section and this story's AC-3 claim. There is no undocumented 12th knob anywhere
// in the GDD prose (Formulas/Edge Cases values like TargetOffset, NearClipPlane=10, the 80uu
// dither threshold, or the execution SocketOffset=(0,40,20) all belong to other stories' scope,
// not this Tuning Knobs table). This class implements the 11 fields the table actually defines;
// flagged here rather than inventing a field to force a round number.
//
// Create a Data Asset whose class is MoonCameraSettings, then assign it to
// AMoonCharacterBase::CameraSettings. The character validates and applies the asset once in
// BeginPlay; invalid or missing assets fall back to this class's documented safe defaults.
// Call IsWithinSafeRanges() before applying an asset to catch designer-entered out-of-range
// values early (see method doc below).
//
// Thread safety: game-thread only. UDataAsset instances and camera components are Unreal
// objects and must not be read or mutated from worker threads without external synchronization.
UCLASS(BlueprintType)
class MOON_API UMoonCameraSettings : public UDataAsset
{
	GENERATED_BODY()

public:
	// SpringArm length, in unreal units. Shipped/GDD default: 450.0uu.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Framing", meta = (ClampMin = "250.0", ClampMax = "500.0", UIMin = "250.0", UIMax = "500.0", Units = "cm"))
	float TargetArmLength = 450.0f;

	// SpringArm socket offset (shoulder framing). Shipped/GDD default: (0, 45, 20). X is fixed at
	// zero by the camera contract.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Framing")
	FVector CameraSocketOffset = FVector(0.0f, 45.0f, 20.0f);

	// Pitch clamp minimum, degrees (max upward look angle). Applied by AMoonPlayerCameraManager
	// (Story 002) — this asset only stores the value.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Pitch Clamp", meta = (ClampMin = "-80.0", ClampMax = "-45.0", UIMin = "-80.0", UIMax = "-45.0", Units = "deg"))
	float CameraPitchMin = -60.0f;

	// Pitch clamp maximum, degrees (max downward look angle). Applied by AMoonPlayerCameraManager
	// (Story 002) — this asset only stores the value.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Pitch Clamp", meta = (ClampMin = "15.0", ClampMax = "45.0", UIMin = "15.0", UIMax = "45.0", Units = "deg"))
	float CameraPitchMax = 30.0f;

	// SpringArm positional lag speed. Shipped/GDD default: 18.0.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Lag", meta = (ClampMin = "5.0", ClampMax = "20.0", UIMin = "5.0", UIMax = "20.0"))
	float CameraLagSpeed = 18.0f;

	// SpringArm rotational lag speed. GDD default: 15.0. Rule 4 keeps rotation lag disabled for
	// aim responsiveness (bEnableCameraRotationLag stays false) — this value is stored for
	// designer tuning if that Rule is ever revisited, not applied to bEnableCameraRotationLag here.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Lag", meta = (ClampMin = "8.0", ClampMax = "25.0", UIMin = "8.0", UIMax = "25.0"))
	float CameraRotationLagSpeed = 15.0f;

	// Max distance the camera lag may fall behind the target before hard-following. GDD default:
	// 60.0uu. Story 004 owns the hard-follow clamp behavior; this story only defines/applies the
	// SpringArm property.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Lag", meta = (ClampMin = "40.0", ClampMax = "120.0", UIMin = "40.0", UIMax = "120.0", Units = "cm"))
	float CameraLagMaxDistance = 60.0f;

	// Base (non-Overdrive) field of view, degrees. GDD default: 90.0.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|FOV", meta = (ClampMin = "80.0", ClampMax = "100.0", UIMin = "80.0", UIMax = "100.0", Units = "deg"))
	float BaseFOV = 90.0f;

	// Luna Overdrive field of view target, degrees. GDD default: 100.0. Applied by Story 006's FOV
	// interpolation hook — this asset only stores the value.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|FOV", meta = (ClampMin = "95.0", ClampMax = "110.0", UIMin = "95.0", UIMax = "110.0", Units = "deg"))
	float OverdriveFOV = 100.0f;

	// Execution cutscene SpringArm length, in unreal units. GDD default: 150.0uu. Applied by
	// Story 007's execution camera blend — this asset only stores the value.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Execution", meta = (ClampMin = "100.0", ClampMax = "250.0", UIMin = "100.0", UIMax = "250.0", Units = "cm"))
	float ExecutionArmLength = 150.0f;

	// SpringArm collision probe radius, in unreal units. GDD default: 12.0uu.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Collision", meta = (ClampMin = "5.0", ClampMax = "25.0", UIMin = "5.0", UIMax = "25.0", Units = "cm"))
	float CameraProbeSize = 12.0f;

	// Story 005 / TR-cam-005 / GDD Edge Case 1: SpringArm's actual collided pivot-to-socket
	// distance (see AMoonCharacterBase::UpdateCameraCornerDither()) at or below which the mesh
	// dither fade + near-clip override engage. GDD default: 80.0uu.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Corner Dither")
	float CornerDitherThreshold = 80.0f;

	// Camera near-clip plane while corner-dithered (GDD Edge Case 1 default: 10.0uu). Only applied
	// while the dither is active; the camera uses the engine's global GNearClippingPlane otherwise.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Corner Dither")
	float CameraNearClipPlane = 10.0f;

	// FInterpTo speed (1/seconds) for the dither fade in/out. Not a GDD-specified value — GDD Edge
	// Case 1 only mandates that recovery fades back out rather than snapping/flickering; this knob
	// is what makes that continuous instead of a hard on/off toggle.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Corner Dither", meta = (ClampMin = "0.5"))
	float CornerDitherFadeSpeed = 8.0f;

	// Returns whether every value obeys the approved GDD safe range. Example:
	// `Settings->IsWithinSafeRanges(FailureReason)` before applying the asset. Performs no
	// allocation on the success path beyond retaining the caller-owned string; intended for
	// one-time initialization, not a hot path.
	bool IsWithinSafeRanges(FString& OutFailureReason) const;
};
