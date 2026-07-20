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

	// Dash motion, played on the character mesh via AMoonCharacterBase::PlayOneShotAnim.
	// Defaults to the Aurora "Ability_RMB_Fwd" animation (project has no dedicated Dash anim yet).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash|Animation")
	TObjectPtr<class UAnimSequence> DashAnim;

	// Playback speed multiplier for DashAnim — sped up so a 1.9s animation reads as a quick
	// forward thrust instead of a slow, obviously-borrowed full-length ability animation.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash|Animation")
	float DashAnimPlayRate = 4.0f;

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
