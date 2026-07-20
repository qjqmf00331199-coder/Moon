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

	// Basic locomotion: swap the single-node playing animation between idle and jog by speed.
	// No AnimBlueprint/blendspace yet, so this is a hard switch rather than a blend.
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
