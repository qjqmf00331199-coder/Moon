#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "MoonAbilitySystemComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MOON_API UMoonAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UMoonAbilitySystemComponent();
};
