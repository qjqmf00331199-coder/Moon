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
class USpringArmComponent;
class UCameraComponent;

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
	virtual void PossessedBy(AController* NewController) override;
	virtual void Landed(const FHitResult& Hit) override;

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

	/** Jump Input (thin logging wrapper around ACharacter::Jump/StopJumping) */
	void Input_Jump();
	void Input_StopJumping();

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

	// Plays a one-shot animation on the mesh (e.g. Dash/spell cast), suppressing the idle/jog
	// locomotion swap in Tick until it finishes. Used by abilities that don't have their own
	// montage/slot system yet (no AnimBlueprint exists for this character). PlayRate scales
	// playback speed (and the auto-resume timer) — e.g. a quick Dash thrust wants this > 1.
	UFUNCTION(BlueprintCallable, Category = "Moon|Animation")
	void PlayOneShotAnim(class UAnimSequence* Anim, float PlayRate = 1.0f);

	// Brief local "hitstop": slows just this character (CustomTimeDilation), not the whole
	// world, for RealDuration seconds — the rest of the game (enemies, projectiles) keeps
	// running normally. Used to sell impact on Dash-end and landing. RealDuration is real time;
	// the restore timer is unaffected by the dilation it's undoing.
	UFUNCTION(BlueprintCallable, Category = "Moon|Animation")
	void TriggerHitStop(float RealDuration, float DilationScale = 0.05f);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;

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

	// Basic locomotion (idle/jog swap by speed). No AnimBlueprint yet — single-node playback switched in code.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	TObjectPtr<class UAnimSequence> IdleAnim;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	TObjectPtr<class UAnimSequence> JogAnim;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	float JogSpeedThreshold = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> SpellFireAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> SpellLightningAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

	// Jump motion: Jump_Start plays once on takeoff, Jump_Apex loops while still airborne
	// afterward, Jump_Land plays once on landing. No AnimBlueprint yet, same single-node
	// playback approach as Idle/Jog above.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	TObjectPtr<class UAnimSequence> JumpStartAnim;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	TObjectPtr<class UAnimSequence> JumpApexAnim;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	TObjectPtr<class UAnimSequence> JumpLandAnim;

	// Plays once after JumpLandAnim finishes, before resuming Idle/Jog — smooths the landing
	// impact pose into locomotion instead of cutting straight from a stiff landing pose.
	// Optional: if unset, locomotion resumes immediately after JumpLandAnim as before.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	TObjectPtr<class UAnimSequence> JumpRecoveryAnim;

	// Jump feel: asymmetric gravity so the descent is snappier than the rise (standard
	// platformer "juice" — e.g. Celeste/Mario) instead of UE's default floaty symmetric arc.
	// Multiplies CharacterMovement's base GravityScale while falling; back to 1x otherwise.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation|Jump Feel")
	float FallingGravityScaleMultiplier = 1.6f;

private:
	// Rate Limiting State
	uint64 LastCastFrame = 0;
	FGameplayTag LastCastElementTag;
	TArray<float> CastTimestamps;

	// Tension State
	float LastTensionGainTime = 0.0f;

	// Locomotion State
	bool bIsPlayingJogAnim = false;

	// Jump/one-shot animation state
	bool bWasFalling = false;
	bool bPlayingOneShotAnim = false;
	FTimerHandle JumpAnimTimerHandle;
	FTimerHandle OneShotAnimTimerHandle;
	FTimerHandle HitStopTimerHandle;

	void RefreshLocomotionAnim();
	void OnJumpStartAnimFinished();
	void OnLandAnimFinished();
	void OnJumpRecoveryAnimFinished();
	void OnOneShotAnimFinished();
	void EndHitStop();
};
