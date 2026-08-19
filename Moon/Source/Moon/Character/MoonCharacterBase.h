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
class UMoonCameraComponent;
class UMoonCameraSettings;
class UMaterialInstanceDynamic;

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

	// Z-axis impulse injection hook (TR-mov-005 / player-movement.md "Interactions with Other
	// Systems" — Dash/Evasion). The safe path for external systems (Dash/Evasion, Arena Morphing —
	// neither has a caller wired up yet) to apply a vertical velocity impulse without reaching into
	// GetCharacterMovement()->Velocity directly. Thin wrapper around LaunchCharacter using the same
	// bXYOverride=false/bZOverride=true pattern Input_Jump() already uses for its coyote-time
	// launch — see that function for the established precedent.
	//
	// SETS Velocity.Z to ZVelocity — this does NOT add to any existing Z velocity (LaunchCharacter's
	// bZOverride=true semantics). Composition with momentum already present on Z is the caller's
	// responsibility; player-movement.md's Dash/Evasion Open Questions section explicitly leaves
	// additive-vs-override impulse composition unresolved pending that epic's own design pass — this
	// hook makes no assumption either way, it only exposes the override primitive.
	//
	// This does not conflict with the control-manifest's "never reintroduce LaunchCharacter... for
	// the dash's horizontal motion" rule (ADR-0007) — that prohibition is scoped to Dash's
	// *horizontal* displacement (which must stay an instant swept SetActorLocation step per
	// ADR-0007). This is the separate Z-axis launch/impulse interface player-movement.md's
	// Interactions section explicitly asks the Movement layer to expose.
	//
	// Deliberately narrow: does not implement Dash's temporary MaxWalkSpeed override (the other half
	// of TR-mov-005) or any generic velocity-injection system — both out of scope for this story
	// (Dash/Evasion epic's job; see Story 002's Out of Scope section).
	//
	// Example usage (future caller, e.g. an air-dash ability):
	//   MoonCharacter->InjectZImpulse(800.0f); // launches straight up at 800uu/s, overriding Velocity.Z
	UFUNCTION(BlueprintCallable, Category = "Moon|Movement")
	void InjectZImpulse(float ZVelocity);

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

	// Story 005 AC-4 provisional benchmark evidence: spawns Count uncontrolled copies of this
	// character's own class in a grid near the player, purely to put Count movement-ticking
	// CharacterMovementComponents into the world so `stat unit`/`stat game` can capture aggregate
	// movement tick cost over ~300 frames. Evidence-only per the story's own edge case ("benchmark
	// is evidence-only until target minimum hardware is final") — not a perf gate, not used
	// anywhere else. Console-usable via Exec while possessing this pawn: `Moon.BenchmarkSpawnMovers 100`.
	UFUNCTION(Exec)
	void Moon_BenchmarkSpawnMovers(int32 Count = 100);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	// Story 005 AC-4: UMoonCameraComponent (not plain UCameraComponent) so the corner-dither state
	// can drive its near-clip-plane override — see MoonCameraComponent.h and
	// UpdateCameraCornerDither() below.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMoonCameraComponent> FollowCamera;

	// Data-driven Tuning Knobs source (ADR-0005 Decision 2 / TR-cam-009). Read once in BeginPlay
	// and applied to CameraBoom/FollowCamera; the constructor's literal SpringArm/Camera setup
	// above is a CDO-preview fallback only and is never re-read once this asset is applied. If
	// unset, BeginPlay leaves the constructor literals in place (null-asset guard).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMoonCameraSettings> CameraSettings;

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
	// Multiplies BaseGravityScale (the TR-mov-004-validated tuning knob) while falling; 1x
	// otherwise — see Tick(). Hard-clamped to the same MinGravityScale (0.1) minimum as
	// BaseGravityScale by ValidateAndClampMovementTuning(), since a zero/negative multiplier here
	// would silently break the descent-gravity feel the same way an unclamped GravityScale would.
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

	// Corner dither state (Story 005 AC-4). CurrentDitherAlpha is the live, continuously-interpolated
	// 0..1 value (see UpdateCameraCornerDither()). MeshDitherMID is created once, lazily, the first
	// time the mesh has a material to spawn a dynamic instance from; stays null (no-op, no crash) if
	// the mesh has no material assigned yet — the art-side Material graph with the matching scalar
	// parameter does not exist as of this story, see DitherFadeParamName's doc comment in the .cpp.
	float CurrentDitherAlpha = 0.0f;
	TObjectPtr<UMaterialInstanceDynamic> MeshDitherMID = nullptr;

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

	// Movement tuning clamp enforcement (TR-mov-004, player-movement.md Tuning Knobs table — these
	// are the GDD's own hard-clamp minimums, not this file's invention). Applied to
	// GetCharacterMovement()'s properties — the actual runtime source of truth today, configured via
	// the BP_MoonCharacter CDO with zero enforcement before this story — by
	// ValidateAndClampMovementTuning(), called once from BeginPlay(). Individual clamps below run
	// BEFORE the AirTime joint bound check (see ComputeAirTime()/ValidateAndClampMovementTuning()) —
	// player-movement.md Formulas section "검증 순서" mandates this order, since running the joint
	// check first would divide by a potentially-unclamped (zero/negative) GravityScale.
	//
	// GravityScale / jump-feel interaction (fixed 2026-07-27, was a KNOWN GAP): the pre-existing
	// asymmetric jump-feel effect in Tick() used to overwrite GetCharacterMovement()->GravityScale
	// outright every frame (MoveComp->GravityScale = bDescending ? FallingGravityScaleMultiplier :
	// 1.0f), silently discarding whatever base GravityScale this validation had just clamped/
	// joint-bound-checked — its own doc comment said "multiplies" but the code assigned, not
	// multiplied. Tick() now computes MoveComp->GravityScale = BaseGravityScale * (bDescending ?
	// FallingGravityScaleMultiplier : 1.0f), so the validated base value is never discarded, and
	// FallingGravityScaleMultiplier itself is now clamped to MinGravityScale below. BaseGravityScale
	// (declared near LastValidGravityScale below) is the actual source of truth for the AirTime
	// formula/joint-bound/revert-policy from here on — GetCharacterMovement()->GravityScale is a
	// derived, per-tick value once Tick() has run at least once.
	static constexpr float MinMaxWalkSpeed = 100.0f;
	static constexpr float MinJumpZVelocity = 100.0f;
	static constexpr float MinGravityScale = 0.1f;
	static constexpr float MinMaxAcceleration = 1000.0f;
	static constexpr float MinBrakingDecelerationWalking = 1000.0f;
	static constexpr float MinGroundFriction = 1.0f;

	// AirTime joint bound (TR-mov-004): individually-clamped JumpZVelocity/GravityScale can still
	// combine into a physically unplayable AirTime (GDD Formulas section, Jump Air Time). Declared as
	// a closed [Min, Max] range in seconds, checked AFTER the individual clamps above.
	static constexpr float AirTimeJointBoundMinSeconds = 0.5f;
	static constexpr float AirTimeJointBoundMaxSeconds = 3.0f;

	// UE's documented default world gravity Z magnitude (-980uu/s^2, unscaled) — used only as a
	// defensive fallback if GetWorld() is somehow null when ComputeAirTime() runs (should not happen
	// from BeginPlay, but avoids an unguarded divide against an uninitialized value).
	static constexpr float DefaultWorldGravityZ = -980.0f;

	// GDD-documented safe fallback defaults (player-movement.md Tuning Knobs table "Current Value"
	// column) — used ONLY when BeginPlay's very first validation pass already fails the joint bound
	// and there is no previous valid pair to revert to yet (bootstrap case, see
	// ValidateAndClampMovementTuning()). Every subsequent rejection reverts to
	// LastValidJumpZVelocity/LastValidGravityScale instead of these.
	static constexpr float DefaultJumpZVelocity = 600.0f;
	static constexpr float DefaultGravityScale = 1.0f;

	// Last known-valid (JumpZVelocity, GravityScale) pair (TR-mov-004 revert policy). Set once
	// BeginPlay's validation pass confirms a joint-bound-valid combination exists (either the
	// as-configured values, or — bootstrap-only — the GDD defaults above). A future runtime setter
	// that changes either value should call ValidateAndClampMovementTuning() again afterward rather
	// than duplicating this revert logic.
	bool bHasValidatedMovementTuning = false;
	float LastValidJumpZVelocity = DefaultJumpZVelocity;
	float LastValidGravityScale = DefaultGravityScale;

	// The TR-mov-004-validated GravityScale value — the actual source of truth for the AirTime
	// formula/joint-bound/revert-policy, and what Tick()'s asymmetric jump-feel multiply reads as
	// its base. Kept distinct from GetCharacterMovement()->GravityScale because that CMC property
	// becomes a derived, per-tick value (BaseGravityScale * feel multiplier) once Tick() has run —
	// reading it back as if it were still the configured base would corrupt this validation the
	// same way naively re-capturing a transform mid-freeze corrupted Story 004's hitstop rewrite.
	float BaseGravityScale = DefaultGravityScale;

	// Computes AirTime per the GDD Formulas section EXACTLY: AirTime = (2 * JumpZVelocity) /
	// (GravityScale * abs(WorldGravityZ)). WorldGravityZ comes from GetWorld()->GetGravityZ() (the
	// world's UNSCALED gravity, e.g. -980) — NOT GetCharacterMovement()->GetGravityZ(), which already
	// has GravityScale multiplied in and would square GravityScale if reused here (GDD Formulas
	// section WorldGravityZ warning).
	float ComputeAirTime(float JumpZVelocity, float GravityScale) const;

	// Applies the six individual hard clamps, then validates the AirTime joint bound and applies the
	// revert-to-last-known-valid-pair policy (TR-mov-004). Called once from BeginPlay() today;
	// structured to take no parameters (reads/writes GetCharacterMovement() directly) so a future
	// runtime setter can call it again after changing GravityScale/JumpZVelocity instead of
	// duplicating this validation.
	void ValidateAndClampMovementTuning();

	// Tick() sub-steps (split out of the former ~150-line monolith for testability/readability —
	// each owns exactly one of Tick()'s per-frame concerns; call order in Tick() is unchanged).
	void UpdateJumpTimers(float DeltaTime);
	void UpdateResourceRegen(float DeltaTime, double CurrentWorldTime);
	void UpdateJumpFeelGravity();
	void UpdateJumpAnimState(float DeltaTime);

	// Story 005 AC-4 (TR-cam-005, GDD Edge Case 1 — corner dithering): measures the SpringArm's
	// actual collided pivot-to-socket distance every frame, fades the mesh's dither material
	// parameter and the camera's near-clip override toward the corner-dithered state below
	// CornerDitherThreshold (and back out above it), continuously via FInterpTo rather than a hard
	// on/off toggle (the GDD edge case explicitly forbids a stuck-transparent/flicker outcome).
	void UpdateCameraCornerDither(float DeltaTime);

protected:
	// Pure/stateless target-alpha step (Story 005 AC-4): 1.0 at or below Threshold, 0.0 above it.
	// Deliberately time-independent so it's unit-testable without a Tick/physics scene — the
	// continuous fade (FInterpTo against DeltaTime) lives in UpdateCameraCornerDither() above, which
	// calls this every frame for its target. Protected (not private) so the test-only accessor
	// pattern (see MoonCameraCollisionGuardrailsTests.cpp) can expose it via `using` — a derived
	// class cannot re-expose a truly private base member, only protected/public ones.
	static float ComputeCornerDitherTargetAlpha(float ActualArmLength, float Threshold);

	// Applies CameraSettings' fields to CameraBoom/FollowCamera (ADR-0005 Decision 2 / Story 001
	// AC-5). Called once from BeginPlay(). No-op (constructor literals stand as the CDO-preview
	// fallback) if CameraSettings is unset — see the null-asset guard note on the property above.
	// Protected so automation accessors can exercise the application path without violating
	// AActor's BeginPlay lifecycle invariants on transient test objects.
	void ApplyCameraSettings();

private:
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
