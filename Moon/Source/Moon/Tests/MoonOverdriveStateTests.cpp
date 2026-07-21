#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "../GAS/MoonOverdriveState.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMoonOverdriveFixedWindowTest,
	"Moon.Combat.Overdrive.FixedWindow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMoonOverdriveFixedWindowTest::RunTest(const FString& Parameters)
{
	FMoonOverdriveState State;

	TestTrue(TEXT("Inactive state starts once"), State.TryStart(100.0, 10.0f));
	TestEqual(TEXT("Active phase entered"), State.GetPhase(), EMoonOverdrivePhase::Active);
	TestEqual(TEXT("End time is fixed at start + duration"), State.GetActiveEndTime(), 110.0);

	TestFalse(TEXT("Active retrigger is ignored"), State.TryStart(104.0, 10.0f));
	TestEqual(TEXT("Ignored retrigger does not extend end time"), State.GetActiveEndTime(), 110.0);
	TestTrue(TEXT("Active immediately before the boundary"), State.IsActive(109.999));
	TestFalse(TEXT("Not active at the exact boundary"), State.IsActive(110.0));
	TestEqual(TEXT("Remaining time clamps to zero at the boundary"), State.GetActiveTimeRemaining(110.0), 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMoonOverdriveRecoveryTest,
	"Moon.Combat.Overdrive.RecoveryBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMoonOverdriveRecoveryTest::RunTest(const FString& Parameters)
{
	FMoonOverdriveState State;
	State.TryStart(0.0, 10.0f);

	TestFalse(TEXT("Cannot expire before end time"), State.TryExpire(9.999, 1.5f));
	TestTrue(TEXT("Expires at exact end time"), State.TryExpire(10.0, 1.5f));
	TestEqual(TEXT("Recovery phase entered"), State.GetPhase(), EMoonOverdrivePhase::Recovery);
	TestEqual(TEXT("Recovery end time is deterministic"), State.GetRecoveryEndTime(), 11.5);
	TestTrue(TEXT("Tension locked during Recovery"), State.IsTensionGainLocked(11.499));
	TestFalse(TEXT("Recovery trigger is ignored"), State.TryStart(11.0, 10.0f));
	TestFalse(TEXT("Recovery cannot finish early"), State.TryFinishRecovery(11.499));
	TestTrue(TEXT("Recovery finishes at exact boundary"), State.TryFinishRecovery(11.5));
	TestEqual(TEXT("Returns to Inactive"), State.GetPhase(), EMoonOverdrivePhase::Inactive);
	TestFalse(TEXT("Tension unlocks after Recovery"), State.IsTensionGainLocked(11.5));
	return true;
}

#endif
