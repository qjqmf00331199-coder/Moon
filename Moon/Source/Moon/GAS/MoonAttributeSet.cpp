#include "MoonAttributeSet.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"
#include "Math/UnrealMathUtility.h"

UMoonAttributeSet::UMoonAttributeSet()
{
	// Initialization values (these will ideally be overridden by a GE_Init data setup later)
	InitHealth(100.0f);
	InitMaxHealth(100.0f);
	InitMana(100.0f);
	InitMaxMana(100.0f);
	InitManaRegenRate(8.0f);
	InitDashCharges(2.0f);
	InitMaxDashCharges(2.0f);
	InitDashRechargeRate(2.0f);
	InitTensionGauge(0.0f);
	InitTensionGaugeMax(100.0f);
}

void UMoonAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	// Pre-clamp base values before change is applied
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	else if (Attribute == GetManaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxMana());
	}
	else if (Attribute == GetDashChargesAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxDashCharges());
	}
	else if (Attribute == GetTensionGaugeAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetTensionGaugeMax());
	}
}

void UMoonAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	// Post-clamp attributes after a GE execution
	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
	}
	else if (Data.EvaluatedData.Attribute == GetManaAttribute())
	{
		SetMana(FMath::Clamp(GetMana(), 0.0f, GetMaxMana()));
	}
	else if (Data.EvaluatedData.Attribute == GetDashChargesAttribute())
	{
		SetDashCharges(FMath::Clamp(GetDashCharges(), 0.0f, GetMaxDashCharges()));
	}
	else if (Data.EvaluatedData.Attribute == GetTensionGaugeAttribute())
	{
		SetTensionGauge(FMath::Clamp(GetTensionGauge(), 0.0f, GetTensionGaugeMax()));
	}
}
