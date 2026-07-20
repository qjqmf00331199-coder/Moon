#include "MoonGameplayAbility_Dash.h"
#include "MoonAttributeSet.h"
#include "MoonAbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Animation/AnimSequence.h"
#include "UObject/ConstructorHelpers.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "../Character/MoonCharacterBase.h"

UMoonGameplayAbility_Dash::UMoonGameplayAbility_Dash()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// GDD Rule 5: 회피 프레임 (I-frames)
	// Add State.Invulnerable automatically when active
	ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Invulnerable")));

	// Default Dash motion — project has no dedicated Dash anim yet. Aurora (an ice mage, no
	// blink/dash in her actual kit) has nothing purpose-built for this, so reuse "Jog_Fwd_Start"
	// (the forward-leaning run-start pose, same skeleton the Moon character mesh already uses
	// for Idle/Jog) — closer to a forward-thrust feel than Aurora's ability animations, which
	// are all melee/cast poses. EditDefaultsOnly so both can be overridden later once a
	// purpose-built Dash animation exists.
	static ConstructorHelpers::FObjectFinder<UAnimSequence> DashAnimFinder(TEXT("/Game/ParagonAurora/Characters/Heroes/Aurora/Animations/Jog_Fwd_Start.Jog_Fwd_Start"));
	if (DashAnimFinder.Succeeded())
	{
		DashAnim = DashAnimFinder.Object;
	}

	// Aurora's own Dash VFX — imported with the character pack but never wired up until now.
	// Carries most of the "impact" read regardless of body-pose animation (same technique
	// most action games use: Genshin, Warframe, etc. lean on burst/trail VFX for dash feel).
	static ConstructorHelpers::FObjectFinder<UParticleSystem> WarmUpFXFinder(TEXT("/Game/ParagonAurora/FX/Particles/Abilities/Dash/FX/P_Aurora_Dash_WarmUp.P_Aurora_Dash_WarmUp"));
	if (WarmUpFXFinder.Succeeded())
	{
		DashWarmUpFX = WarmUpFXFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UParticleSystem> TrailFXFinder(TEXT("/Game/ParagonAurora/FX/Particles/Abilities/Dash/FX/P_Aurora_Dash_Flare.P_Aurora_Dash_Flare"));
	if (TrailFXFinder.Succeeded())
	{
		DashTrailFX = TrailFXFinder.Object;
	}
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
		const FVector DashDirection = ApplyDashImpulse(Character);

		// Dash motion (one-shot, no AnimBlueprint/montage system yet — see PlayOneShotAnim)
		if (DashAnim)
		{
			Character->PlayOneShotAnim(DashAnim, DashAnimPlayRate);
		}

		// Dash VFX: a one-shot burst at the moment of the dash, plus a trail that rides along
		// for the dash's duration (deactivated, not destroyed, in OnDashFinished so its existing
		// particles fade out naturally instead of popping off). Oriented to DashDirection rather
		// than snapped to the mesh's facing — SnapToTarget ignores the Rotation param entirely
		// and just matches the mesh, which is wrong for a back-dash/strafe-dash where the
		// character doesn't face the direction it's actually dashing in. KeepWorldPosition
		// respects the explicit world rotation we pass while still following the character.
		const FRotator DashRotation = DashDirection.Rotation();
		if (DashWarmUpFX)
		{
			UGameplayStatics::SpawnEmitterAttached(DashWarmUpFX, Character->GetMesh(), NAME_None, Character->GetActorLocation(), DashRotation, FVector(1.0f), EAttachLocation::KeepWorldPosition, true);
		}
		if (DashTrailFX)
		{
			ActiveDashTrailComponent = UGameplayStatics::SpawnEmitterAttached(DashTrailFX, Character->GetMesh(), NAME_None, Character->GetActorLocation(), DashRotation, FVector(1.0f), EAttachLocation::KeepWorldPosition, false);
		}

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

FVector UMoonGameplayAbility_Dash::ApplyDashImpulse(ACharacter* Character)
{
	if (!Character) return FVector::ForwardVector;

	FVector DashDirection = Character->GetActorForwardVector();
	// Calculate a horizontal input direction. If there is no input, use actor forward.
	UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement();
	if (MoveComp)
	{
		FVector InputVector = MoveComp->GetLastInputVector();
		InputVector.Z = 0.0f;
		if (!InputVector.Normalize())
		{
			DashDirection.Z = 0.0f;
			DashDirection.Normalize();
		}
		else
		{
			DashDirection = InputVector;
		}

		// Preserve the existing tuning formula (600 * 2.5 * 0.2 = 300uu by default), but
		// apply that distance immediately. Sweep=true prevents stepping through walls.
		const float DashDistance = MoveComp->MaxWalkSpeed * DashSpeedMultiplier * DashDuration;
		Character->SetActorLocation(Character->GetActorLocation() + (DashDirection * DashDistance), true);

		// Do not let held movement input continue advancing the character during the short
		// animation/i-frame window after the instant reposition.
		bRestoreFallingMovement = MoveComp->IsFalling();
		MoveComp->StopMovementImmediately();
		MoveComp->DisableMovement();
		bMovementLockedByDash = true;
	}

	return DashDirection;
}

void UMoonGameplayAbility_Dash::RestoreCharacterMovement(ACharacter* Character)
{
	if (!bMovementLockedByDash || !Character)
	{
		return;
	}

	if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
	{
		MoveComp->SetMovementMode(bRestoreFallingMovement ? MOVE_Falling : MOVE_Walking);
	}

	bMovementLockedByDash = false;
}

void UMoonGameplayAbility_Dash::CheckJustDodge(ACharacter* PlayerCharacter)
{
	// TODO: Iterate over enemies and check their attack commit time vs JustDodgeWindow
	// This will grant State.Executable to enemies and refund 1 charge if successful.
	// Skeleton implemented for now.
}

void UMoonGameplayAbility_Dash::OnDashFinished()
{
	if (ActiveDashTrailComponent.IsValid())
	{
		ActiveDashTrailComponent->Deactivate();
		ActiveDashTrailComponent.Reset();
	}

	if (AMoonCharacterBase* Character = Cast<AMoonCharacterBase>(GetAvatarActorFromActorInfo()))
	{
		// Dash impact: local-only hitstop so the rest of the combat simulation stays live.
		Character->TriggerHitStop(0.055f);
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UMoonGameplayAbility_Dash::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DashTimerHandle);
	}

	RestoreCharacterMovement(Cast<ACharacter>(ActorInfo->AvatarActor.Get()));

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
