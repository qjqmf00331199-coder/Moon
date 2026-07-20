#include "MoonGameplayAbility_Dash.h"
#include "MoonAttributeSet.h"
#include "MoonAbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "../Character/MoonCharacterBase.h"

UMoonGameplayAbility_Dash::UMoonGameplayAbility_Dash()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// GDD Rule 5: 회피 프레임 (I-frames)
	// Add State.Invulnerable automatically when active
	ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Invulnerable")));
}

bool UMoonGameplayAbility_Dash::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	// Rule 9: MovementLocked 무시 불가
	if (ActorInfo->AbilitySystemComponent.IsValid())
	{
		if (ActorInfo->AbilitySystemComponent->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.MovementLocked"))))
		{
			return false;
		}
	}

	// Check if we have at least 1 charge
	AMoonCharacterBase* MoonChar = Cast<AMoonCharacterBase>(ActorInfo->AvatarActor.Get());
	if (MoonChar && MoonChar->GetAttributeSet())
	{
		if (MoonChar->GetAttributeSet()->GetDashCharges() < 1.0f)
		{
			return false;
		}
	}

	return true;
}

void UMoonGameplayAbility_Dash::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AMoonCharacterBase* Character = Cast<AMoonCharacterBase>(ActorInfo->AvatarActor.Get());
	if (Character)
	{
		// Consume 1 dash charge
		if (Character->GetAttributeSet())
		{
			float CurrentCharges = Character->GetAttributeSet()->GetDashCharges();
			Character->GetAttributeSet()->SetDashCharges(FMath::Max(CurrentCharges - 1.0f, 0.0f));
		}

		// Apply impulse
		ApplyDashImpulse(Character);

		// Perform Just Dodge Check (Skeleton)
		CheckJustDodge(Character);

		// Set timer to end dash
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(DashTimerHandle, this, &UMoonGameplayAbility_Dash::OnDashFinished, DashDuration, false);
		}
	}
	else
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}
}

void UMoonGameplayAbility_Dash::ApplyDashImpulse(ACharacter* Character) const
{
	if (!Character) return;

	FVector DashDirection;
	// Calculate input direction. If no input, use actor forward vector.
	// We can approximate this by checking the CharacterMovementComponent's LastInputVector
	UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement();
	if (MoveComp)
	{
		FVector InputVector = MoveComp->GetLastInputVector();
		if (InputVector.IsNearlyZero())
		{
			DashDirection = Character->GetActorForwardVector();
		}
		else
		{
			DashDirection = InputVector.GetSafeNormal();
		}

		// Calculate Dash Velocity
		float BaseSpeed = MoveComp->MaxWalkSpeed;
		FVector DashVelocity = DashDirection * BaseSpeed * DashSpeedMultiplier;

		// If falling/airborne, handle air-dash logic (Rule 3)
		if (MoveComp->IsFalling())
		{
			DashVelocity.Z = FMath::Max(DashVelocity.Z, 0.0f); // Prevents falling faster, acts as slight hover
		}

		// Override Velocity (Rule 4)
		Character->LaunchCharacter(DashVelocity, true, true);
	}
}

void UMoonGameplayAbility_Dash::CheckJustDodge(ACharacter* PlayerCharacter)
{
	// TODO: Iterate over enemies and check their attack commit time vs JustDodgeWindow
	// This will grant State.Executable to enemies and refund 1 charge if successful.
	// Skeleton implemented for now.
}

void UMoonGameplayAbility_Dash::OnDashFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UMoonGameplayAbility_Dash::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DashTimerHandle);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
