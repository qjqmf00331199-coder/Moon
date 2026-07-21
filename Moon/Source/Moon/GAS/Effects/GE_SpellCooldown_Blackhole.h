#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_SpellCooldown_Blackhole.generated.h"

/**
 * Cooldown Gameplay Effect for the Blackhole spell.
 *
 * What it modifies: nothing (no attribute Modifiers) — it exists purely to grant a tracking tag.
 * Stacking:          N/A (only one instance should ever be active; re-casting is blocked by
 *                    CanActivateAbility's cooldown check before a second instance could apply).
 * Duration:          HasDuration, 6.0s (matches design/gdd/spell-casting-base.md Tuning Knobs
 *                    — Blackhole CooldownDuration = 6.0s).
 * Removal:           Automatically removed when its Duration expires (standard GE duration
 *                    expiry); no manual removal needed.
 *
 * Grants Cooldown.Spell.Blackhole to the caster for the GE's duration. UGameplayAbility's default
 * GetCooldownTags()/CheckCooldown() read UGameplayEffect::GetGrantedTags(), which is populated by
 * the UTargetTagsGameplayEffectComponent added in the constructor below.
 *
 * UE5.8 API note: the legacy monolithic "InheritableOwnedTagsContainer" property that used to
 * carry a GE's granted tags directly is UE_DEPRECATED(5.3, ...) in this engine version — verified
 * against the installed 5.8 engine headers (GameplayEffect.h). Granting tags now goes through the
 * GameplayEffectComponent architecture (UTargetTagsGameplayEffectComponent::SetAndApplyTargetTagChanges,
 * which writes into UGameplayEffect::CachedGrantedTags — see GetGrantedTags()). Do not use the
 * deprecated property even though it still compiles.
 *
 * UE5.8 API note #2: UGameplayEffect::AddComponent<T>() calls NewObject(this, NAME_None, ...)
 * internally, and 5.8's CoreUObject added a new guard (UObjectGlobals.h, FObjectInitializer::
 * AssertIfInConstructor) that fatal-errors on exactly that call shape when it runs inside the
 * owning object's own constructor — verified against installed 5.8 engine source
 * (UObjectGlobals.cpp:4877). This breaks the standard 5.3-5.7 "AddComponent in the GE
 * constructor" tutorial pattern. Fix: add components from PostInitProperties() instead — by
 * the time it runs, FObjectInitializer's destructor has already decremented IsInConstructor/
 * reset ConstructedObject (UObjectGlobals.cpp ~4051), so the guard no longer fires.
 *
 * Assigned as UMoonGameplayAbility_Spell_Blackhole::CooldownGameplayEffectClass.
 */
UCLASS()
class MOON_API UGE_SpellCooldown_Blackhole : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGE_SpellCooldown_Blackhole();

	virtual void PostInitProperties() override;
};
