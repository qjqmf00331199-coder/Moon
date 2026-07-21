#include "MoonGameplayAbility_Spell_Fire.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "EngineUtils.h"
#include "Effects/GE_Damage_Instant.h"
#include "Effects/GE_SpellCooldown_Fire.h"
#include "Effects/GE_SpellCost_Fire.h"
#include "../Character/MoonCharacterBase.h"
#include "../Character/MoonFracturePillar.h"
#include "../Character/TargetDummy.h"

UMoonGameplayAbility_Spell_Fire::UMoonGameplayAbility_Spell_Fire()
{
	SetAssetTags(FGameplayTagContainer(FGameplayTag::RequestGameplayTag(FName("Spell.Element.Fire"))));
	CostGameplayEffectClass = UGE_SpellCost_Fire::StaticClass();
	CooldownGameplayEffectClass = UGE_SpellCooldown_Fire::StaticClass();
}

void UMoonGameplayAbility_Spell_Fire::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	UAbilitySystemComponent* SourceASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!Avatar || !SourceASC || !Avatar->GetWorld())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const FVector Start = Avatar->GetActorLocation();
	const FVector End = Start + Avatar->GetActorForwardVector() * TraceRange;
	FCollisionQueryParams QueryParams(FName(TEXT("FireSpellTrace")), false, Avatar);
	TArray<FHitResult> Hits;
	Avatar->GetWorld()->SweepMultiByObjectType(
		Hits, Start, End, FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(TraceRadius), QueryParams);

	AActor* HitActor = nullptr;
	for (const FHitResult& Hit : Hits)
	{
		AActor* Candidate = Hit.GetActor();
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
				FGameplayEffectSpecHandle DamageSpec = MakeOutgoingGameplayEffectSpec(UGE_Damage_Instant::StaticClass());
				if (DamageSpec.IsValid())
				{
					DamageSpec.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Damage")), -Damage);
					SourceASC->ApplyGameplayEffectSpecToTarget(*DamageSpec.Data.Get(), TargetASC);
				}
				if (AMoonCharacterBase* Character = Cast<AMoonCharacterBase>(Avatar))
				{
					Character->AddTensionFromSpellHit(ManaCostForTension);
				}
			}
		}

		if (ATargetDummy* Dummy = Cast<ATargetDummy>(HitActor))
		{
			AMoonFracturePillar* Nearest = nullptr;
			float NearestDistanceSquared = FMath::Square(PillarSearchRadius);
			for (TActorIterator<AMoonFracturePillar> It(Avatar->GetWorld()); It; ++It)
			{
				const float DistanceSquared = FVector::DistSquared(It->GetActorLocation(), Dummy->GetActorLocation());
				if (!It->IsFractured() && DistanceSquared <= NearestDistanceSquared)
				{
					NearestDistanceSquared = DistanceSquared;
					Nearest = *It;
				}
			}
			Dummy->ApplyFireDetonation(Nearest);
		}
	}
	else
	{
		UE_LOG(LogTemp, Display, TEXT("[MoonSignatureChain] Fire found no GAS target"));
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
