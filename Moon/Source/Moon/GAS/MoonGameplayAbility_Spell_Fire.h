#pragma once

#include "CoreMinimal.h"
#include "MoonGameplayAbility_Spell.h"
#include "MoonGameplayAbility_Spell_Fire.generated.h"

/** Fire step of the disposable signature-chain engine spike. */
UCLASS()
class MOON_API UMoonGameplayAbility_Spell_Fire : public UMoonGameplayAbility_Spell
{
	GENERATED_BODY()
public:
	UMoonGameplayAbility_Spell_Fire();
protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	UPROPERTY(EditDefaultsOnly, Category = "Moon|Signature Chain|Spike") float Damage = 25.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Moon|Signature Chain|Spike") float ManaCostForTension = 25.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Moon|Signature Chain|Spike") float TraceRange = 600.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Moon|Signature Chain|Spike") float TraceRadius = 120.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Moon|Signature Chain|Spike") float PillarSearchRadius = 350.0f;
};
