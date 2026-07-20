#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MoonCombatHUDWidget.generated.h"

class UMoonAbilitySystemComponent;
class UMoonAttributeSet;

/**
 * Combat HUD base class. 
 * Owns the data binding to Health, Mana, Dash, Tension, and Overdrive states.
 * 100% Read-only, Event-driven (Tick only for visual lerp/sweeps).
 */
UCLASS(Abstract)
class MOON_API UMoonCombatHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Setup bindings to the player's GAS components
	UFUNCTION(BlueprintCallable, Category = "Moon|HUD")
	void BindToPlayer(APawn* PlayerPawn);

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// Event hooks for Blueprint to drive animations/visuals

	// W1: Health
	UFUNCTION(BlueprintImplementableEvent, Category = "Moon|HUD|Health")
	void OnHealthChanged(float CurrentHealth, float MaxHealth);

	// W2: Low Health Warning
	UFUNCTION(BlueprintImplementableEvent, Category = "Moon|HUD|Health")
	void OnLowHealthWarningStateChanged(bool bIsLowHealth);

	// W3: Mana
	UFUNCTION(BlueprintImplementableEvent, Category = "Moon|HUD|Mana")
	void OnManaChanged(float CurrentMana, float MaxMana);

	// W5: Dash Charges
	UFUNCTION(BlueprintImplementableEvent, Category = "Moon|HUD|Dash")
	void OnDashChargesChanged(int32 DisplayedCharges, float RechargeFraction);

	// W6: Tension Gauge
	UFUNCTION(BlueprintImplementableEvent, Category = "Moon|HUD|Tension")
	void OnTensionStateChanged(bool bIsBuilding, bool bIsDecaying, bool bIsCharged, float DisplayedTensionValue);

	// W7: Overdrive
	UFUNCTION(BlueprintImplementableEvent, Category = "Moon|HUD|Overdrive")
	void OnOverdriveStateChanged(bool bIsActive);

	// W7: Execution Prompt
	UFUNCTION(BlueprintImplementableEvent, Category = "Moon|HUD|Execution")
	void OnExecutionPromptChanged(bool bIsVisible);

	// The player's attribute set
	TWeakObjectPtr<UMoonAttributeSet> BoundAttributeSet;
	TWeakObjectPtr<UMoonAbilitySystemComponent> BoundASC;

	// Tuning Knobs
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Moon|HUD|Tuning")
	float LowHealthWarningThreshold = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Moon|HUD|Tuning")
	float TensionChargedHighlightThreshold = 0.90f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Moon|HUD|Tuning")
	float GaugeLerpSpeed = 10.0f;

	// Internal state
	float DisplayedTension = 0.0f;
	float TargetTension = 0.0f;
	bool bOverdriveActive = false;
	bool bLowHealthActive = false;

private:
	// Attribute Change Callbacks
	void HandleHealthChanged(const struct FOnAttributeChangeData& Data);
	void HandleManaChanged(const struct FOnAttributeChangeData& Data);
	void HandleDashChargesChanged(const struct FOnAttributeChangeData& Data);
	void HandleTensionChanged(const struct FOnAttributeChangeData& Data);
};
