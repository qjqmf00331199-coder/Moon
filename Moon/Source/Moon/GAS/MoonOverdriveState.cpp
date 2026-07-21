#include "MoonOverdriveState.h"

bool FMoonOverdriveState::TryStart(double CurrentTime, float ActiveDuration)
{
	if (Phase != EMoonOverdrivePhase::Inactive || ActiveDuration <= 0.0f)
	{
		return false;
	}

	Phase = EMoonOverdrivePhase::Active;
	ActiveEndTime = CurrentTime + static_cast<double>(ActiveDuration);
	RecoveryEndTime = 0.0;
	return true;
}

bool FMoonOverdriveState::TryExpire(double CurrentTime, float RecoveryDuration)
{
	if (Phase != EMoonOverdrivePhase::Active || CurrentTime < ActiveEndTime)
	{
		return false;
	}

	if (RecoveryDuration > 0.0f)
	{
		Phase = EMoonOverdrivePhase::Recovery;
		RecoveryEndTime = CurrentTime + static_cast<double>(RecoveryDuration);
	}
	else
	{
		Reset();
	}
	return true;
}

bool FMoonOverdriveState::TryFinishRecovery(double CurrentTime)
{
	if (Phase != EMoonOverdrivePhase::Recovery || CurrentTime < RecoveryEndTime)
	{
		return false;
	}

	Reset();
	return true;
}

void FMoonOverdriveState::Reset()
{
	Phase = EMoonOverdrivePhase::Inactive;
	ActiveEndTime = 0.0;
	RecoveryEndTime = 0.0;
}

bool FMoonOverdriveState::IsActive(double CurrentTime) const
{
	return Phase == EMoonOverdrivePhase::Active && CurrentTime < ActiveEndTime;
}

bool FMoonOverdriveState::IsTensionGainLocked(double CurrentTime) const
{
	if (Phase == EMoonOverdrivePhase::Active)
	{
		return true;
	}

	return Phase == EMoonOverdrivePhase::Recovery && CurrentTime < RecoveryEndTime;
}

float FMoonOverdriveState::GetActiveTimeRemaining(double CurrentTime) const
{
	return IsActive(CurrentTime)
		? static_cast<float>(FMath::Max(0.0, ActiveEndTime - CurrentTime))
		: 0.0f;
}
