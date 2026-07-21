#include "GE_Damage_Instant.h"
#include "../MoonAttributeSet.h"

UGE_Damage_Instant::UGE_Damage_Instant()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FSetByCallerFloat DamageMagnitude;
	DamageMagnitude.DataTag = FGameplayTag::RequestGameplayTag(FName("Data.Damage"));

	FGameplayModifierInfo DamageModifier;
	DamageModifier.Attribute = UMoonAttributeSet::GetHealthAttribute();
	DamageModifier.ModifierOp = EGameplayModOp::Additive;
	DamageModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(DamageMagnitude);

	Modifiers.Add(DamageModifier);
}
