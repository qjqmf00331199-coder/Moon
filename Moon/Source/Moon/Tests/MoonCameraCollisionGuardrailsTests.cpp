#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "../Character/MoonCharacterBase.h"
#include "../Camera/MoonCameraSettings.h"
#include "GameFramework/SpringArmComponent.h"
#include "Engine/CollisionProfile.h"
#include "Editor.h"

// Story 005 (TR-cam-005, ADR-0005 Decision 5, GDD Rule 5 / Edge Case 3): collision-probe config +
// destructible-ignore contract, plus the AC-4 corner-dither target-alpha step (GDD Edge Case 1).
//
// CameraBoom/CameraSettings/ComputeCornerDitherTargetAlpha are protected/private on
// AMoonCharacterBase by design — this test-only accessor exposes them via `using`, matching the
// AMoonCharacterBase_TestAccessor convention already established in
// MoonCameraApplySettingsRuntimeTests.cpp (kept as a separate local type here rather than shared,
// since these are two independent translation units).
class AMoonCharacterBase_CollisionGuardrailsTestAccessor : public AMoonCharacterBase
{
public:
	using AMoonCharacterBase::CameraBoom;
	using AMoonCharacterBase::CameraSettings;
	using AMoonCharacterBase::ApplyCameraSettings;
	using AMoonCharacterBase::ComputeCornerDitherTargetAlpha;
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMoonCameraCollisionProbeConfigTest,
	"Moon.Camera.CollisionGuardrails.AC1_ProbeConfig",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMoonCameraCollisionProbeConfigTest::RunTest(const FString& Parameters)
{
	// AC-1 (QA-TEST AC-1): bDoCollisionTest=true, ProbeSize sourced from CameraSettings (not a
	// literal), ProbeChannel=ECC_Camera — read back after the runtime application path, not just checked in
	// the constructor.
	AMoonCharacterBase_CollisionGuardrailsTestAccessor* Character = NewObject<AMoonCharacterBase_CollisionGuardrailsTestAccessor>(GetTransientPackage());
	TestNotNull(TEXT("Character can be constructed via NewObject"), Character);
	if (!Character)
	{
		return false;
	}

	UMoonCameraSettings* Settings = NewObject<UMoonCameraSettings>(GetTransientPackage());
	TestNotNull(TEXT("UMoonCameraSettings can be constructed"), Settings);
	if (!Settings)
	{
		return false;
	}
	// Non-default probe size (not the 12.0 GDD default) so this can't pass off a constructor
	// literal that happens to match — same discipline as Story 001/002's asset-vs-literal tests.
	Settings->CameraProbeSize = 18.0f;
	Character->CameraSettings = Settings;
	Character->ApplyCameraSettings();

	TestNotNull(TEXT("CameraBoom exists after applying settings"), Character->CameraBoom.Get());
	if (Character->CameraBoom)
	{
		TestTrue(TEXT("bDoCollisionTest is true"), Character->CameraBoom->bDoCollisionTest);
		TestEqual(TEXT("ProbeChannel is ECC_Camera"), static_cast<int32>(Character->CameraBoom->ProbeChannel.GetValue()), static_cast<int32>(ECC_Camera));
		TestEqual(TEXT("ProbeSize is sourced from CameraSettings, not a constructor literal"), Character->CameraBoom->ProbeSize, 18.0f);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMoonCameraDestructibleIgnoreProfileTest,
	"Moon.Camera.CollisionGuardrails.AC2_DestructibleIgnoresCameraProbe",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMoonCameraDestructibleIgnoreProfileTest::RunTest(const FString& Parameters)
{
	// AC-2 (GDD Edge Case 3): the project's Config/DefaultEngine.ini +EditProfiles entry must patch
	// the stock "Destructible" collision profile's response to the Camera channel to Ignore, while
	// leaving WorldStatic/WorldDynamic untouched (still Block) — this is a real, loaded-from-ini
	// check against UCollisionProfile, not an assumption placeholder. No physics world/actor spawn
	// is needed: the collision profile table is populated at engine init from
	// BaseEngine.ini + DefaultEngine.ini before any test runs.
	FCollisionResponseTemplate DestructibleTemplate;
	const bool bFound = UCollisionProfile::Get()->GetProfileTemplate(FName(TEXT("Destructible")), DestructibleTemplate);
	TestTrue(TEXT("\"Destructible\" collision profile is registered"), bFound);
	if (!bFound)
	{
		return false;
	}

	TestEqual(TEXT("Destructible profile ignores the Camera channel (Config/DefaultEngine.ini +EditProfiles)"),
		static_cast<int32>(DestructibleTemplate.ResponseToChannels.GetResponse(ECC_Camera)), static_cast<int32>(ECR_Ignore));
	TestEqual(TEXT("Destructible profile still blocks WorldStatic (only Camera is ignored, not all channels)"),
		static_cast<int32>(DestructibleTemplate.ResponseToChannels.GetResponse(ECC_WorldStatic)), static_cast<int32>(ECR_Block));
	TestEqual(TEXT("Destructible profile still blocks WorldDynamic (only Camera is ignored, not all channels)"),
		static_cast<int32>(DestructibleTemplate.ResponseToChannels.GetResponse(ECC_WorldDynamic)), static_cast<int32>(ECR_Block));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMoonCameraCornerDitherThresholdTest,
	"Moon.Camera.CollisionGuardrails.AC4_CornerDitherTargetAlpha",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMoonCameraCornerDitherThresholdTest::RunTest(const FString& Parameters)
{
	// AC-4 (QA-TEST-10, GDD Edge Case 1): the pure/stateless step function that
	// UpdateCameraCornerDither() interpolates toward every tick. Testing it directly (no Tick, no
	// physics scene, no spawned actor) is exactly why it was split out as a static pure function —
	// see its header doc comment.
	TestEqual(TEXT("At the threshold exactly (80uu <= 80uu), target alpha is 1.0 (dithered)"),
		AMoonCharacterBase_CollisionGuardrailsTestAccessor::ComputeCornerDitherTargetAlpha(80.0f, 80.0f), 1.0f);
	TestEqual(TEXT("Below the threshold, target alpha is 1.0 (dithered)"),
		AMoonCharacterBase_CollisionGuardrailsTestAccessor::ComputeCornerDitherTargetAlpha(40.0f, 80.0f), 1.0f);
	TestEqual(TEXT("Above the threshold, target alpha is 0.0 (recovered/opaque) — must fade back out, not stay stuck transparent"),
		AMoonCharacterBase_CollisionGuardrailsTestAccessor::ComputeCornerDitherTargetAlpha(81.0f, 80.0f), 0.0f);
	TestEqual(TEXT("Well above the threshold (full extension), target alpha is 0.0"),
		AMoonCharacterBase_CollisionGuardrailsTestAccessor::ComputeCornerDitherTargetAlpha(452.6f, 80.0f), 0.0f);

	return true;
}

#endif
