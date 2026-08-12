#include "MoonCharacterBase.h"
#include "TargetDummy.h"
#include "../GAS/MoonAbilitySystemComponent.h"
#include "../GAS/MoonAttributeSet.h"
#include "GameplayAbilitySpec.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "TimerManager.h"
#include "EngineUtils.h"
// Movement Insights trace scopes (TR-mov-009 / ADR-0009 Decision 6). Header lives in Core/Public
// (Core is already a PublicDependencyModuleName in Moon.Build.cs), so no Build.cs change is
// needed. Verified against this project's actual UE5.8 install
// (Engine/Source/Runtime/Core/Public/ProfilingDebugging/CpuProfilerTrace.h) — not assumed.
#include "ProfilingDebugging/CpuProfilerTrace.h"

AMoonCharacterBase::AMoonCharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;

	// Third-person follow camera. Boom handles collision so the camera never clips into the level.
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	// Combat camera: a light right-shoulder composition keeps the player readable while
	// opening enough space to see incoming threats. Position lag softens instant dash steps
	// without adding rotation latency to aiming.
	CameraBoom->TargetArmLength = 450.0f;
	CameraBoom->SocketOffset = FVector(0.0f, 45.0f, 20.0f);
	CameraBoom->TargetOffset = FVector(0.0f, 0.0f, 55.0f);
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 18.0f;
	CameraBoom->CameraLagMaxDistance = 60.0f;
	CameraBoom->bUseCameraLagSubstepping = true;
	CameraBoom->CameraLagMaxTimeStep = 1.0f / 60.0f;
	CameraBoom->SetRelativeRotation(FRotator(-15.0f, 0.0f, 0.0f));
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Movement/Camera contract: facing snaps to controller yaw while movement remains camera-relative.
	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;

	// Create ASC and AttributeSet. Since this is a solo PC game, Avatar = Owner.
	AbilitySystemComponent = CreateDefaultSubobject<UMoonAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(false);

	AttributeSet = CreateDefaultSubobject<UMoonAttributeSet>(TEXT("AttributeSet"));
}

UAbilitySystemComponent* AMoonCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AMoonCharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Movement Insights trace (TR-mov-009 / ADR-0009 Decision 6): marks the first post-CMC-tick
	// point this frame where Velocity is available at its updated value (Super::Tick() above has
	// already run CharacterMovementComponent::TickComponent). Scoped narrowly around the substate
	// read below rather than the rest of Tick() — later per-frame work in this function (mana
	// regen, tension decay, hitstop presentation, locomotion swap) is unrelated to movement and
	// should not be folded into this scope's measured duration. Name kept exactly as ADR-0009
	// specifies: "MovementInputTrace.VelocityUpdated".
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(MovementInputTrace.VelocityUpdated);

		// Airborne substate (TR-mov-003): derived purely from Velocity.Z's sign, sampled here
		// because Super::Tick() above has already run CMC's own movement tick this frame — reading
		// Velocity.Z before Super::Tick() would see last frame's stale value. Ascending/Falling both
		// map to the single native MOVE_Falling mode; there is no stored transition table and no
		// custom movement mode (ADR-0009 Decision 2). External Z-impulse re-entry (Dash/Launch
		// pushing Velocity.Z positive while Falling) and ceiling-bump deceleration back to Falling
		// both fall out of this one-line sign check with no special-casing.
		if (const UCharacterMovementComponent* MoveCompForSubState = GetCharacterMovement())
		{
			AirborneSubState = (MoveCompForSubState->Velocity.Z > 0.0f) ? EMoonAirborneSubState::Ascending : EMoonAirborneSubState::Falling;
		}
	}

	UpdateJumpTimers(DeltaTime);

	const double CurrentWorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	UpdateOverdriveState(CurrentWorldTime);
	UpdateResourceRegen(DeltaTime, CurrentWorldTime);
	UpdateJumpFeelGravity();
	UpdateJumpAnimState(DeltaTime);

	// Basic locomotion: swap the single-node playing animation between idle and jog by speed.
	// No AnimBlueprint/blendspace yet, so this is a hard switch rather than a blend.
	// Suppressed while a one-shot anim (jump start/land, dash, spell cast) controls the mesh.
	if (!bPlayingOneShotAnim)
	{
		if (USkeletalMeshComponent* MeshComp = GetMesh())
		{
			const float Speed = GetVelocity().Size();
			const bool bShouldJog = Speed > JogSpeedThreshold;
			if (bShouldJog && !bIsPlayingJogAnim && JogAnim)
			{
				MeshComp->PlayAnimation(JogAnim, true);
				bIsPlayingJogAnim = true;
			}
			else if (!bShouldJog && bIsPlayingJogAnim && IdleAnim)
			{
				MeshComp->PlayAnimation(IdleAnim, true);
				bIsPlayingJogAnim = false;
			}
		}
	}

	// Hitstop presentation freeze (TR-mov-008 / ADR-0009 Decision 5): mesh-only, no Time
	// Dilation. Placed last, matching the ADR's Tick() ordering — Capsule/CMC have already moved
	// normally via Super::Tick() above, and this step is the one that overrides the mesh's visual
	// presentation on top of that, purely for presentation.
	UpdateHitStopPresentation(DeltaTime);
}

void AMoonCharacterBase::UpdateJumpTimers(float DeltaTime)
{
	// Jump input buffer / coyote time (TR-mov-007): plain delta-time countdowns, never a fixed
	// frame count. Each stops decrementing once armed reaches (or crosses) zero elapsed-time —
	// clamped at UnarmedTimerSentinel so an expired/unarmed timer settles at a single well-known
	// value instead of drifting to an arbitrary negative number.
	if (JumpInputBufferTimer > 0.0f)
	{
		JumpInputBufferTimer = FMath::Max(JumpInputBufferTimer - DeltaTime, UnarmedTimerSentinel);
	}
	if (CoyoteTimeTimer > 0.0f)
	{
		CoyoteTimeTimer = FMath::Max(CoyoteTimeTimer - DeltaTime, UnarmedTimerSentinel);
	}
}

void AMoonCharacterBase::UpdateResourceRegen(float DeltaTime, double CurrentWorldTime)
{
	if (!AttributeSet)
	{
		return;
	}

	// Mana Regen pauses during the free-cast Overdrive window. Recovery restores normal
	// regeneration immediately while keeping tension locked for its short downbeat.
	float CurrentMana = AttributeSet->GetMana();
	float MaxMana = AttributeSet->GetMaxMana();
	float ManaRegen = AttributeSet->GetManaRegenRate();
	if (!OverdriveState.IsActive(CurrentWorldTime) && CurrentMana < MaxMana && ManaRegen > 0.0f)
	{
		float NewMana = FMath::Clamp(CurrentMana + (ManaRegen * DeltaTime), 0.0f, MaxMana);
		AttributeSet->SetMana(NewMana);
	}

	// Dash Charge Regen
	float CurrentCharges = AttributeSet->GetDashCharges();
	float MaxCharges = AttributeSet->GetMaxDashCharges();
	float DashRechargeRate = AttributeSet->GetDashRechargeRate();
	if (CurrentCharges < MaxCharges && DashRechargeRate > 0.0f)
	{
		float NewCharges = FMath::Clamp(CurrentCharges + (DeltaTime / DashRechargeRate), 0.0f, MaxCharges);
		AttributeSet->SetDashCharges(NewCharges);
	}

	// Tension Decay (Rule 3)
	float CurrentTension = AttributeSet->GetTensionGauge();
	if (CurrentTension > 0.0f)
	{
		float CurrentTime = GetWorld()->GetTimeSeconds();
		if (CurrentTime - LastTensionGainTime > TensionDecayGracePeriod)
		{
			float NewTension = FMath::Max(0.0f, CurrentTension - (TensionDecayRatePerSec * DeltaTime));
			AttributeSet->SetTensionGauge(NewTension);
		}
	}
}

void AMoonCharacterBase::UpdateJumpFeelGravity()
{
	// Jump feel: fall faster than we rise (asymmetric gravity) for a snappier arc instead of
	// UE's default floaty symmetric one. Multiplies BaseGravityScale (the TR-mov-004-validated
	// tuning knob, see ValidateAndClampMovementTuning()) rather than overwriting it outright —
	// the previous flat-assignment version silently discarded the validated base value every
	// frame, undermining the AirTime joint-bound guarantee for the entire falling phase.
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		const bool bDescending = MoveComp->IsFalling() && GetVelocity().Z < 0.0f;
		MoveComp->GravityScale = BaseGravityScale * (bDescending ? FallingGravityScaleMultiplier : 1.0f);
	}
}

void AMoonCharacterBase::UpdateJumpAnimState(float DeltaTime)
{
	// Jump motion: detect the moment we start falling (jump or walking off a ledge) and play
	// Jump_Start once. OnJumpStartAnimFinished() hands off to a looping Jump_Apex if still
	// airborne once Jump_Start finishes; Landed() plays Jump_Land on touchdown.
	const bool bIsFalling = GetCharacterMovement()->IsFalling();
	const bool bJustStartedFalling = bIsFalling && !bWasFalling;

	// Coyote time (TR-mov-007): only armed when the character left the ground without jumping
	// (JumpCurrentCount still 0 this frame — walked off a ledge, was pushed, etc.). An explicit
	// jump already increments JumpCurrentCount by the time CMC's IsFalling() flips true, so a
	// normal jump-triggered fall does not re-arm this timer.
	if (bJustStartedFalling && JumpCurrentCount == 0)
	{
		CoyoteTimeTimer = JumpGraceWindowSeconds;
	}

	if (bJustStartedFalling && JumpStartAnim)
	{
		if (USkeletalMeshComponent* MeshComp = GetMesh())
		{
			bPlayingOneShotAnim = true;
			MeshComp->PlayAnimation(JumpStartAnim, false);
			GetWorld()->GetTimerManager().SetTimer(JumpAnimTimerHandle, this, &AMoonCharacterBase::OnJumpStartAnimFinished, JumpStartAnim->GetPlayLength(), false);
		}
	}
	bWasFalling = bIsFalling;
}

void AMoonCharacterBase::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	// Jump input buffer (TR-mov-007): a jump pressed shortly before touchdown is honored here.
	// By this point the character is grounded again, so the normal Jump() path (native CanJump())
	// succeeds without any special-casing.
	if (IsWithinGraceWindow(JumpInputBufferTimer))
	{
		// Reset to the unarmed sentinel, not 0.0f — 0.0f is itself a valid "exactly 150ms
		// elapsed" passing value, so resetting to it here would let this same buffered jump be
		// consumed again on a later frame before it is next armed.
		JumpInputBufferTimer = UnarmedTimerSentinel;
		Jump();
	}

	if (JumpLandAnim)
	{
		if (USkeletalMeshComponent* MeshComp = GetMesh())
		{
			bPlayingOneShotAnim = true;
			MeshComp->PlayAnimation(JumpLandAnim, false);
			GetWorld()->GetTimerManager().SetTimer(JumpAnimTimerHandle, this, &AMoonCharacterBase::OnLandAnimFinished, JumpLandAnim->GetPlayLength(), false);
		}
	}
	else
	{
		bPlayingOneShotAnim = false;
		RefreshLocomotionAnim();
	}

	// Landing impact hitstop (TR-mov-008 / ADR-0009 Decision 5): triggered AFTER the landing pose
	// is set above (moved from before it — see the Story 004 evidence doc) so the pose the mesh's
	// presentation freezes on is the landing impact pose itself, not the pre-landing airborne
	// pose. Capsule/CMC are never touched and keep moving at 100% normal tick rate for the whole
	// freeze window; see TriggerHitStop() below for the capture-and-blend mechanism.
	TriggerHitStop(0.055f);
}

void AMoonCharacterBase::RefreshLocomotionAnim()
{
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp) return;

	const float Speed = GetVelocity().Size();
	const bool bShouldJog = Speed > JogSpeedThreshold;
	if (bShouldJog && JogAnim)
	{
		MeshComp->PlayAnimation(JogAnim, true);
		bIsPlayingJogAnim = true;
	}
	else if (IdleAnim)
	{
		MeshComp->PlayAnimation(IdleAnim, true);
		bIsPlayingJogAnim = false;
	}
}

void AMoonCharacterBase::OnJumpStartAnimFinished()
{
	if (GetCharacterMovement()->IsFalling() && JumpApexAnim)
	{
		if (USkeletalMeshComponent* MeshComp = GetMesh())
		{
			// Still airborne: hold Jump_Apex until Landed() takes over. Not looped — Jump_Apex
			// is a short clip (~0.2s) with no matching first/last frame, so looping it visibly
			// popped/twitched at the seam on every repeat. Playing once and freezing on its
			// last frame reads as a clean held pose instead.
			MeshComp->PlayAnimation(JumpApexAnim, false);
		}
	}
	else
	{
		// Already landed while Jump_Start was still playing (short hop) — resume locomotion now.
		bPlayingOneShotAnim = false;
		RefreshLocomotionAnim();
	}
}

void AMoonCharacterBase::OnLandAnimFinished()
{
	if (JumpRecoveryAnim)
	{
		if (USkeletalMeshComponent* MeshComp = GetMesh())
		{
			// Still suppressing idle/jog — Jump_Recovery bridges the landing pose into locomotion.
			MeshComp->PlayAnimation(JumpRecoveryAnim, false);
			GetWorld()->GetTimerManager().SetTimer(JumpAnimTimerHandle, this, &AMoonCharacterBase::OnJumpRecoveryAnimFinished, JumpRecoveryAnim->GetPlayLength(), false);
		}
	}
	else
	{
		bPlayingOneShotAnim = false;
		RefreshLocomotionAnim();
	}
}

void AMoonCharacterBase::OnJumpRecoveryAnimFinished()
{
	bPlayingOneShotAnim = false;
	RefreshLocomotionAnim();
}

void AMoonCharacterBase::PlayOneShotAnim(UAnimSequence* Anim, float PlayRate)
{
	if (!Anim) return;

	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp) return;

	bPlayingOneShotAnim = true;
	MeshComp->PlayAnimation(Anim, false);

	if (!FMath::IsNearlyEqual(PlayRate, 1.0f))
	{
		if (UAnimSingleNodeInstance* SingleNode = MeshComp->GetSingleNodeInstance())
		{
			SingleNode->SetPlayRate(PlayRate);
		}
	}

	const float Duration = Anim->GetPlayLength() / FMath::Max(PlayRate, KINDA_SMALL_NUMBER);
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(OneShotAnimTimerHandle, this, &AMoonCharacterBase::OnOneShotAnimFinished, Duration, false);
	}
}

void AMoonCharacterBase::OnOneShotAnimFinished()
{
	bPlayingOneShotAnim = false;
	RefreshLocomotionAnim();
}

void AMoonCharacterBase::TriggerHitStop(float RealDuration)
{
	if (RealDuration <= 0.0f)
	{
		EndHitStop();
		return;
	}

	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		// Capture-and-blend presentation freeze (TR-mov-008 / ADR-0009 Decision 5): the Capsule
		// and CharacterMovementComponent are never touched here and keep ticking at 100% normal
		// rate for the whole freeze window — only the mesh's visual presentation freezes. No form
		// of Time Dilation (actor-level or global) is used anywhere in this path.
		FreezeStartMeshTransform = MeshComp->GetComponentTransform();
		HitStopBlendCurrentTransform = FreezeStartMeshTransform;

		// Only (re)capture the true original relative offset when starting fresh from a fully
		// natural, non-frozen state. If a hitstop is already active (this call is a re-trigger —
		// e.g. dash-cancelling mid-freeze), the mesh's CURRENT relative transform has already been
		// overwritten by this system's own SetWorldTransform() calls below; capturing it here
		// would bake that transient value in as "the original" and permanently corrupt the
		// eventual restore.
		if (HitStopPhase == EMoonHitStopPhase::Inactive)
		{
			CapturedMeshRelativeTransform = MeshComp->GetRelativeTransform();
		}

		// Hold the mesh's anim playback position for the freeze duration. bPauseAnims confirmed
		// present and correct against the installed UE5.8 engine headers (SkeletalMeshComponent.h)
		// via /code-review engine-specialist verification, 2026-08-12 — no longer a hallucination
		// risk. Pauses anim tick/playback advance without disabling bone refresh.
		MeshComp->bPauseAnims = true;
	}

	HitStopPhase = EMoonHitStopPhase::Freezing;
	HitStopBlendElapsed = 0.0f;

	if (UWorld* World = GetWorld())
	{
		// This timer runs on the world's real clock — nothing in this path ever dilates it.
		// Reapplying hitstop refreshes the freeze window instead of an older timer restoring
		// presentation in the middle of a newer impact.
		World->GetTimerManager().ClearTimer(HitStopTimerHandle);
		World->GetTimerManager().SetTimer(HitStopTimerHandle, this, &AMoonCharacterBase::EndHitStop, RealDuration, false);
	}
}

void AMoonCharacterBase::EndHitStop()
{
	// Freeze window elapsed: resume anim playback and start the short blend from the held freeze
	// pose back to the mesh's real, Capsule-following transform. Never an instant snap (Core
	// Rule 9) — UpdateHitStopPresentation() does the actual blending, called every Tick().
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->bPauseAnims = false;
	}

	if (HitStopPhase == EMoonHitStopPhase::Freezing)
	{
		HitStopPhase = EMoonHitStopPhase::BlendingOut;
		HitStopBlendElapsed = 0.0f;
	}
}

void AMoonCharacterBase::UpdateHitStopPresentation(float DeltaTime)
{
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp || HitStopPhase == EMoonHitStopPhase::Inactive)
	{
		return;
	}

	if (HitStopPhase == EMoonHitStopPhase::Freezing)
	{
		// Super::Tick() (called earlier this frame) already moved the Capsule — and, via normal
		// attachment, would have moved the mesh right along with it — so force the mesh back to
		// the captured freeze transform every tick. This is what makes the freeze visual-only
		// instead of a "frozen pose sliding along the real position" artifact (Core Rule 9).
		HitStopBlendCurrentTransform = FreezeStartMeshTransform;
		MeshComp->SetWorldTransform(HitStopBlendCurrentTransform, false, nullptr, ETeleportType::TeleportPhysics);
		return;
	}

	// BlendingOut: interpolate from the held freeze pose toward the mesh's natural, Capsule-
	// following transform — computed from the captured relative offset composed with the attach
	// parent's CURRENT world transform, NOT MeshComp->GetComponentTransform(). After a Freezing
	// tick, the component's stored relative transform has already been overwritten by the forced
	// SetWorldTransform() above, so GetComponentTransform() would report the frozen position, not
	// the natural one.
	const USceneComponent* AttachParent = MeshComp->GetAttachParent();
	const FTransform NaturalWorldTransform = AttachParent
		? CapturedMeshRelativeTransform * AttachParent->GetComponentTransform()
		: MeshComp->GetComponentTransform();

	HitStopBlendElapsed += DeltaTime;

	const FVector NewLocation = FMath::VInterpTo(HitStopBlendCurrentTransform.GetLocation(), NaturalWorldTransform.GetLocation(), DeltaTime, HitStopBlendOutInterpSpeed);
	const FQuat NewRotation = FQuat::Slerp(HitStopBlendCurrentTransform.GetRotation(), NaturalWorldTransform.GetRotation(), FMath::Clamp(DeltaTime * HitStopBlendOutInterpSpeed, 0.0f, 1.0f));
	HitStopBlendCurrentTransform = FTransform(NewRotation, NewLocation, NaturalWorldTransform.GetScale3D());

	// A plain "distance below epsilon" convergence check alone would not be reliable here: the
	// blend target (NaturalWorldTransform) keeps moving every tick because the Capsule keeps
	// moving, so exponential decay toward it settles at a nonzero steady-state lag rather than
	// ever reaching exactly zero. HitStopBlendOutDuration is the hard stop that actually ends the
	// blend ("1-2 frames" per the ADR/GDD); the distance check remains as an early-out for the
	// (common) case where the blend closes the gap before the cap is reached.
	const bool bLocationConverged = FVector::DistSquared(NewLocation, NaturalWorldTransform.GetLocation()) < KINDA_SMALL_NUMBER;
	const bool bElapsedCapReached = HitStopBlendElapsed >= HitStopBlendOutDuration;

	if (bLocationConverged || bElapsedCapReached)
	{
		// Restore the EXACT original relative offset rather than trusting the last blended world
		// transform — repeated SetWorldTransform() calls during Freezing/BlendingOut overwrite the
		// component's stored relative transform every tick, so this is what guarantees zero
		// residual drift across many hitstops in a row instead of it silently accumulating.
		MeshComp->SetRelativeTransform(CapturedMeshRelativeTransform);
		HitStopPhase = EMoonHitStopPhase::Inactive;
		return;
	}

	MeshComp->SetWorldTransform(HitStopBlendCurrentTransform, false, nullptr, ETeleportType::TeleportPhysics);
}

void AMoonCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	// Movement tuning clamp + AirTime joint bound enforcement (TR-mov-004) — load-time validation
	// pass. See ValidateAndClampMovementTuning()'s header comment for the runtime-write-time caveat
	// (no runtime setter exists yet, so this is the only call site today) and the known Tick()/
	// GravityScale gap this does not close.
	ValidateAndClampMovementTuning();

	// Initialize the Ability System Component
	if (AbilitySystemComponent)
	{
		// Both Owner and Avatar are this character (solo play)
		AbilitySystemComponent->InitAbilityActorInfo(this, this);

		InitializeAttributes();
		InitializeAbilities();
	}
}

float AMoonCharacterBase::ComputeAirTime(float JumpZVelocity, float GravityScale) const
{
	// player-movement.md Formulas section, Jump Air Time: AirTime = (2 * JumpZVelocity) /
	// (GravityScale * abs(WorldGravityZ)). WorldGravityZ is UWorld::GetGravityZ() (the world's
	// unscaled gravity, e.g. -980) — deliberately not UCharacterMovementComponent::GetGravityZ(),
	// which already bakes GravityScale in and would silently square it here (the GDD explicitly
	// warns against this — the discrepancy only shows up away from the default GravityScale=1.0).
	const float WorldGravityZ = GetWorld() ? GetWorld()->GetGravityZ() : DefaultWorldGravityZ;
	return (2.0f * JumpZVelocity) / (GravityScale * FMath::Abs(WorldGravityZ));
}

void AMoonCharacterBase::ValidateAndClampMovementTuning()
{
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!MoveComp)
	{
		return;
	}

	// --- Individual hard clamps (TR-mov-004) — must run before the AirTime joint bound check below,
	// so that check never divides by an unclamped (zero/negative) GravityScale.
	if (MoveComp->MaxWalkSpeed < MinMaxWalkSpeed)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MoonMovementTuning] MaxWalkSpeed %.3f is below the hard clamp minimum %.3f — clamping."), MoveComp->MaxWalkSpeed, MinMaxWalkSpeed);
		MoveComp->MaxWalkSpeed = MinMaxWalkSpeed;
	}
	if (MoveComp->JumpZVelocity < MinJumpZVelocity)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MoonMovementTuning] JumpZVelocity %.3f is below the hard clamp minimum %.3f — clamping."), MoveComp->JumpZVelocity, MinJumpZVelocity);
		MoveComp->JumpZVelocity = MinJumpZVelocity;
	}
	if (MoveComp->GravityScale < MinGravityScale)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MoonMovementTuning] GravityScale %.3f is below the hard clamp minimum %.3f — clamping."), MoveComp->GravityScale, MinGravityScale);
		MoveComp->GravityScale = MinGravityScale;
	}
	if (MoveComp->MaxAcceleration < MinMaxAcceleration)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MoonMovementTuning] MaxAcceleration %.3f is below the hard clamp minimum %.3f — clamping."), MoveComp->MaxAcceleration, MinMaxAcceleration);
		MoveComp->MaxAcceleration = MinMaxAcceleration;
	}
	if (MoveComp->BrakingDecelerationWalking < MinBrakingDecelerationWalking)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MoonMovementTuning] BrakingDecelerationWalking %.3f is below the hard clamp minimum %.3f — clamping."), MoveComp->BrakingDecelerationWalking, MinBrakingDecelerationWalking);
		MoveComp->BrakingDecelerationWalking = MinBrakingDecelerationWalking;
	}
	if (MoveComp->GroundFriction < MinGroundFriction)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MoonMovementTuning] GroundFriction %.3f is below the hard clamp minimum %.3f — clamping."), MoveComp->GroundFriction, MinGroundFriction);
		MoveComp->GroundFriction = MinGroundFriction;
	}
	if (FallingGravityScaleMultiplier < MinGravityScale)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MoonMovementTuning] FallingGravityScaleMultiplier %.3f is below the hard clamp minimum %.3f — clamping."), FallingGravityScaleMultiplier, MinGravityScale);
		FallingGravityScaleMultiplier = MinGravityScale;
	}

	// --- AirTime joint bound (TR-mov-004) — only evaluated AFTER the individual clamps above, using
	// the now-guaranteed-safe (non-zero, non-negative) clamped values. BaseGravityScale (not
	// MoveComp->GravityScale) is the value being validated here and is what Tick()'s asymmetric
	// jump-feel multiply reads as its base going forward — see BaseGravityScale's header comment.
	const float ClampedJumpZVelocity = MoveComp->JumpZVelocity;
	const float ClampedGravityScale = MoveComp->GravityScale;
	const float CandidateAirTime = ComputeAirTime(ClampedJumpZVelocity, ClampedGravityScale);
	const bool bCandidateValid = CandidateAirTime >= AirTimeJointBoundMinSeconds && CandidateAirTime <= AirTimeJointBoundMaxSeconds;

	if (!bHasValidatedMovementTuning)
	{
		// Bootstrap: no previous valid pair exists yet — this is the very first validation pass.
		if (!bCandidateValid)
		{
			// No previous value to revert to — this is a content/config error. Fall back to the
			// GDD's own documented safe defaults as the last resort (player-movement.md Tuning
			// Knobs table "Current Value" column), not to whatever was configured.
			UE_LOG(LogTemp, Error, TEXT("[MoonMovementTuning] Initial JumpZVelocity=%.3f/GravityScale=%.3f produce AirTime=%.3fs, outside the joint bound [%.2fs, %.2fs], and no previous valid tuning exists yet — falling back to GDD defaults JumpZVelocity=%.1f/GravityScale=%.1f."),
				ClampedJumpZVelocity, ClampedGravityScale, CandidateAirTime, AirTimeJointBoundMinSeconds, AirTimeJointBoundMaxSeconds, DefaultJumpZVelocity, DefaultGravityScale);
			MoveComp->JumpZVelocity = DefaultJumpZVelocity;
			MoveComp->GravityScale = DefaultGravityScale;
		}
		LastValidJumpZVelocity = MoveComp->JumpZVelocity;
		LastValidGravityScale = MoveComp->GravityScale;
		bHasValidatedMovementTuning = true;
	}
	else if (!bCandidateValid)
	{
		// A previously-validated pair exists — revert to it rather than falling back to engine/GDD
		// defaults (story-mandated revert policy). The rejected combination is intentionally NOT
		// written into LastValidJumpZVelocity/LastValidGravityScale below — it must not become the
		// new baseline for future reverts.
		UE_LOG(LogTemp, Warning, TEXT("[MoonMovementTuning] JumpZVelocity=%.3f/GravityScale=%.3f produce AirTime=%.3fs, outside the joint bound [%.2fs, %.2fs] — reverting to the last known-valid pair JumpZVelocity=%.3f/GravityScale=%.3f."),
			ClampedJumpZVelocity, ClampedGravityScale, CandidateAirTime, AirTimeJointBoundMinSeconds, AirTimeJointBoundMaxSeconds, LastValidJumpZVelocity, LastValidGravityScale);
		MoveComp->JumpZVelocity = LastValidJumpZVelocity;
		MoveComp->GravityScale = LastValidGravityScale;
	}
	else
	{
		// Joint bound passed — this combination becomes the new last-known-valid pair.
		LastValidJumpZVelocity = ClampedJumpZVelocity;
		LastValidGravityScale = ClampedGravityScale;
	}

	// BaseGravityScale is the final word on "what GravityScale actually is" from here on — Tick()
	// reads this every frame instead of GetCharacterMovement()->GravityScale, which it overwrites
	// with BaseGravityScale * feel-multiplier and would otherwise be misread as the base on any
	// later re-validation pass.
	BaseGravityScale = MoveComp->GravityScale;
}

void AMoonCharacterBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// Add Input Mapping Context. Done here rather than BeginPlay: for GameMode-spawned
	// pawns, BeginPlay fires before Possess(), so Controller is still null at that point.
	if (APlayerController* PlayerController = Cast<APlayerController>(NewController))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}
}

void AMoonCharacterBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Add Input Mapping Context here rather than BeginPlay/PossessedBy: this is called from
	// PawnClientRestart, client-side, guaranteed after possession, with GetLocalPlayer() valid.
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();

		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("DefaultMappingContext is not set on %s."), *GetNameSafe(this));
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("EnhancedInputLocalPlayerSubsystem not found for %s."), *GetNameSafe(this));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("SetupPlayerInputComponent: Controller is not a PlayerController (%s)."), Controller ? *Controller->GetClass()->GetName() : TEXT("None"));
	}

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMoonCharacterBase::Move);
		}

		if (LookAction)
		{
			EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMoonCharacterBase::Look);
		}

		if (DashAction)
		{
			EnhancedInputComponent->BindAction(DashAction, ETriggerEvent::Triggered, this, &AMoonCharacterBase::Input_Dash);
		}

		if (SpellBlackholeAction)
		{
			EnhancedInputComponent->BindAction(SpellBlackholeAction, ETriggerEvent::Triggered, this, &AMoonCharacterBase::Input_SpellBlackhole);
		}

		if (SpellFireAction)
		{
			EnhancedInputComponent->BindAction(SpellFireAction, ETriggerEvent::Triggered, this, &AMoonCharacterBase::Input_SpellFire);
		}

		if (SpellLightningAction)
		{
			EnhancedInputComponent->BindAction(SpellLightningAction, ETriggerEvent::Triggered, this, &AMoonCharacterBase::Input_SpellLightning);
		}

		if (ExecuteAction)
		{
			EnhancedInputComponent->BindAction(ExecuteAction, ETriggerEvent::Triggered, this, &AMoonCharacterBase::Input_Execute);
		}

		if (JumpAction)
		{
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AMoonCharacterBase::Input_Jump);
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AMoonCharacterBase::Input_StopJumping);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("JumpAction is not set on %s — jump input will not be bound."), *GetNameSafe(this));
		}
	}
}

void AMoonCharacterBase::Move(const FInputActionValue& Value)
{
	// Movement Insights trace (TR-mov-009 / ADR-0009 Decision 6): the very first statement in the
	// Enhanced Input Triggered callback for IA_Move, before the MovementLocked gate below — this
	// measures Enhanced Input's delegate dispatch latency itself, so it must fire unconditionally
	// regardless of whether the input is subsequently gated. Name kept exactly as ADR-0009
	// specifies: "MovementInputTrace.InputTriggered".
	TRACE_CPUPROFILER_EVENT_SCOPE(MovementInputTrace.InputTriggered);

	// MovementLocked (TR-mov-006): access-control gate for the not-yet-designed Status Effect
	// system. bMovementLocked has no writer yet (SetMovementLocked() is private and uncalled) —
	// this check exists now so the gate is already correct once that system is designed.
	if (bMovementLocked)
	{
		return;
	}

	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AMoonCharacterBase::SetMovementLocked(bool bLocked)
{
	// TR-mov-006 reservation: intentionally uncalled. The Status Effect system (not yet
	// designed) is the only intended future caller — its ADR must expose this via a narrow
	// friend/interface grant rather than this method becoming public/BlueprintCallable directly.
	// Do not add a call site here or elsewhere until that ADR exists (ADR-0009 Decision 3).
	bMovementLocked = bLocked;
}

void AMoonCharacterBase::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AMoonCharacterBase::Input_Dash(const FInputActionValue& Value)
{
	TryActivateAbilityByTag(FGameplayTag::RequestGameplayTag(FName("Ability.Dash")));
}

void AMoonCharacterBase::Input_SpellBlackhole(const FInputActionValue& Value)
{
	TryActivateAbilityByTag(FGameplayTag::RequestGameplayTag(FName("Spell.Element.Blackhole")));
}

void AMoonCharacterBase::Input_SpellFire(const FInputActionValue& Value)
{
	TryActivateAbilityByTag(FGameplayTag::RequestGameplayTag(FName("Spell.Element.Fire")));
}

void AMoonCharacterBase::Input_SpellLightning(const FInputActionValue& Value)
{
	TryActivateAbilityByTag(FGameplayTag::RequestGameplayTag(FName("Spell.Element.Lightning")));
}

void AMoonCharacterBase::Input_Execute(const FInputActionValue& Value)
{
	if (!GetWorld())
	{
		return;
	}

	ATargetDummy* NearestExecutable = nullptr;
	float NearestDistanceSquared = FMath::Square(ExecutionRange);
	for (TActorIterator<ATargetDummy> It(GetWorld()); It; ++It)
	{
		if (!It->IsExecutable())
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared(GetActorLocation(), It->GetActorLocation());
		if (DistanceSquared <= NearestDistanceSquared)
		{
			NearestDistanceSquared = DistanceSquared;
			NearestExecutable = *It;
		}
	}

	if (NearestExecutable)
	{
		NearestExecutable->TryExtractCore(this);
	}
}

void AMoonCharacterBase::Input_Jump()
{
	const bool bIsFalling = GetCharacterMovement()->IsFalling();

	// Coyote time (TR-mov-007): native ACharacter::CanJump() unconditionally refuses a jump when
	// JumpCurrentCount==0 while airborne (it has no concept of a post-ground-exit grace window),
	// so a plain Jump() call here would silently do nothing during the coyote window. Bypass that
	// gate directly with LaunchCharacter using the same JumpZVelocity CMC would have applied.
	if (bIsFalling && JumpCurrentCount == 0 && IsWithinGraceWindow(CoyoteTimeTimer))
	{
		// Reset to the unarmed sentinel, not 0.0f — see the identical note in Landed().
		CoyoteTimeTimer = UnarmedTimerSentinel;
		LaunchCharacter(FVector(0.0f, 0.0f, GetCharacterMovement()->JumpZVelocity), false, true);
		return;
	}

	// Jump input buffer (TR-mov-007): pressing jump while airborne and not coyote-eligible is
	// buffered so landing within the grace window still triggers a jump (see Landed()). Jump()
	// is still called normally below — this is purely additive, e.g. a double-jump ability would
	// still fire through the native path unaffected.
	if (bIsFalling)
	{
		JumpInputBufferTimer = JumpGraceWindowSeconds;
	}

	Jump();
}

void AMoonCharacterBase::Input_StopJumping()
{
	StopJumping();
}

void AMoonCharacterBase::InjectZImpulse(float ZVelocity)
{
	// TR-mov-005 safe hook — SETS Velocity.Z via LaunchCharacter's bZOverride=true (does not add to
	// any existing Z velocity). See the header declaration's comment for composition/override-scope
	// caveats. No caller is wired up yet (Dash/Evasion, Arena Morphing) — this only exposes the hook.
	LaunchCharacter(FVector(0.0f, 0.0f, ZVelocity), false, true);
}

void AMoonCharacterBase::TryActivateAbilityByTag(FGameplayTag AbilityTag)
{
	if (AbilitySystemComponent)
	{
		// TryActivateAbilitiesByTag looks for an exact or matching tag.
		// The ability itself will be granted with this tag as an AbilityTag.
		FGameplayTagContainer TagContainer;
		TagContainer.AddTag(AbilityTag);
		AbilitySystemComponent->TryActivateAbilitiesByTag(TagContainer);
	}
}

bool AMoonCharacterBase::CheckAndConsumeSpellCastLimit(FGameplayTag ElementTag)
{
	uint64 CurrentFrame = GFrameCounter;
	
	// 1. Per-element per-frame cap
	if (CurrentFrame == LastCastFrame && ElementTag == LastCastElementTag)
	{
		return false;
	}

	// 2. Global rate limit (MaxCastsPerSecond = 20)
	float CurrentTime = GetWorld()->GetTimeSeconds();
	
	// Remove timestamps older than 1 second
	CastTimestamps.RemoveAll([CurrentTime](float Time) { return CurrentTime - Time > 1.0f; });
	
	// Check against the Tuning Knob (20)
	if (CastTimestamps.Num() >= 20)
	{
		return false;
	}

	// Success! Consume the limit
	LastCastFrame = CurrentFrame;
	LastCastElementTag = ElementTag;
	CastTimestamps.Add(CurrentTime);
	
	return true;
}


void AMoonCharacterBase::InitializeAttributes()
{
	if (AbilitySystemComponent && DefaultAttributesEffect)
	{
		FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
		Context.AddInstigator(this, this);
		
		AbilitySystemComponent->ApplyGameplayEffectToSelf(
			DefaultAttributesEffect.GetDefaultObject(), 
			1.0f, 
			Context
		);
	}
}

void AMoonCharacterBase::InitializeAbilities()
{
	if (AbilitySystemComponent)
	{
		for (TSubclassOf<UGameplayAbility>& Ability : DefaultAbilities)
		{
			if (Ability)
			{
				AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(Ability, 1, INDEX_NONE, this));
			}
		}
	}
}

void AMoonCharacterBase::AddTension(float Amount)
{
	if (!AttributeSet || !AbilitySystemComponent) return;

	const double CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	UpdateOverdriveState(CurrentTime);
	if (OverdriveState.IsTensionGainLocked(CurrentTime))
	{
		return;
	}

	float CurrentTension = AttributeSet->GetTensionGauge();
	float MaxTension = AttributeSet->GetTensionGaugeMax();
	
	float NewTension = FMath::Clamp(CurrentTension + Amount, 0.0f, MaxTension);
	AttributeSet->SetTensionGauge(NewTension);

	LastTensionGainTime = static_cast<float>(CurrentTime);

	// Overdrive Trigger (Rule 5)
	if (NewTension >= MaxTension)
	{
		AttributeSet->SetTensionGauge(0.0f);
		TriggerOverdrive();
	}
}

bool AMoonCharacterBase::TriggerOverdrive()
{
	if (!AbilitySystemComponent || !AttributeSet || !GetWorld() || AttributeSet->GetHealth() <= 0.0f)
	{
		return false;
	}

	const double CurrentTime = GetWorld()->GetTimeSeconds();
	UpdateOverdriveState(CurrentTime);
	if (!OverdriveState.TryStart(CurrentTime, OverdriveDuration))
	{
		return false;
	}

	const FGameplayTag CostBypassTag = FGameplayTag::RequestGameplayTag(FName("CostBypass.Active"));
	AbilitySystemComponent->SetLooseGameplayTagCount(CostBypassTag, 1);
	OnOverdriveStarted.Broadcast();
	return true;
}

void AMoonCharacterBase::ForceEndOverdrive(EMoonOverdriveEndReason Reason)
{
	if (!AbilitySystemComponent || OverdriveState.GetPhase() == EMoonOverdrivePhase::Inactive)
	{
		return;
	}

	const FGameplayTag CostBypassTag = FGameplayTag::RequestGameplayTag(FName("CostBypass.Active"));
	AbilitySystemComponent->SetLooseGameplayTagCount(CostBypassTag, 0);
	OverdriveState.Reset();
	OnOverdriveEnded.Broadcast(Reason);
}

bool AMoonCharacterBase::IsOverdriveActive() const
{
	return GetWorld() && OverdriveState.IsActive(GetWorld()->GetTimeSeconds());
}

bool AMoonCharacterBase::IsTensionGainLocked() const
{
	return GetWorld() && OverdriveState.IsTensionGainLocked(GetWorld()->GetTimeSeconds());
}

float AMoonCharacterBase::GetOverdriveTimeRemaining() const
{
	return GetWorld() ? OverdriveState.GetActiveTimeRemaining(GetWorld()->GetTimeSeconds()) : 0.0f;
}

void AMoonCharacterBase::UpdateOverdriveState(double CurrentTime)
{
	if (OverdriveState.TryExpire(CurrentTime, OverdriveRecoveryDuration))
	{
		if (AbilitySystemComponent)
		{
			const FGameplayTag CostBypassTag = FGameplayTag::RequestGameplayTag(FName("CostBypass.Active"));
			AbilitySystemComponent->SetLooseGameplayTagCount(CostBypassTag, 0);
		}
		OnOverdriveEnded.Broadcast(EMoonOverdriveEndReason::Expired);
	}

	OverdriveState.TryFinishRecovery(CurrentTime);
}

void AMoonCharacterBase::AddTensionFromSpellHit(float ManaCost)
{
	AddTension(ManaCost * TensionGainCoefficient);
}

void AMoonCharacterBase::AddTensionFromJustDodge()
{
	AddTension(JustDodgeTensionBonus);
}

void AMoonCharacterBase::ApplyTensionDamagePenalty()
{
	if (!AttributeSet) return;
	float CurrentTension = AttributeSet->GetTensionGauge();
	float NewTension = CurrentTension * (1.0f - DamagePenaltyPercent);
	AttributeSet->SetTensionGauge(NewTension);
}
