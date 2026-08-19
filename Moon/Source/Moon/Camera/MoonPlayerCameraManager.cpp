#include "MoonPlayerCameraManager.h"
#include "MoonCameraSettings.h"
#include "UObject/ConstructorHelpers.h"

AMoonPlayerCameraManager::AMoonPlayerCameraManager()
{
	// CDO/no-asset-assigned fallback (AC-3 edge case) — GDD Rule 2 / Formula 4 default range.
	// Overwritten by ApplyPitchClamp() in InitializeFor() once CameraSettings is non-null.
	ViewPitchMin = -60.0f;
	ViewPitchMax = 30.0f;

	// This native class has no Blueprint CDO on which designers can assign the shared camera
	// settings asset. Keep the tuning values in the DataAsset and use a hard reference only as the
	// asset locator so the same source of truth is loaded and cooked for every player camera manager.
	static ConstructorHelpers::FObjectFinder<UMoonCameraSettings> CameraSettingsFinder(
		TEXT("/Game/Moon/Camera/DA_MoonCameraSettings.DA_MoonCameraSettings"));
	if (CameraSettingsFinder.Succeeded())
	{
		CameraSettings = CameraSettingsFinder.Object;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[MoonCamera] DA_MoonCameraSettings could not be loaded; using pitch fallback values."));
	}
}

void AMoonPlayerCameraManager::InitializeFor(APlayerController* PC)
{
	Super::InitializeFor(PC);

	ApplyPitchClamp();
}

void AMoonPlayerCameraManager::ApplyPitchClamp()
{
	// Null-asset guard (AC-3 edge case) — must not crash with no asset assigned, falls back to the
	// constructor's safe literal default set above.
	if (!CameraSettings)
	{
		return;
	}

	// Direct assignment per GDD Formula 4: theta_final = Clamp(theta_pitch, CameraPitchMin,
	// CameraPitchMax). ViewPitchMin/ViewPitchMax are the exact engine-side bounds APlayerController's
	// per-tick rotation update (LimitViewPitch) clamps against, including a single large-delta frame
	// — the engine clamps unconditionally every tick, so no extra overshoot guard is needed here.
	ViewPitchMin = CameraSettings->CameraPitchMin;
	ViewPitchMax = CameraSettings->CameraPitchMax;
}
