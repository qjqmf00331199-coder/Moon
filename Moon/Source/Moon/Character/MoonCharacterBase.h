#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "InputActionValue.h"
#include "GameplayTagContainer.h"
#include "../GAS/MoonOverdriveState.h"
#include "MoonCharacterBase.generated.h"

class UMoonAbilitySystemComponent;
class UMoonAttributeSet;
class UGameplayAbility;
class UGameplayEffect;
class UInputMappingContext;
class UInputAction;
class USpringArmComponent;
class UCameraComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMoonOverdriveStartedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMoonOverdriveEndedSignature, EMoonOverdriveEndReason, Reason);

// Airborne substate (TR-mov-003): a derived read-only value, never a stored state machine and
// never a custom CMC movement mode. Both values map to the single native MOVE_Falling mode;
// AMoonCharacterBase::Tick() derives this purely from Velocity.Z's sign after Super::Tick() has
// run each frame. See ADR-0009 Decision 2 / Alternative 3 (explicitly rejects
// SetMovementModeWithCustomMode() for this purpose).
UENUM(BlueprintType)
enum class EMoonAirborneSubState : uint8
{
	Ascending,
	Falling
};

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
	void Input_Execute(const FInputActionValue& Value);

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

	UFUNCTION(BlueprintImplementableEvent, Category = "Moon|Tension", meta = (DeprecatedFunction, DeprecationMessage = "Use TriggerOverdrive and OnOverdriveStarted instead."))
	void OnOverdriveTriggered();

	// Luna Overdrive fixed-window state. Trigger calls received during Active or Recovery are ignored.
	UFUNCTION(BlueprintCallable, Category = "Moon|Overdrive")
	bool TriggerOverdrive();

	UFUNCTION(BlueprintCallable, Category = "Moon|Overdrive")
	void ForceEndOverdrive(EMoonOverdriveEndReason Reason);

	UFUNCTION(BlueprintPure, Category = "Moon|Overdrive")
	bool IsOverdriveActive() const;

	UFUNCTION(BlueprintPure, Category = "Moon|Overdrive")
	bool IsTensionGainLocked() const;

	UFUNCTION(BlueprintPure, Category = "Moon|Overdrive")
	float GetOverdriveTimeRemaining() const;

	UPROPERTY(BlueprintAssignable, Category = "Moon|Overdrive")
	FMoonOverdriveStartedSignature OnOverdriveStarted;

	UPROPERTY(BlueprintAssignable, Category = "Moon|Overdrive")
	FMoonOverdriveEndedSignature OnOverdriveEnded;

	// Derived read-only airborne substate (TR-mov-003) — see EMoonAirborneSubState above.
	// Updated once per frame in Tick(), after Super::Tick() has run.
	UFUNCTION(BlueprintPure, Category = "Moon|Movement")
	EMoonAirborneSubState GetAirborneSubState() const { return AirborneSubState; }

	// Movement lock read-only query (TR-mov-006). Write access is a private reservation for the
	// not-yet-designed Status Effect system — see SetMovementLocked() below.
	UFUNCTION(BlueprintPure, Category = "Moon|Movement")
	bool IsMovementLocked() const { return bMovementLocked; }

	// Plays a one-shot animation on the mesh (e.g. Dash/spell cast), suppressing the idle/jog
	// locomotion swap in Tick until it finishes. Used by abilities that don't have their own
	// montage/slot system yet (no AnimBlueprint exists for this character). PlayRate scales
	// playback speed (and the auto-resume timer) — e.g. a quick Dash thrust wants this > 1.
	UFUNCTION(BlueprintCallable, Category = "Moon|Animation")
	void PlayOneShotAnim(class UAnimSequence* Anim, float PlayRate = 1.0f);

	// Presentation-only "hitstop" (TR-mov-008 / ADR-0009 Decision 5 / player-movement.md Core
	// Rule 9): briefly freezes the mesh's VISUAL presentation only (world transform + anim
	// playback) for RealDuration seconds, then blends it back to its actual position over a short
	// window. The Capsule and CharacterMovementComponent are never touched — gameplay position
	// keeps advancing at 100% normal tick rate throughout. No form of Time Dilation (actor-level
	// or global) is used anywhere in this path. Used to sell impact on Dash-end and landing.
	UFUNCTION(BlueprintCallable, Category = "Moon|Animation")
	void TriggerHitStop(float RealDuration);

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Moon|Signature Chain|Spike", meta = (ClampMin = "100.0"))
	float ExecutionRange = 350.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Moon|Overdrive|Tuning", meta = (ClampMin = "6.0", ClampMax = "15.0"))
	float OverdriveDuration = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Moon|Overdrive|Tuning", meta = (ClampMin = "1.0", ClampMax = "2.0"))
	float OverdriveRecoveryDuration = 1.5f;

	// Hitstop presentation tuning (TR-mov-008): capture-and-blend, no form of Time Dilation.
	// See TriggerHitStop()/EndHitStop()/UpdateHitStopPresentation().
	// Blend-out interpolation constant (1/seconds) for the post-freeze unfreeze correction —
	// higher is snappier. This alone cannot guarantee convergence (the blend target keeps moving
	// while the Capsule keeps moving), so HitStopBlendOutDuration below is the actual hard stop.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Moon|Animation|Hitstop Tuning", meta = (ClampMin = "5.0"))
	float HitStopBlendOutInterpSpeed = 40.0f;

	// Hard cap (seconds) on the unfreeze blend — "1-2 frames" per the ADR/GDD. Needed because
	// interpolating toward a still-moving target (the mesh's natural, Capsule-following transform)
	// never reaches exact convergence on its own; this is what actually ends the blend.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Moon|Animation|Hitstop Tuning", meta = (ClampMin = "0.008"))
	float HitStopBlendOutDuration = 0.033f;

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
	FMoonOverdriveState OverdriveState;

	// Locomotion State
	bool bIsPlayingJogAnim = false;

	// Jump/one-shot animation state
	bool bWasFalling = false;
	bool bPlayingOneShotAnim = false;
	FTimerHandle JumpAnimTimerHandle;
	FTimerHandle OneShotAnimTimerHandle;
	FTimerHandle HitStopTimerHandle;

	// Hitstop presentation freeze (TR-mov-008 / ADR-0009 Decision 5): capture-and-blend, never
	// Time Dilation. Capsule/CMC keep ticking at 100% normal rate the entire time; only the mesh's
	// visual presentation freezes then blends back. See TriggerHitStop()/EndHitStop() and
	// UpdateHitStopPresentation() (called from Tick()).
	enum class EMoonHitStopPhase : uint8
	{
		Inactive,
		Freezing,
		BlendingOut
	};
	EMoonHitStopPhase HitStopPhase = EMoonHitStopPhase::Inactive;

	// World-space mesh transform captured the instant the freeze began — held every tick during
	// Freezing, and the blend-out start point during BlendingOut.
	FTransform FreezeStartMeshTransform;

	// The mesh's RELATIVE transform (to its attach parent, the Capsule) at the last moment it was
	// in a fully natural, non-frozen state. SetWorldTransform() on an attached component silently
	// rewrites its stored relative transform every time it's called, so repeatedly forcing a
	// world-space freeze would otherwise permanently corrupt this offset once the freeze ends.
	// Restoring this exact value when the blend finishes guarantees zero residual drift, even
	// across repeated/re-triggered hitstops (see the Inactive-only recapture guard in
	// TriggerHitStop()).
	FTransform CapturedMeshRelativeTransform;

	// Current held/blending world transform, updated every tick while HitStopPhase != Inactive.
	FTransform HitStopBlendCurrentTransform;

	// Elapsed real time (seconds) since BlendingOut started. The blend target (the mesh's natural,
	// Capsule-following transform) keeps moving every tick, so interpolation toward it alone never
	// converges to zero distance (exponential decay toward a moving target settles at a nonzero
	// steady-state lag) — this elapsed-time cap against HitStopBlendOutDuration is what guarantees
	// the blend actually finishes.
	float HitStopBlendElapsed = 0.0f;

	// Airborne substate (TR-mov-003): derived once per frame in Tick(), after Super::Tick(), from
	// GetCharacterMovement()->Velocity.Z's sign. Not a stored transition table.
	EMoonAirborneSubState AirborneSubState = EMoonAirborneSubState::Falling;

	// MovementLocked (TR-mov-006): private access-control reservation. The Status Effect system
	// (not yet designed) is the sole intended future caller of SetMovementLocked() — see
	// ADR-0009 Decision 3. Do not call this from anywhere until that system's ADR grants access;
	// Spell Casting must never gain write access, even accidentally.
	bool bMovementLocked = false;
	void SetMovementLocked(bool bLocked);

	// Jump input buffer / coyote time (TR-mov-007): Character-owned float accumulators, seconds,
	// REMAINING time until the grace window closes — armed at JumpGraceWindowSeconds (0.150f) and
	// decremented via -= DeltaTime in Tick() (never a fixed frame count). Consumption uses an
	// inclusive boundary against JumpGraceWindowSeconds (149ms/150ms elapsed pass, 150.5ms/151ms
	// elapsed fail) — see ADR-0009 Decision 4.
	//
	// UnarmedTimerSentinel (a negative value, never reachable by a real countdown from 0.150f
	// down to 0) is the "not armed" default/reset value. This is deliberately NOT 0.0f: a timer
	// armed at 0.150f and decremented for exactly 150ms elapsed lands on remaining == 0.0f, which
	// must PASS the grace-window check (149ms/150ms are both required to pass, inclusive) — so
	// 0.0f cannot double as the "never armed" sentinel without incorrectly failing that exact
	// boundary case.
	static constexpr float JumpGraceWindowSeconds = 0.150f;
	static constexpr float UnarmedTimerSentinel = -1.0f;
	float JumpInputBufferTimer = UnarmedTimerSentinel;
	float CoyoteTimeTimer = UnarmedTimerSentinel;

	// True while a timer's REMAINING value is still within its inclusive grace window:
	// [0.0f, JumpGraceWindowSeconds]. The >= 0.0f lower bound is what makes the exact
	// 150ms-elapsed case (remaining == 0.0f) pass; it also correctly excludes the negative
	// UnarmedTimerSentinel default and any post-expiry negative remaining value. The
	// <= JumpGraceWindowSeconds upper bound is the ADR-mandated boundary operator (not a bare
	// > 0.f check) so that any value beyond the 150ms window — however it got there — is rejected.
	static bool IsWithinGraceWindow(float TimerSeconds) { return TimerSeconds >= 0.0f && TimerSeconds <= JumpGraceWindowSeconds; }

	void RefreshLocomotionAnim();
	void OnJumpStartAnimFinished();
	void OnLandAnimFinished();
	void OnJumpRecoveryAnimFinished();
	void OnOneShotAnimFinished();
	void EndHitStop();

	// Per-tick presentation step for the active hitstop phase (Freezing: force the mesh back to
	// FreezeStartMeshTransform; BlendingOut: InterpTo the mesh from its held transform toward its
	// natural, Capsule-following transform). No-op while Inactive. Called once per Tick(), after
	// Super::Tick() and the existing locomotion/anim logic (TR-mov-008).
	void UpdateHitStopPresentation(float DeltaTime);

	void UpdateOverdriveState(double CurrentTime);
};
