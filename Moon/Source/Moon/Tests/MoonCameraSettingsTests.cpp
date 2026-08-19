#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "../Camera/MoonCameraSettings.h"

// Story 001 AC-3: UMoonCameraSettings' 11 documented Tuning Knobs fields (design/gdd/
// camera-system-base.md section 7 — see that class's header comment for the "12 vs 11" count
// discrepancy this story flagged rather than silently resolved) must default to the currently
// shipped, hand-tuned values. A default-constructed instance needs no World/Actor, so this is a
// genuine compiled Automation Spec test (no static-analysis substitute needed for this AC).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMoonCameraSettingsDefaultsTest,
	"Moon.Camera.Settings.ShippedDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMoonCameraSettingsDefaultsTest::RunTest(const FString& Parameters)
{
	const UMoonCameraSettings* Settings = NewObject<UMoonCameraSettings>(GetTransientPackage());
	TestNotNull(TEXT("UMoonCameraSettings can be constructed"), Settings);
	if (!Settings)
	{
		return false;
	}

	TestEqual(TEXT("TargetArmLength matches shipped feel"), Settings->TargetArmLength, 450.0f);
	TestEqual(TEXT("CameraSocketOffset matches shipped feel"), Settings->CameraSocketOffset, FVector(0.0f, 45.0f, 20.0f));
	TestEqual(TEXT("CameraPitchMin matches GDD Tuning Knobs table"), Settings->CameraPitchMin, -60.0f);
	TestEqual(TEXT("CameraPitchMax matches GDD Tuning Knobs table"), Settings->CameraPitchMax, 30.0f);
	TestEqual(TEXT("CameraLagSpeed matches shipped feel"), Settings->CameraLagSpeed, 18.0f);
	TestEqual(TEXT("CameraRotationLagSpeed matches GDD Tuning Knobs table"), Settings->CameraRotationLagSpeed, 15.0f);
	TestEqual(TEXT("CameraLagMaxDistance matches shipped feel"), Settings->CameraLagMaxDistance, 60.0f);
	TestEqual(TEXT("BaseFOV matches GDD Tuning Knobs table"), Settings->BaseFOV, 90.0f);
	TestEqual(TEXT("OverdriveFOV matches GDD Tuning Knobs table"), Settings->OverdriveFOV, 100.0f);
	TestEqual(TEXT("ExecutionArmLength matches GDD Tuning Knobs table"), Settings->ExecutionArmLength, 150.0f);
	TestEqual(TEXT("CameraProbeSize matches GDD Tuning Knobs table"), Settings->CameraProbeSize, 12.0f);

	// AC-3 edge case: no field left at a C++ zero-initialized default. Every one of the 11 fields
	// above has a nonzero GDD value, so a bulk zero-check doubles as the "missed field" guard.
	TestNotEqual(TEXT("TargetArmLength is not zero-initialized"), Settings->TargetArmLength, 0.0f);
	TestNotEqual(TEXT("CameraSocketOffset is not zero-initialized"), Settings->CameraSocketOffset, FVector::ZeroVector);
	TestNotEqual(TEXT("CameraLagSpeed is not zero-initialized"), Settings->CameraLagSpeed, 0.0f);
	TestNotEqual(TEXT("CameraRotationLagSpeed is not zero-initialized"), Settings->CameraRotationLagSpeed, 0.0f);
	TestNotEqual(TEXT("CameraLagMaxDistance is not zero-initialized"), Settings->CameraLagMaxDistance, 0.0f);
	TestNotEqual(TEXT("BaseFOV is not zero-initialized"), Settings->BaseFOV, 0.0f);
	TestNotEqual(TEXT("OverdriveFOV is not zero-initialized"), Settings->OverdriveFOV, 0.0f);
	TestNotEqual(TEXT("ExecutionArmLength is not zero-initialized"), Settings->ExecutionArmLength, 0.0f);
	TestNotEqual(TEXT("CameraProbeSize is not zero-initialized"), Settings->CameraProbeSize, 0.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMoonCameraSettingsCornerDitherValidationTest,
	"Moon.Camera.Settings.RejectsInvalidCornerDitherValues",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMoonCameraSettingsCornerDitherValidationTest::RunTest(const FString& Parameters)
{
	UMoonCameraSettings* Settings = NewObject<UMoonCameraSettings>(GetTransientPackage());
	TestNotNull(TEXT("UMoonCameraSettings can be constructed for validation"), Settings);
	if (!Settings)
	{
		return false;
	}

	FString FailureReason;
	Settings->CornerDitherThreshold = 0.0f;
	TestFalse(TEXT("A zero dither threshold is rejected"), Settings->IsWithinSafeRanges(FailureReason));

	Settings->CornerDitherThreshold = 80.0f;
	Settings->CameraNearClipPlane = -1.0f;
	TestFalse(TEXT("A negative near clip plane is rejected"), Settings->IsWithinSafeRanges(FailureReason));

	Settings->CameraNearClipPlane = 10.0f;
	Settings->CornerDitherFadeSpeed = 0.0f;
	TestFalse(TEXT("A zero dither fade speed is rejected"), Settings->IsWithinSafeRanges(FailureReason));

	Settings->CornerDitherFadeSpeed = 8.0f;
	TestTrue(TEXT("The shipped corner-dither values remain valid"), Settings->IsWithinSafeRanges(FailureReason));
	return true;
}

#endif
