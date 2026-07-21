#include "GE_CoreExtractionReward.h"

#include "../MoonAttributeSet.h"

UGE_CoreExtractionReward::UGE_CoreExtractionReward()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FSetByCallerFloat ManaRestoreMagnitude;
	ManaRestoreMagnitude.DataTag = FGameplayTag::RequestGameplayTag(FName("Data.ManaRestore"));

	FGameplayModifierInfo ManaModifier;
	ManaModifier.Attribute = UMoonAttributeSet::GetManaAttribute();
	ManaModifier.ModifierOp = EGameplayModOp::Additive;
	ManaModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(ManaRestoreMagnitude);
	Modifiers.Add(ManaModifier);
}
