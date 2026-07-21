#include "GE_SpellCooldown_Blackhole.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_SpellCooldown_Blackhole::UGE_SpellCooldown_Blackhole()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	// 6.0s, matches design/gdd/spell-casting-base.md Tuning Knobs (Blackhole CooldownDuration).
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(6.0f));
}

void UGE_SpellCooldown_Blackhole::PostInitProperties()
{
	Super::PostInitProperties();

	// UE5.8 tag-granting pattern (see class comment in the header for why this replaces the
	// deprecated InheritableOwnedTagsContainer property, and why it must run here rather than
	// in the constructor).
	UTargetTagsGameplayEffectComponent& CooldownTagsComponent = AddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer CooldownTagChanges;
	CooldownTagChanges.Added.AddTag(FGameplayTag::RequestGameplayTag(FName("Cooldown.Spell.Blackhole")));
	CooldownTagsComponent.SetAndApplyTargetTagChanges(CooldownTagChanges);
}
