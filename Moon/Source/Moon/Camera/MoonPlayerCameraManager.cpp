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
	// Use the same validation contract as AMoonCharacterBase. Otherwise one invalid shared asset
	// could be rejected by the character while this manager still applied its out-of-range pitch.
	const UMoonCameraSettings* EffectiveSettings = CameraSettings;
	FString FailureReason;
	if (!IsValid(EffectiveSettings))
	{
		EffectiveSettings = GetDefault<UMoonCameraSettings>();
	}
	else if (!EffectiveSettings->IsWithinSafeRanges(FailureReason))
	{
		UE_LOG(LogTemp, Error, TEXT("[MoonCamera] %s rejected CameraSettings asset '%s': %s Applying UMoonCameraSettings safe pitch defaults."),
			*GetNameSafe(this), *GetNameSafe(EffectiveSettings), *FailureReason);
		EffectiveSettings = GetDefault<UMoonCameraSettings>();
	}

	// Direct assignment per GDD Formula 4: theta_final = Clamp(theta_pitch, CameraPitchMin,
	// CameraPitchMax). ViewPitchMin/ViewPitchMax are the exact engine-side bounds APlayerController's
	// per-tick rotation update (LimitViewPitch) clamps against, including a single large-delta frame
	// — the engine clamps unconditionally every tick, so no extra overshoot guard is needed here.
	ViewPitchMin = EffectiveSettings->CameraPitchMin;
	ViewPitchMax = EffectiveSettings->CameraPitchMax;
}
