#include "MoonGameplayAbility_Spell.h"
#include "AbilitySystemComponent.h"
#include "../Character/MoonCharacterBase.h"

UMoonGameplayAbility_Spell::UMoonGameplayAbility_Spell()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	CostBypassTag = FGameplayTag::RequestGameplayTag(FName("CostBypass.Active"), false);
}

bool UMoonGameplayAbility_Spell::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	bool bCanActivate = Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);

	// If the normal checks fail (e.g. because of Cost/Cooldown), but we have the bypass tag, we can activate anyway.
	if (!bCanActivate && ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		if (CostBypassTag.IsValid() && ActorInfo->AbilitySystemComponent->HasMatchingGameplayTag(CostBypassTag))
		{
			// We have the bypass tag, so we ignore Cost and Cooldown failures.
			// Note: We still need to respect tags that block the ability entirely.
			if (DoesAbilitySatisfyTagRequirements(*ActorInfo->AbilitySystemComponent, SourceTags, TargetTags, OptionalRelevantTags))
			{
				bCanActivate = true;
			}
		}
	}

	// Rate Limiting (Rule 11: per-element per-frame cap, global 20 CPS cap)
	if (bCanActivate && ActorInfo && ActorInfo->AvatarActor.IsValid())
	{
		AMoonCharacterBase* MoonCharacter = Cast<AMoonCharacterBase>(ActorInfo->AvatarActor.Get());
		if (MoonCharacter)
		{
			// Extract Element Tag (Spell.Element.*)
			FGameplayTag ElementTag;
			for (const FGameplayTag& Tag : GetAssetTags())
			{
				if (Tag.MatchesTag(FGameplayTag::RequestGameplayTag(FName("Spell.Element"))))
				{
					ElementTag = Tag;
					break;
				}
			}

			// Perform Rate Limit check (this consumes the limit if successful)
			if (!MoonCharacter->CheckAndConsumeSpellCastLimit(ElementTag))
			{
				bCanActivate = false;
			}
		}
	}

	return bCanActivate;
}

bool UMoonGameplayAbility_Spell::CommitAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, OUT FGameplayTagContainer* OptionalRelevantTags)
{
	// Check for Bypass
	bool bBypassCostAndCooldown = false;
	if (CostBypassTag.IsValid() && ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		bBypassCostAndCooldown = ActorInfo->AbilitySystemComponent->HasMatchingGameplayTag(CostBypassTag);
	}

	if (bBypassCostAndCooldown)
	{
		// Bypass normal commit which applies GE Cost/Cooldown
		return true;
	}

	return Super::CommitAbility(Handle, ActorInfo, ActivationInfo, OptionalRelevantTags);
}
