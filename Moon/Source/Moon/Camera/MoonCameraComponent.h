#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraComponent.h"
#include "MoonCameraComponent.generated.h"

// Story 005 AC-4 (TR-cam-005, GDD Edge Case 1): stock UCameraComponent has no per-instance
// near-clip-plane property to set from outside — FMinimalViewInfo::PerspectiveNearClipPlane is the
// only per-view knob for this (verified against the installed UE5.8 headers,
// Engine/Classes/Camera/CameraTypes.h / CameraComponent.h; a value <= 0 there falls back to the
// engine's global GNearClippingPlane), and it is only reachable by overriding GetCameraView(). This
// subclass exists solely to inject that one override when corner-dithered — see
// AMoonCharacterBase::UpdateCameraCornerDither().
UCLASS(ClassGroup = Camera, meta = (BlueprintSpawnableComponent))
class MOON_API UMoonCameraComponent : public UCameraComponent
{
	GENERATED_BODY()

public:
	// <= 0 disables the override (engine falls back to GNearClippingPlane, normal behavior). Set to
	// a positive value to force that near-clip distance (GDD Edge Case 1: 10.0uu while
	// corner-dithered).
	UPROPERTY(BlueprintReadWrite, Category = "Camera")
	float NearClipPlaneOverride = -1.0f;

	virtual void GetCameraView(float DeltaTime, FMinimalViewInfo& DesiredView) override;
};
