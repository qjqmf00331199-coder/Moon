#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_CoreExtractionReward.generated.h"

/** Instant, SetByCaller mana reward used by the signature-chain engine spike. */
UCLASS()
class MOON_API UGE_CoreExtractionReward : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGE_CoreExtractionReward();
};
