#include "MoonGameplayAbility_Spell_Blackhole.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Effects/GE_SpellCost_Blackhole.h"
#include "Effects/GE_SpellCooldown_Blackhole.h"
#include "Effects/GE_Damage_Instant.h"
#include "../Character/MoonCharacterBase.h"
#include "../Character/TargetDummy.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Animation/AnimSequence.h"
#include "UObject/ConstructorHelpers.h"

UMoonGameplayAbility_Spell_Blackhole::UMoonGameplayAbility_Spell_Blackhole()
{
	// UE5.8: UGameplayAbility::AbilityTags is UE_DEPRECATED(5.5) in favor of SetAssetTags(),
	// which is documented as constructor-only. Verified against the installed 5.8 engine headers.
	SetAssetTags(FGameplayTagContainer(FGameplayTag::RequestGameplayTag(FName("Spell.Element.Blackhole"))));

	CostGameplayEffectClass = UGE_SpellCost_Blackhole::StaticClass();
	CooldownGameplayEffectClass = UGE_SpellCooldown_Blackhole::StaticClass();

	// Default cast motion — no purpose-built Blackhole cast anim yet, so reuse Aurora's generic
	// "Cast" (same skeleton the Moon character mesh already uses for Idle/Jog). This ability has
	// no Blueprint wrapper, so the default is set here rather than left for a BP CDO to assign.
	static ConstructorHelpers::FObjectFinder<UAnimSequence> CastAnimFinder(TEXT("/Game/ParagonAurora/Characters/Heroes/Aurora/Animations/Cast.Cast"));
	if (CastAnimFinder.Succeeded())
	{
		CastAnim = CastAnimFinder.Object;
	}
}

void UMoonGameplayAbility_Spell_Blackhole::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// Uses UMoonGameplayAbility_Spell::CommitAbility (not Super::Super) so the CostBypass.Active
	// bypass (Luna Overdrive hook) is respected.
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AActor* AvatarActor = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	UAbilitySystemComponent* SourceASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;

	if (!AvatarActor || !SourceASC || !AvatarActor->GetWorld())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Cast motion plays on cast regardless of hit/whiff (unlike Tension, which is hit-only).
	AMoonCharacterBase* MoonCharacter = Cast<AMoonCharacterBase>(AvatarActor);
	if (MoonCharacter && CastAnim)
	{
		MoonCharacter->PlayOneShotAnim(CastAnim);
	}

	// Smoke-test hit detection only: a simple forward sphere trace. Not final VFX/hit design.
	const FVector TraceStart = AvatarActor->GetActorLocation();
	const FVector TraceEnd = TraceStart + (AvatarActor->GetActorForwardVector() * TraceRange);

	FCollisionQueryParams QueryParams(FName(TEXT("BlackholeSpellTrace")), false, AvatarActor);

	TArray<FHitResult> HitResults;
	AvatarActor->GetWorld()->SweepMultiByObjectType(
		HitResults,
		TraceStart,
		TraceEnd,
		FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(TraceRadius),
		QueryParams
	);

	AActor* HitActor = nullptr;
	for (const FHitResult& HitResult : HitResults)
	{
		AActor* Candidate = HitResult.GetActor();
		if (Candidate && Cast<IAbilitySystemInterface>(Candidate))
		{
			HitActor = Candidate;
			break;
		}
	}

	if (HitActor)
	{
		if (IAbilitySystemInterface* TargetASI = Cast<IAbilitySystemInterface>(HitActor))
		{
			if (UAbilitySystemComponent* TargetASC = TargetASI->GetAbilitySystemComponent())
			{
				// Health/Damage Core Rule 1: apply damage through the single shared GE, never by
				// writing to the Health attribute directly.
				FGameplayEffectSpecHandle DamageSpecHandle = MakeOutgoingGameplayEffectSpec(UGE_Damage_Instant::StaticClass());
				if (DamageSpecHandle.IsValid())
				{
					DamageSpecHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Damage")), -PlaceholderDamage);
					SourceASC->ApplyGameplayEffectSpecToTarget(*DamageSpecHandle.Data.Get(), TargetASC);
				}

				// Tension accrues on OnSpellHit, not OnSpellCast (spell-casting-base.md) — a whiff
				// grants no Tension, so this call only happens inside the bHit branch.
				if (MoonCharacter)
				{
					MoonCharacter->AddTensionFromSpellHit(ManaCostForTension);
				}

				if (ATargetDummy* Dummy = Cast<ATargetDummy>(HitActor))
				{
					Dummy->ApplyBlackholeSetup();
				}
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Display, TEXT("[MoonSignatureChain] Blackhole found no GAS target"));
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
