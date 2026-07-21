#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_SpellCooldown_Fire.generated.h"

/** Engine-spike Fire cooldown. */
UCLASS()
class MOON_API UGE_SpellCooldown_Fire : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UGE_SpellCooldown_Fire();
	virtual void PostInitProperties() override;
};
