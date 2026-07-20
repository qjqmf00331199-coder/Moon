#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "MoonGameplayAbility_Dash.generated.h"

/**
 * Dash & Evasion Ability
 * Implements Dash charges, impulse override, and i-frame application.
 */
UCLASS()
class MOON_API UMoonGameplayAbility_Dash : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UMoonGameplayAbility_Dash();

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	// Tuning parameters from GDD
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash")
	float DashSpeedMultiplier = 2.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash")
	float DashDuration = 0.2f;

	// Timer handle for ending the dash
	FTimerHandle DashTimerHandle;

	// Ends the dash after duration
	void OnDashFinished();

	// Applies the impulse to the character
	void ApplyDashImpulse(class ACharacter* Character) const;

	// Core logic for checking Just-Dodge window
	// Will be fully implemented when Enemy AI is ready
	void CheckJustDodge(class ACharacter* PlayerCharacter);
};
