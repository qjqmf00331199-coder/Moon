#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "MoonGameplayAbility_Spell.generated.h"

/**
 * Base class for all Spell Casting abilities in Moon Fragment Hunt.
 * Handles custom Cost and Cooldown bypass logic for Luna Overdrive.
 */
UCLASS()
class MOON_API UMoonGameplayAbility_Spell : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UMoonGameplayAbility_Spell();

	/** Overridden to check for CostBypass.Active */
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	/** Overridden to conditionally apply Cost and Cooldown */
	virtual bool CommitAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) override;

protected:
	bool IsCostBypassActive(const FGameplayAbilityActorInfo* ActorInfo) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Moon|Spell")
	FGameplayTag CostBypassTag;
};
