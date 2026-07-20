#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "InputActionValue.h"
#include "GameplayTagContainer.h"
#include "MoonCharacterBase.generated.h"

class UMoonAbilitySystemComponent;
class UMoonAttributeSet;
class UGameplayAbility;
class UGameplayEffect;
class UInputMappingContext;
class UInputAction;

UCLASS()
class MOON_API AMoonCharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AMoonCharacterBase();

	// IAbilitySystemInterface implementation
	virtual class UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	
	UMoonAttributeSet* GetAttributeSet() const { return AttributeSet; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	/** Dash Input */
	void Input_Dash(const FInputActionValue& Value);

	/** Spell Inputs */
	void Input_SpellBlackhole(const FInputActionValue& Value);
	void Input_SpellFire(const FInputActionValue& Value);
	void Input_SpellLightning(const FInputActionValue& Value);

	/** Helper to try activating abilities by a tag */
	void TryActivateAbilityByTag(FGameplayTag AbilityTag);

	// GAS Initialization methods
	virtual void InitializeAbilities();
	virtual void InitializeAttributes();

public:
	/** Checks rate limits (per-frame element cap and global MaxCastsPerSecond) */
	bool CheckAndConsumeSpellCastLimit(FGameplayTag ElementTag);

	// Combo/Tension Gauge Interface
	UFUNCTION(BlueprintCallable, Category = "Moon|Tension")
	void AddTension(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Moon|Tension")
	void AddTensionFromSpellHit(float ManaCost);

	UFUNCTION(BlueprintCallable, Category = "Moon|Tension")
	void AddTensionFromJustDodge();

	UFUNCTION(BlueprintCallable, Category = "Moon|Tension")
	void ApplyTensionDamagePenalty();

	UFUNCTION(BlueprintImplementableEvent, Category = "Moon|Tension")
	void OnOverdriveTriggered();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMoonAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMoonAttributeSet> AttributeSet;

	// Default abilities granted when the character is initialized
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;

	// Default attributes initialization effect
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	TSubclassOf<UGameplayEffect> DefaultAttributesEffect;

	// Tension Tuning Knobs
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Moon|Tension|Tuning")
	float TensionGainCoefficient = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Moon|Tension|Tuning")
	float JustDodgeTensionBonus = 20.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Moon|Tension|Tuning")
	float TensionDecayGracePeriod = 3.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Moon|Tension|Tuning")
	float TensionDecayRatePerSec = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Moon|Tension|Tuning")
	float DamagePenaltyPercent = 0.20f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Moon|Tension|Tuning")
	float OverdriveTensionGainMultiplier = 0.4f;

public:
	// Enhanced Input
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> DashAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> ExecuteAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> SpellBlackholeAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> SpellFireAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> SpellLightningAction;

private:
	// Rate Limiting State
	uint64 LastCastFrame = 0;
	FGameplayTag LastCastElementTag;
	TArray<float> CastTimestamps;

	// Tension State
	float LastTensionGainTime = 0.0f;
};
