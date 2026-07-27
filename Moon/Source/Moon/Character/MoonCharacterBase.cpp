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

	const double CurrentWorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	UpdateOverdriveState(CurrentWorldTime);

	if (AttributeSet)
	{
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

	// Jump feel: fall faster than we rise (asymmetric gravity) for a snappier arc instead of
	// UE's default floaty symmetric one. Only touches GravityScale while actually descending.
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		const bool bDescending = MoveComp->IsFalling() && GetVelocity().Z < 0.0f;
		MoveComp->GravityScale = bDescending ? FallingGravityScaleMultiplier : 1.0f;
	}

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

	// Basic locomotion: swap the single-node playing animation between idle and jog by speed.
	// No AnimBlueprint/blendspace yet, so this is a hard switch rather than a blend.
	// Suppressed while a one-shot anim (jump start/land, dash, spell cast) controls the mesh.
	if (!bPlayingOneShotAnim)
	{
		if (USkeletalMeshComponent* MeshComp = GetMesh())
		{
			const float Speed = GetVelocity().Size();
			const bool bShouldJog = Speed > JogSpeedThreshold;
			static float DebugLogAccum = 0.0f;
			DebugLogAccum += DeltaTime;
			if (DebugLogAccum > 0.5f)
			{
				DebugLogAccum = 0.0f;
				UE_LOG(LogTemp, Warning, TEXT("[MoonDebug] Tick locomotion: Speed=%.1f bShouldJog=%d bIsPlayingJogAnim=%d JogAnim=%s IdleAnim=%s"),
					Speed, bShouldJog, bIsPlayingJogAnim, *GetNameSafe(JogAnim.Get()), *GetNameSafe(IdleAnim.Get()));
			}
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

		// Hold the mesh's anim playback position for the freeze duration. bPauseAnims is a
		// long-stable USkeletalMeshComponent property (predates this project's pinned engine
		// version by several years) but is not documented in this project's curated UE5.8
		// engine-reference library, so treat it as unverified-against-5.8-headers, low risk —
		// the same caveat this codebase already applies to bUseCameraLagSubstepping in ADR-0005.
		// Fallback if this ever needs re-verifying: GetSingleNodeInstance()->SetPlaying(false),
		// since this character's locomotion already goes through the single-node anim path.
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

	// Initialize the Ability System Component
	if (AbilitySystemComponent)
	{
		// Both Owner and Avatar are this character (solo play)
		AbilitySystemComponent->InitAbilityActorInfo(this, this);

		InitializeAttributes();
		InitializeAbilities();
	}
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

	UE_LOG(LogTemp, Warning, TEXT("[MoonDebug] SetupPlayerInputComponent called. Controller=%s DefaultMappingContext=%s"),
		*GetNameSafe(Controller), *GetNameSafe(DefaultMappingContext.Get()));

	// Add Input Mapping Context here rather than BeginPlay/PossessedBy: this is called from
	// PawnClientRestart, client-side, guaranteed after possession, with GetLocalPlayer() valid.
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
		UE_LOG(LogTemp, Warning, TEXT("[MoonDebug] Controller cast OK. LocalPlayer=%s"), *GetNameSafe(LocalPlayer));

		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
		{
			UE_LOG(LogTemp, Warning, TEXT("[MoonDebug] Subsystem OK."));
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
				UE_LOG(LogTemp, Warning, TEXT("[MoonDebug] AddMappingContext called for %s. HasMappingContext=%s"),
					*GetNameSafe(DefaultMappingContext.Get()), Subsystem->HasMappingContext(DefaultMappingContext) ? TEXT("true") : TEXT("false"));
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[MoonDebug] DefaultMappingContext is NULL on this instance!"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[MoonDebug] Subsystem is NULL!"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[MoonDebug] Controller cast FAILED. Controller class=%s"), Controller ? *Controller->GetClass()->GetName() : TEXT("None"));
	}

	UE_LOG(LogTemp, Warning, TEXT("[MoonDebug] PlayerInputComponent class=%s, EnhancedInputComponent cast=%s"),
		PlayerInputComponent ? *PlayerInputComponent->GetClass()->GetName() : TEXT("None"),
		Cast<UEnhancedInputComponent>(PlayerInputComponent) ? TEXT("OK") : TEXT("FAILED"));

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMoonCharacterBase::Move);
			UE_LOG(LogTemp, Warning, TEXT("[MoonDebug] Bound MoveAction %s"), *GetNameSafe(MoveAction.Get()));
		}

		if (LookAction)
		{
			EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMoonCharacterBase::Look);
		}

		if (DashAction)
		{
			EnhancedInputComponent->BindAction(DashAction, ETriggerEvent::Triggered, this, &AMoonCharacterBase::Input_Dash);
			UE_LOG(LogTemp, Warning, TEXT("[MoonDebug] Bound DashAction %s"), *GetNameSafe(DashAction.Get()));
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
			UE_LOG(LogTemp, Warning, TEXT("[MoonDebug] Bound JumpAction %s"), *GetNameSafe(JumpAction.Get()));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[MoonDebug] JumpAction is NULL — not bound!"));
		}
	}
}

void AMoonCharacterBase::Move(const FInputActionValue& Value)
{
	// MovementLocked (TR-mov-006): access-control gate for the not-yet-designed Status Effect
	// system. bMovementLocked has no writer yet (SetMovementLocked() is private and uncalled) —
	// this check exists now so the gate is already correct once that system is designed.
	if (bMovementLocked)
	{
		return;
	}

	FVector2D MovementVector = Value.Get<FVector2D>();

	UE_LOG(LogTemp, Warning, TEXT("[MoonDebug] Move fired: %s"), *MovementVector.ToString());

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
	UE_LOG(LogTemp, Warning, TEXT("[MoonDebug] Input_Dash fired"));
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
	UE_LOG(LogTemp, Warning, TEXT("[MoonDebug] Input_Jump fired. CanJump=%d IsFalling=%d JumpCurrentCount=%d/%d MovementMode=%d"),
		CanJump(), GetCharacterMovement()->IsFalling(), JumpCurrentCount, JumpMaxCount, (int32)GetCharacterMovement()->MovementMode.GetValue());

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
	UE_LOG(LogTemp, Warning, TEXT("[MoonDebug] Input_StopJumping fired"));
	StopJumping();
}

void AMoonCharacterBase::TryActivateAbilityByTag(FGameplayTag AbilityTag)
{
	if (AbilitySystemComponent)
	{
		// TryActivateAbilitiesByTag looks for an exact or matching tag.
		// The ability itself will be granted with this tag as an AbilityTag.
		FGameplayTagContainer TagContainer;
		TagContainer.AddTag(AbilityTag);
		const bool bActivated = AbilitySystemComponent->TryActivateAbilitiesByTag(TagContainer);
		UE_LOG(LogTemp, Warning, TEXT("[MoonDebug] TryActivateAbilityByTag %s -> %s"), *AbilityTag.ToString(), bActivated ? TEXT("true") : TEXT("false"));
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
