#pragma once

#include "CoreMinimal.h"
#include "MoonGameplayAbility_Spell.h"
#include "MoonGameplayAbility_Spell_Blackhole.generated.h"

/**
 * Blackhole spell ability — minimal smoke-test slice.
 *
 * Flow: CommitAbility (applies GE_SpellCost_Blackhole -70 Mana and GE_SpellCooldown_Blackhole 6s
 * cooldown, respecting the base class's CostBypass.Active bypass) -> forward sphere trace for the
 * first actor implementing IAbilitySystemInterface -> on hit, apply GE_Damage_Instant (SetByCaller
 * Data.Damage = -PlaceholderDamage) to the target and grant Tension via
 * AMoonCharacterBase::AddTensionFromSpellHit (per spell-casting-base.md, Tension accrues on
 * OnSpellHit, not OnSpellCast — a whiff grants no Tension) -> EndAbility (always, hit or whiff).
 *
 * Trace shape/range and PlaceholderDamage are smoke-test values only, not final VFX/hit-detection
 * or balance design (owned by the not-yet-designed Spell Weaving/balance system).
 */
UCLASS()
class MOON_API UMoonGameplayAbility_Spell_Blackhole : public UMoonGameplayAbility_Spell
{
	GENERATED_BODY()

public:
	UMoonGameplayAbility_Spell_Blackhole();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	// PLACEHOLDER: not a real balance value, just a smoke-test magnitude. Real spell damage output
	// is owned by the Spell Weaving/balance system, which is not yet designed as of this writing.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Moon|Spell|Blackhole")
	float PlaceholderDamage = 25.0f;

	// Smoke-test trace shape, not final hit-detection design.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Moon|Spell|Blackhole")
	float TraceRange = 500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Moon|Spell|Blackhole")
	float TraceRadius = 100.0f;

	// Tension granted on a successful hit (spell-casting-base.md: Tension = f(ManaCost)); matches
	// the Blackhole ManaCost tuning knob (70), NOT PlaceholderDamage.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Moon|Spell|Blackhole")
	float ManaCostForTension = 70.0f;

	// Cast motion, played on the character mesh via AMoonCharacterBase::PlayOneShotAnim.
	// Defaults to Aurora's generic "Cast" animation (no purpose-built Blackhole cast anim yet).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Moon|Spell|Blackhole")
	TObjectPtr<class UAnimSequence> CastAnim;
};
