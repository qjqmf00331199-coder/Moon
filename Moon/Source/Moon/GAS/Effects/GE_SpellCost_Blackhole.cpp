#include "GE_SpellCost_Blackhole.h"
#include "../MoonAttributeSet.h"

UGE_SpellCost_Blackhole::UGE_SpellCost_Blackhole()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo ManaCostModifier;
	ManaCostModifier.Attribute = UMoonAttributeSet::GetManaAttribute();
	ManaCostModifier.ModifierOp = EGameplayModOp::Additive;
	// -70 Mana, matches design/gdd/spell-casting-base.md Tuning Knobs (Blackhole ManaCost = 70).
	ManaCostModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-70.0f));

	Modifiers.Add(ManaCostModifier);
}
