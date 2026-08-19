#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "../Camera/MoonPlayerCameraManager.h"
#include "../Camera/MoonCameraSettings.h"

// Story 002 AC-2/AC-3 (TR-cam-002, GDD Formula 4, QA-TEST-04): AMoonPlayerCameraManager's pitch
// clamp must (a) be sourced from UMoonCameraSettings rather than hardcoded, with a safe fallback
// when no asset is assigned, and (b) actually hold the [ViewPitchMin, ViewPitchMax] boundary even
// under a single extreme-delta input (fast mouse flick), not just "the two properties got set".
//
// `ApplyPitchClamp()` is private by design (Story 002's own encapsulation, mirrors
// AMoonCharacterBase's protected ApplyCameraSettings() from Story 001) — this test-only accessor
// exposes it via `using` rather than loosening the real class's access level, matching the
// `AMoonCharacterBase_TestAccessor` convention in MoonCameraApplySettingsRuntimeTests.cpp.
class AMoonPlayerCameraManager_TestAccessor : public AMoonPlayerCameraManager
{
public:
	using AMoonPlayerCameraManager::ApplyPitchClamp;
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMoonPlayerCameraManagerPitchClampTest,
	"Moon.Camera.PlayerCameraManager.PitchClampHoldsBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMoonPlayerCameraManagerPitchClampTest::RunTest(const FString& Parameters)
{
	// AC-3: values sourced from the data asset, not hardcoded — use non-default bounds
	// (-45.0/15.0) so this can't pass off the GDD-default constructor literals (-60.0/30.0),
	// same discipline as Story 001's MoonCameraAppliesAtRuntimeTest (300.0 vs 450.0).
	{
		AMoonPlayerCameraManager_TestAccessor* Manager = NewObject<AMoonPlayerCameraManager_TestAccessor>(GetTransientPackage());
		TestNotNull(TEXT("Manager can be constructed via NewObject"), Manager);
		if (!Manager)
		{
			return false;
		}

		UMoonCameraSettings* Settings = NewObject<UMoonCameraSettings>(GetTransientPackage());
		TestNotNull(TEXT("UMoonCameraSettings can be constructed"), Settings);
		if (!Settings)
		{
			return false;
		}
		Settings->CameraPitchMin = -45.0f;
		Settings->CameraPitchMax = 15.0f;
		Manager->CameraSettings = Settings;

		Manager->ApplyPitchClamp();

		TestEqual(TEXT("ViewPitchMin sourced from CameraSettings, not the -60.0 GDD default"), Manager->ViewPitchMin, -45.0f);
		TestEqual(TEXT("ViewPitchMax sourced from CameraSettings, not the 30.0 GDD default"), Manager->ViewPitchMax, 15.0f);

		// AC-2 (QA-TEST-04): the clamp must hold even for a single very-large-delta frame (fast
		// mouse flick), in both directions, with zero overshoot. This calls the actual engine
		// LimitViewPitch() path (APlayerCameraManager, verified against the installed UE5.8
		// header) rather than re-deriving FMath::Clamp in the test, so it exercises the real
		// runtime clamp mechanism, not just the two source properties.
		FRotator FlickUp(-179.0f, 0.0f, 0.0f);
		Manager->LimitViewPitch(FlickUp, Manager->ViewPitchMin, Manager->ViewPitchMax);
		const float NormalizedUpPitch = FRotator::NormalizeAxis(FlickUp.Pitch);
		TestTrue(TEXT("Extreme upward flick never exceeds ViewPitchMin (-45.0)"), NormalizedUpPitch >= -45.0f - KINDA_SMALL_NUMBER);
		TestTrue(TEXT("Extreme upward flick stays within ViewPitchMax"), NormalizedUpPitch <= 15.0f + KINDA_SMALL_NUMBER);

		FRotator FlickDown(179.0f, 0.0f, 0.0f);
		Manager->LimitViewPitch(FlickDown, Manager->ViewPitchMin, Manager->ViewPitchMax);
		const float NormalizedDownPitch = FRotator::NormalizeAxis(FlickDown.Pitch);
		TestTrue(TEXT("Extreme downward flick never exceeds ViewPitchMax (15.0)"), NormalizedDownPitch <= 15.0f + KINDA_SMALL_NUMBER);
		TestTrue(TEXT("Extreme downward flick stays within ViewPitchMin"), NormalizedDownPitch >= -45.0f - KINDA_SMALL_NUMBER);
	}

	// AC-3 edge case: no asset assigned — must fall back to the safe GDD-default clamp
	// (-60.0/30.0) without crashing, and that fallback must still hold under the same
	// large-delta stress as above.
	{
		AMoonPlayerCameraManager_TestAccessor* Manager = NewObject<AMoonPlayerCameraManager_TestAccessor>(GetTransientPackage());
		TestNotNull(TEXT("Manager can be constructed via NewObject"), Manager);
		if (!Manager)
		{
			return false;
		}

		// Production now assigns the shared DataAsset on the native CDO. Clear it explicitly to
		// exercise the defensive no-asset path rather than depending on an unwired production state.
		Manager->CameraSettings = nullptr;
		TestNull(TEXT("CameraSettings can be cleared for fallback validation"), Manager->CameraSettings.Get());

		Manager->ApplyPitchClamp();

		TestEqual(TEXT("ViewPitchMin falls back to the GDD default when no asset is assigned"), Manager->ViewPitchMin, -60.0f);
		TestEqual(TEXT("ViewPitchMax falls back to the GDD default when no asset is assigned"), Manager->ViewPitchMax, 30.0f);

		FRotator FlickUp(-179.0f, 0.0f, 0.0f);
		Manager->LimitViewPitch(FlickUp, Manager->ViewPitchMin, Manager->ViewPitchMax);
		const float NormalizedFallbackPitch = FRotator::NormalizeAxis(FlickUp.Pitch);
		TestTrue(TEXT("Fallback clamp holds on extreme upward flick"), NormalizedFallbackPitch >= -60.0f - KINDA_SMALL_NUMBER && NormalizedFallbackPitch <= 30.0f + KINDA_SMALL_NUMBER);
	}

	// An assigned but invalid shared asset must be rejected just like AMoonCharacterBase rejects it.
	// This also proves the fallback overwrites stale values rather than merely returning early.
	{
		AMoonPlayerCameraManager_TestAccessor* Manager = NewObject<AMoonPlayerCameraManager_TestAccessor>(GetTransientPackage());
		TestNotNull(TEXT("Manager can be constructed for invalid-asset validation"), Manager);
		if (!Manager)
		{
			return false;
		}

		UMoonCameraSettings* InvalidSettings = NewObject<UMoonCameraSettings>(GetTransientPackage());
		TestNotNull(TEXT("Invalid test settings can be constructed"), InvalidSettings);
		if (!InvalidSettings)
		{
			return false;
		}

		InvalidSettings->CameraPitchMin = -90.0f;
		InvalidSettings->CameraPitchMax = 60.0f;
		Manager->ViewPitchMin = -10.0f;
		Manager->ViewPitchMax = 10.0f;
		Manager->CameraSettings = InvalidSettings;
		AddExpectedError(TEXT("rejected CameraSettings asset"), EAutomationExpectedErrorFlags::Contains, 1);
		Manager->ApplyPitchClamp();

		TestEqual(TEXT("Invalid CameraPitchMin falls back to the safe class default"), Manager->ViewPitchMin, -60.0f);
		TestEqual(TEXT("Invalid CameraPitchMax falls back to the safe class default"), Manager->ViewPitchMax, 30.0f);
	}

	return true;
}

#endif
