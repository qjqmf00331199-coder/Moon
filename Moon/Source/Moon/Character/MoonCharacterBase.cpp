#include "MoonCharacterBase.h"
#include "../GAS/MoonAbilitySystemComponent.h"
#include "../GAS/MoonAttributeSet.h"
#include "GameplayAbilitySpec.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"

AMoonCharacterBase::AMoonCharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;

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
}

void AMoonCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}

	// Initialize the Ability System Component
	if (AbilitySystemComponent)
	{
		// Both Owner and Avatar are this character (solo play)
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
		
		InitializeAttributes();
		InitializeAbilities();
	}
}

void AMoonCharacterBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
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
	}
}

void AMoonCharacterBase::Move(const FInputActionValue& Value)
{
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
