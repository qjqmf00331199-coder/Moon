#include "MoonCombatHUDWidget.h"
#include "../Character/MoonCharacterBase.h"
#include "../GAS/MoonAbilitySystemComponent.h"
#include "../GAS/MoonAttributeSet.h"
#include "GameplayEffectTypes.h"
#include "GameplayEffectExtension.h"

void UMoonCombatHUDWidget::BindToPlayer(APawn* PlayerPawn)
{
	AMoonCharacterBase* Character = Cast<AMoonCharacterBase>(PlayerPawn);
	if (!Character) return;

	BoundCharacter = Character;
	BoundASC = Cast<UMoonAbilitySystemComponent>(Character->GetAbilitySystemComponent());
	BoundAttributeSet = Character->GetAttributeSet();

	if (BoundASC.IsValid() && BoundAttributeSet.IsValid())
	{
		// Bind to attribute delegates
		BoundASC->GetGameplayAttributeValueChangeDelegate(BoundAttributeSet->GetHealthAttribute()).AddUObject(this, &UMoonCombatHUDWidget::HandleHealthChanged);
		BoundASC->GetGameplayAttributeValueChangeDelegate(BoundAttributeSet->GetManaAttribute()).AddUObject(this, &UMoonCombatHUDWidget::HandleManaChanged);
		BoundASC->GetGameplayAttributeValueChangeDelegate(BoundAttributeSet->GetDashChargesAttribute()).AddUObject(this, &UMoonCombatHUDWidget::HandleDashChargesChanged);
		BoundASC->GetGameplayAttributeValueChangeDelegate(BoundAttributeSet->GetTensionGaugeAttribute()).AddUObject(this, &UMoonCombatHUDWidget::HandleTensionChanged);

		// Initial updates
		OnHealthChanged(BoundAttributeSet->GetHealth(), BoundAttributeSet->GetMaxHealth());
		OnManaChanged(BoundAttributeSet->GetMana(), BoundAttributeSet->GetMaxMana());
		
		float Charges = BoundAttributeSet->GetDashCharges();
		OnDashChargesChanged(FMath::FloorToInt(Charges), Charges - FMath::FloorToFloat(Charges));

		TargetTension = BoundAttributeSet->GetTensionGauge();
		DisplayedTension = TargetTension;

		Character->OnOverdriveStarted.RemoveDynamic(this, &UMoonCombatHUDWidget::HandleOverdriveStarted);
		Character->OnOverdriveEnded.RemoveDynamic(this, &UMoonCombatHUDWidget::HandleOverdriveEnded);
		Character->OnOverdriveStarted.AddDynamic(this, &UMoonCombatHUDWidget::HandleOverdriveStarted);
		Character->OnOverdriveEnded.AddDynamic(this, &UMoonCombatHUDWidget::HandleOverdriveEnded);

		bOverdriveActive = Character->IsOverdriveActive();
		OnOverdriveStateChanged(bOverdriveActive);
		OnOverdriveTimeChanged(Character->GetOverdriveTimeRemaining());
	}
}

void UMoonCombatHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Handle visual lerping for Tension Gauge (GDD Rule 3)
	if (BoundAttributeSet.IsValid())
	{
		float MaxTension = BoundAttributeSet->GetTensionGaugeMax();
		if (MaxTension > 0.0f)
		{
			if (!FMath::IsNearlyEqual(DisplayedTension, TargetTension, 0.1f))
			{
				DisplayedTension = FMath::FInterpTo(DisplayedTension, TargetTension, InDeltaTime, GaugeLerpSpeed);
			}

			// Charged highlight is evaluated on REAL target tension, not displayed (Rule 5)
			bool bIsCharged = (TargetTension / MaxTension) >= TensionChargedHighlightThreshold;
			
			// Basic inference of building vs decaying based on lerp direction
			bool bIsBuilding = TargetTension > DisplayedTension;
			bool bIsDecaying = TargetTension < DisplayedTension;

			OnTensionStateChanged(bIsBuilding, bIsDecaying, bIsCharged, DisplayedTension);
		}
	}

	if (bOverdriveActive && BoundCharacter.IsValid())
	{
		OnOverdriveTimeChanged(BoundCharacter->GetOverdriveTimeRemaining());
	}
}

void UMoonCombatHUDWidget::HandleHealthChanged(const FOnAttributeChangeData& Data)
{
	float MaxHealth = BoundAttributeSet.IsValid() ? BoundAttributeSet->GetMaxHealth() : 100.0f;
	OnHealthChanged(Data.NewValue, MaxHealth);

	// Low Health Warning check
	bool bIsLow = (Data.NewValue / MaxHealth) <= LowHealthWarningThreshold;
	if (bIsLow != bLowHealthActive)
	{
		bLowHealthActive = bIsLow;
		OnLowHealthWarningStateChanged(bLowHealthActive);
	}
}

void UMoonCombatHUDWidget::HandleManaChanged(const FOnAttributeChangeData& Data)
{
	float MaxMana = BoundAttributeSet.IsValid() ? BoundAttributeSet->GetMaxMana() : 100.0f;
	OnManaChanged(Data.NewValue, MaxMana);
}

void UMoonCombatHUDWidget::HandleDashChargesChanged(const FOnAttributeChangeData& Data)
{
	int32 DisplayedCharges = FMath::FloorToInt(Data.NewValue);
	float RechargeFraction = Data.NewValue - (float)DisplayedCharges;
	OnDashChargesChanged(DisplayedCharges, RechargeFraction);
}

void UMoonCombatHUDWidget::HandleTensionChanged(const FOnAttributeChangeData& Data)
{
	TargetTension = Data.NewValue;
}

void UMoonCombatHUDWidget::HandleOverdriveStarted()
{
	bOverdriveActive = true;
	OnOverdriveStateChanged(true);
	OnOverdriveTimeChanged(BoundCharacter.IsValid() ? BoundCharacter->GetOverdriveTimeRemaining() : 0.0f);
}

void UMoonCombatHUDWidget::HandleOverdriveEnded(EMoonOverdriveEndReason Reason)
{
	bOverdriveActive = false;
	OnOverdriveTimeChanged(0.0f);
	OnOverdriveStateChanged(false);
}
