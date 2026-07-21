#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_SpellCost_Fire.generated.h"

/** Engine-spike Fire mana cost. */
UCLASS()
class MOON_API UGE_SpellCost_Fire : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_SpellCost_Fire();
};
