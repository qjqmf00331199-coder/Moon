#include "MoonCameraSettings.h"

bool UMoonCameraSettings::IsWithinSafeRanges(FString& OutFailureReason) const
{
	OutFailureReason.Reset();

	if (!FMath::IsWithinInclusive(TargetArmLength, 250.0f, 500.0f))
	{
		OutFailureReason = TEXT("TargetArmLength must be within [250, 500].");
	}
	else if (!FMath::IsNearlyZero(CameraSocketOffset.X)
		|| !FMath::IsWithinInclusive(CameraSocketOffset.Y, -50.0, 50.0)
		|| !FMath::IsWithinInclusive(CameraSocketOffset.Z, -30.0, 50.0))
	{
		OutFailureReason = TEXT("CameraSocketOffset requires X=0, Y within [-50, 50], and Z within [-30, 50].");
	}
	else if (!FMath::IsWithinInclusive(CameraPitchMin, -80.0f, -45.0f))
	{
		OutFailureReason = TEXT("CameraPitchMin must be within [-80, -45].");
	}
	else if (!FMath::IsWithinInclusive(CameraPitchMax, 15.0f, 45.0f))
	{
		OutFailureReason = TEXT("CameraPitchMax must be within [15, 45].");
	}
	else if (CameraPitchMin >= CameraPitchMax)
	{
		OutFailureReason = TEXT("CameraPitchMin must be less than CameraPitchMax.");
	}
	else if (!FMath::IsWithinInclusive(CameraLagSpeed, 5.0f, 20.0f))
	{
		OutFailureReason = TEXT("CameraLagSpeed must be within [5, 20].");
	}
	else if (!FMath::IsWithinInclusive(CameraRotationLagSpeed, 8.0f, 25.0f))
	{
		OutFailureReason = TEXT("CameraRotationLagSpeed must be within [8, 25].");
	}
	else if (!FMath::IsWithinInclusive(CameraLagMaxDistance, 40.0f, 120.0f))
	{
		OutFailureReason = TEXT("CameraLagMaxDistance must be within [40, 120].");
	}
	else if (!FMath::IsWithinInclusive(BaseFOV, 80.0f, 100.0f))
	{
		OutFailureReason = TEXT("BaseFOV must be within [80, 100].");
	}
	else if (!FMath::IsWithinInclusive(OverdriveFOV, 95.0f, 110.0f))
	{
		OutFailureReason = TEXT("OverdriveFOV must be within [95, 110].");
	}
	else if (!FMath::IsWithinInclusive(ExecutionArmLength, 100.0f, 250.0f))
	{
		OutFailureReason = TEXT("ExecutionArmLength must be within [100, 250].");
	}
	else if (!FMath::IsWithinInclusive(CameraProbeSize, 5.0f, 25.0f))
	{
		OutFailureReason = TEXT("CameraProbeSize must be within [5, 25].");
	}
	// CornerDitherThreshold, CameraNearClipPlane, and CornerDitherFadeSpeed have no GDD-approved
	// safe range (Story 001 scope covers only the 11 core tuning knobs above); not validated here.

	return OutFailureReason.IsEmpty();
}
