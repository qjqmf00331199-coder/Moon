#include "GE_SpellCost_Fire.h"
#include "../MoonAttributeSet.h"

UGE_SpellCost_Fire::UGE_SpellCost_Fire()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UMoonAttributeSet::GetManaAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-25.0f));
	Modifiers.Add(Modifier);
}
