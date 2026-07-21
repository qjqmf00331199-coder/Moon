#include "GE_SpellCooldown_Fire.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_SpellCooldown_Fire::UGE_SpellCooldown_Fire()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(3.0f));
}

void UGE_SpellCooldown_Fire::PostInitProperties()
{
	Super::PostInitProperties();
	UTargetTagsGameplayEffectComponent& Component = AddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer Changes;
	Changes.Added.AddTag(FGameplayTag::RequestGameplayTag(FName("Cooldown.Spell.Fire")));
	Component.SetAndApplyTargetTagChanges(Changes);
}
