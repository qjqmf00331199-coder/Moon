#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "MoonAttributeSet.generated.h"

// Macro to define standard getter/setter/init functions for attributes
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class MOON_API UMoonAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UMoonAttributeSet();

	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Health")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UMoonAttributeSet, Health)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Health")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UMoonAttributeSet, MaxHealth)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Mana")
	FGameplayAttributeData Mana;
	ATTRIBUTE_ACCESSORS(UMoonAttributeSet, Mana)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Mana")
	FGameplayAttributeData MaxMana;
	ATTRIBUTE_ACCESSORS(UMoonAttributeSet, MaxMana)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Mana")
	FGameplayAttributeData ManaRegenRate;
	ATTRIBUTE_ACCESSORS(UMoonAttributeSet, ManaRegenRate)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Dash")
	FGameplayAttributeData DashCharges;
	ATTRIBUTE_ACCESSORS(UMoonAttributeSet, DashCharges)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Dash")
	FGameplayAttributeData MaxDashCharges;
	ATTRIBUTE_ACCESSORS(UMoonAttributeSet, MaxDashCharges)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Dash")
	FGameplayAttributeData DashRechargeRate;
	ATTRIBUTE_ACCESSORS(UMoonAttributeSet, DashRechargeRate)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Tension")
	FGameplayAttributeData TensionGauge;
	ATTRIBUTE_ACCESSORS(UMoonAttributeSet, TensionGauge)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Tension")
	FGameplayAttributeData TensionGaugeMax;
	ATTRIBUTE_ACCESSORS(UMoonAttributeSet, TensionGaugeMax)

	// Virtual functions for clamping and rules
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;
};
