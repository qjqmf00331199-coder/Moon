#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_SpellCost_Blackhole.generated.h"

/**
 * Mana cost Gameplay Effect for the Blackhole spell.
 *
 * What it modifies: Mana, Additive, -70.0 (matches design/gdd/spell-casting-base.md Tuning Knobs
 *                    — Blackhole ManaCost = 70).
 * Stacking:          N/A (Instant effects do not stack).
 * Duration:          Instant (applies once, immediately, no persistent state).
 * Removal:           N/A (Instant effects are not "active" and cannot be removed).
 *
 * Assigned as UMoonGameplayAbility_Spell_Blackhole::CostGameplayEffectClass. Applied atomically by
 * UGameplayAbility::CommitAbility — never subtract Mana by writing to the attribute directly.
 *
 * NOTE: the modifier magnitude is set directly in this C++ constructor rather than authored as a
 * Blueprint data-only GE. This is an intentional minimal-slice exception for this smoke-test task
 * (no .uasset/Blueprint edits permitted). If this spell's tuning moves under active iteration,
 * consider converting to a Blueprint subclass of a shared "cost GE" base per normal project
 * convention.
 */
UCLASS()
class MOON_API UGE_SpellCost_Blackhole : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGE_SpellCost_Blackhole();
};
