#include "MoonCharacterBase.h"
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

AMoonCharacterBase::AMoonCharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;

	// Third-person follow camera. Boom handles collision so the camera never clips into the level.
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->SetRelativeRotation(FRotator(-15.0f, 0.0f, 0.0f));
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

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

	if (AttributeSet)
	{
		// Mana Regen
		float CurrentMana = AttributeSet->GetMana();
		float MaxMana = AttributeSet->GetMaxMana();
		float ManaRegen = AttributeSet->GetManaRegenRate();
		if (CurrentMana < MaxMana && ManaRegen > 0.0f)
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
	if (bIsFalling && !bWasFalling && JumpStartAnim)
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
}

void AMoonCharacterBase::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	// Landing impact: pause only this character very briefly. CustomTimeDilation leaves
	// the world clock untouched, so enemies and projectiles continue to update normally.
	TriggerHitStop(0.055f);

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

void AMoonCharacterBase::TriggerHitStop(float RealDuration, float DilationScale)
{
	if (RealDuration <= 0.0f)
	{
		EndHitStop();
		return;
	}

	CustomTimeDilation = FMath::Clamp(DilationScale, 0.001f, 1.0f);

	if (UWorld* World = GetWorld())
	{
		// Timers advance on the world's clock, rather than this actor's dilated tick.
		// Reapplying hitstop refreshes the short window instead of allowing an older
		// timer to restore time in the middle of a newer impact.
		World->GetTimerManager().ClearTimer(HitStopTimerHandle);
		World->GetTimerManager().SetTimer(HitStopTimerHandle, this, &AMoonCharacterBase::EndHitStop, RealDuration, false);
	}
}

void AMoonCharacterBase::EndHitStop()
{
	CustomTimeDilation = 1.0f;
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

void AMoonCharacterBase::Input_Jump()
{
	UE_LOG(LogTemp, Warning, TEXT("[MoonDebug] Input_Jump fired. CanJump=%d IsFalling=%d JumpCurrentCount=%d/%d MovementMode=%d"),
		CanJump(), GetCharacterMovement()->IsFalling(), JumpCurrentCount, JumpMaxCount, (int32)GetCharacterMovement()->MovementMode.GetValue());
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

	// Check if overdrive is active (Rule 7)
	bool bOverdriveActive = AbilitySystemComponent->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("CostBypass.Active")));
	
	float ActualGain = Amount;
	if (bOverdriveActive)
	{
		ActualGain *= OverdriveTensionGainMultiplier;
	}

	float CurrentTension = AttributeSet->GetTensionGauge();
	float MaxTension = AttributeSet->GetTensionGaugeMax();
	
	float NewTension = FMath::Clamp(CurrentTension + ActualGain, 0.0f, MaxTension);
	AttributeSet->SetTensionGauge(NewTension);

	LastTensionGainTime = GetWorld()->GetTimeSeconds();

	// Overdrive Trigger (Rule 5)
	if (NewTension >= MaxTension)
	{
		AttributeSet->SetTensionGauge(0.0f);
		OnOverdriveTriggered();
	}
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
