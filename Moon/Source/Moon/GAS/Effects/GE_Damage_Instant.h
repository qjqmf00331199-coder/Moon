#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_Damage_Instant.generated.h"

/**
 * Shared single-entry-point damage Gameplay Effect.
 *
 * Per design/gdd/health-damage-core.md Rule 1: all damage (spells, melee, everything) must apply
 * Health changes through exactly one Gameplay Effect, never by writing to the Health attribute
 * directly. This class is that entry point — do not create a second "damage GE" class; extend
 * this one (e.g. with an Execution Calculation) if more complex damage math is needed later.
 *
 * What it modifies: Health, Additive, magnitude is SetByCaller (tag "Data.Damage"). Callers pass a
 *                    negative magnitude to deal damage (e.g. SetSetByCallerMagnitude(Data.Damage,
 *                    -Amount)).
 * Stacking:          N/A (Instant effects do not stack).
 * Duration:          Instant.
 * Removal:           N/A (Instant effects are not "active" and cannot be removed).
 *
 * This minimal slice does not implement shield/invulnerable interception (health-damage-core.md's
 * EffectiveDamageApplied formula) — there is no shield/invulnerable target yet, so this is a
 * straight modifier. When that system exists, it should intercept via an Execution Calculation or
 * a PreAttributeChange/PostGameplayEffectExecute hook on the receiving AttributeSet, not by adding
 * a second damage GE.
 */
UCLASS()
class MOON_API UGE_Damage_Instant : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGE_Damage_Instant();
};
