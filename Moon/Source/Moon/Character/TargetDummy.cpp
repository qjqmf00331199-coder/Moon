#include "TargetDummy.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "AbilitySystemComponent.h"
#include "../GAS/MoonAbilitySystemComponent.h"
#include "../GAS/MoonAttributeSet.h"
#include "../GAS/Effects/GE_CoreExtractionReward.h"
#include "MoonCharacterBase.h"
#include "MoonFracturePillar.h"
#include "UObject/ConstructorHelpers.h"

ATargetDummy::ATargetDummy()
{
	PrimaryActorTick.bCanEverTick = false;

	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComponent"));
	CapsuleComponent->InitCapsuleSize(42.0f, 96.0f);
	// Plain AActor capsules default to "OverlapAllDynamic" (Pawn -> Overlap), unlike ACharacter's
	// auto-configured "Pawn" profile. SweepSingleByChannel (used by MoonGameplayAbility_Spell_
	// Blackhole's hit trace) only reports blocking hits, so without this the dummy is untargetable.
	CapsuleComponent->SetCollisionProfileName(TEXT("Pawn"));
	SetRootComponent(CapsuleComponent);

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(CapsuleComponent);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualMesh->SetRelativeScale3D(FVector(0.55f, 0.55f, 1.35f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMeshFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMeshFinder.Succeeded())
	{
		VisualMesh->SetStaticMesh(CylinderMeshFinder.Object);
	}

	StateLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("StateLight"));
	StateLight->SetupAttachment(CapsuleComponent);
	StateLight->SetRelativeLocation(FVector(0.0f, 0.0f, 115.0f));
	StateLight->SetIntensity(2800.0f);
	StateLight->SetAttenuationRadius(260.0f);
	StateLight->SetLightColor(FLinearColor(0.18f, 0.38f, 1.0f));

	// Solo/offline smoke-test actor: not replicated, mirrors AMoonCharacterBase's ASC setup.
	AbilitySystemComponent = CreateDefaultSubobject<UMoonAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(false);

	AttributeSet = CreateDefaultSubobject<UMoonAttributeSet>(TEXT("AttributeSet"));
}

void ATargetDummy::ApplyBlackholeSetup()
{
	if (SignatureChainState == EMoonSignatureChainState::Extracted)
	{
		return;
	}

	SignatureChainState = EMoonSignatureChainState::Gathered;
	StateLight->SetLightColor(FLinearColor(0.38f, 0.08f, 1.0f));
	StateLight->SetIntensity(5200.0f);
	UE_LOG(LogTemp, Display, TEXT("[MoonSignatureChain] Blackhole setup applied to %s"), *GetName());
}

bool ATargetDummy::ApplyFireDetonation(AMoonFracturePillar* Pillar)
{
	if (SignatureChainState != EMoonSignatureChainState::Gathered || !Pillar || !Pillar->Fracture())
	{
		return false;
	}

	SignatureChainState = EMoonSignatureChainState::Executable;
	StateLight->SetLightColor(FLinearColor(1.0f, 0.02f, 0.01f));
	StateLight->SetIntensity(9000.0f);
	UE_LOG(LogTemp, Display, TEXT("[MoonSignatureChain] Fire detonation armed execution on %s"), *GetName());
	return true;
}

bool ATargetDummy::TryExtractCore(AMoonCharacterBase* ExecutingCharacter)
{
	if (SignatureChainState != EMoonSignatureChainState::Executable || !ExecutingCharacter)
	{
		return false;
	}

	if (UAbilitySystemComponent* ExecutorASC = ExecutingCharacter->GetAbilitySystemComponent())
	{
		FGameplayEffectContextHandle Context = ExecutorASC->MakeEffectContext();
		Context.AddInstigator(ExecutingCharacter, ExecutingCharacter);
		FGameplayEffectSpecHandle RewardSpec = ExecutorASC->MakeOutgoingSpec(UGE_CoreExtractionReward::StaticClass(), 1.0f, Context);
		if (RewardSpec.IsValid())
		{
			RewardSpec.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.ManaRestore")), 50.0f);
			ExecutorASC->ApplyGameplayEffectSpecToSelf(*RewardSpec.Data.Get());
		}
	}

	ExecutingCharacter->AddTension(ExecutionTensionReward);
	SignatureChainState = EMoonSignatureChainState::Extracted;
	VisualMesh->SetVisibility(false, true);
	CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StateLight->SetVisibility(false);
	UE_LOG(LogTemp, Display, TEXT("[MoonSignatureChain] Core extracted from %s"), *GetName());
	return true;
}

UAbilitySystemComponent* ATargetDummy::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ATargetDummy::BeginPlay()
{
	Super::BeginPlay();

	if (AbilitySystemComponent)
	{
		// Owner and Avatar are both this actor (no controller/possession for a target dummy).
		AbilitySystemComponent->InitAbilityActorInfo(this, this);

		InitializeAttributes();
	}
}

void ATargetDummy::InitializeAttributes()
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
