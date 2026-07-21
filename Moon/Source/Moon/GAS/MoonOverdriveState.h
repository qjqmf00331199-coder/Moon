#pragma once

#include "CoreMinimal.h"
#include "MoonOverdriveState.generated.h"

UENUM(BlueprintType)
enum class EMoonOverdrivePhase : uint8
{
	Inactive,
	Active,
	Recovery
};

UENUM(BlueprintType)
enum class EMoonOverdriveEndReason : uint8
{
	Expired,
	PlayerDeath
};

/**
 * Pure time-state for Luna Overdrive. Gameplay-tag ownership and event dispatch stay on the
 * character; this struct keeps the fixed-window rules deterministic and automation-testable.
 */
struct FMoonOverdriveState
{
	bool TryStart(double CurrentTime, float ActiveDuration);
	bool TryExpire(double CurrentTime, float RecoveryDuration);
	bool TryFinishRecovery(double CurrentTime);
	void Reset();

	bool IsActive(double CurrentTime) const;
	bool IsTensionGainLocked(double CurrentTime) const;
	float GetActiveTimeRemaining(double CurrentTime) const;

	EMoonOverdrivePhase GetPhase() const { return Phase; }
	double GetActiveEndTime() const { return ActiveEndTime; }
	double GetRecoveryEndTime() const { return RecoveryEndTime; }

private:
	EMoonOverdrivePhase Phase = EMoonOverdrivePhase::Inactive;
	double ActiveEndTime = 0.0;
	double RecoveryEndTime = 0.0;
};
