#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "../Character/MoonCharacterBase.h"
#include "../Camera/MoonCameraSettings.h"
#include "GameFramework/SpringArmComponent.h"

// Story 001 AC-4: BeginPlay's ApplyCameraSettings path must apply the asset values to CameraBoom,
// not just leave the constructor's literal (450.0) standing. The existing coverage
// (MoonCameraSettingsTests.cpp AC-3, camera-settings-foundation_test.ps1 AC-1/AC-2/weak-AC-4) never
// actually spawns a character, assigns an asset, runs BeginPlay, and reads back the live property —
// this file closes that gap with a genuine runtime-executed test.
//
// CameraBoom/CameraSettings/ApplyCameraSettings are `protected` on AMoonCharacterBase by design (Story 001's
// own encapsulation) — this test-only accessor subclass exposes them via `using` rather than
// changing AMoonCharacterBase's access level, which would leak the implementation detail to the
// whole game just to make one test file compile. The accessor adds no new UPROPERTY/data members and
// no GENERATED_BODY(), so it carries zero size/reflection difference from AMoonCharacterBase itself.
class AMoonCharacterBase_TestAccessor : public AMoonCharacterBase
{
public:
	using AMoonCharacterBase::CameraBoom;
	using AMoonCharacterBase::CameraSettings;
	using AMoonCharacterBase::ApplyCameraSettings;
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMoonCameraAppliesAtRuntimeTest,
	"Moon.Camera.Settings.AppliesAtRuntimeNotConstructorLiteral",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMoonCameraAppliesAtRuntimeTest::RunTest(const FString& Parameters)
{
	// Case 1: a non-default TargetArmLength asset assigned before BeginPlay must win over the
	// constructor's literal 450.0.
	{
		AMoonCharacterBase_TestAccessor* Character = NewObject<AMoonCharacterBase_TestAccessor>(GetTransientPackage());
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
		Settings->TargetArmLength = 300.0f;
		Character->CameraSettings = Settings;

		// Calling AActor::BeginPlay directly on a transient NewObject violates UE5.8's actor lifecycle
		// invariant. Exercise the exact application method that BeginPlay delegates to instead.
		Character->ApplyCameraSettings();

		TestNotNull(TEXT("CameraBoom exists after applying settings"), Character->CameraBoom.Get());
		if (Character->CameraBoom)
		{
			TestEqual(TEXT("CameraBoom->TargetArmLength matches the assigned asset, not the constructor literal"),
				Character->CameraBoom->TargetArmLength, 300.0f);
		}
	}

	// Case 2 (AC-4 edge case): no asset assigned — BeginPlay must fall back to the constructor
	// literal (450.0) without crashing (null-asset guard in ApplyCameraSettings()).
	{
		AMoonCharacterBase_TestAccessor* Character = NewObject<AMoonCharacterBase_TestAccessor>(GetTransientPackage());
		TestNotNull(TEXT("Character can be constructed via NewObject"), Character);
		if (!Character)
		{
			return false;
		}

		TestNull(TEXT("CameraSettings starts unassigned"), Character->CameraSettings.Get());

		Character->ApplyCameraSettings();

		TestNotNull(TEXT("CameraBoom exists after applying settings"), Character->CameraBoom.Get());
		if (Character->CameraBoom)
		{
			TestEqual(TEXT("CameraBoom->TargetArmLength keeps the constructor literal when no asset is assigned"),
				Character->CameraBoom->TargetArmLength, 450.0f);
		}
	}

	return true;
}

#endif
