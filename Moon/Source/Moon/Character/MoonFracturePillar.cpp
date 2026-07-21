#include "MoonFracturePillar.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

AMoonFracturePillar::AMoonFracturePillar()
{
	PrimaryActorTick.bCanEverTick = false;

	PillarMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PillarMesh"));
	PillarMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	// Spell target acquisition currently sweeps the Pawn channel. The pillar is resolved as
	// a secondary environmental consequence after a target hit, so it must not occlude that hit.
	PillarMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	PillarMesh->SetRelativeScale3D(FVector(0.65f, 0.65f, 2.6f));
	SetRootComponent(PillarMesh);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshFinder.Succeeded())
	{
		PillarMesh->SetStaticMesh(CubeMeshFinder.Object);
	}

	FractureLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FractureLight"));
	FractureLight->SetupAttachment(PillarMesh);
	FractureLight->SetRelativeLocation(FVector(0.0f, 0.0f, 65.0f));
	FractureLight->SetLightColor(FLinearColor(0.08f, 0.42f, 1.0f));
	FractureLight->SetIntensity(4200.0f);
	FractureLight->SetAttenuationRadius(320.0f);
}

bool AMoonFracturePillar::Fracture()
{
	if (bFractured)
	{
		return false;
	}

	bFractured = true;
	PillarMesh->SetVisibility(false, true);
	PillarMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FractureLight->SetLightColor(FLinearColor(1.0f, 0.04f, 0.01f));
	FractureLight->SetIntensity(14000.0f);
	FractureLight->SetVisibility(false, true);
	UE_LOG(LogTemp, Display, TEXT("[MoonSignatureChain] Environment fractured: %s"), *GetName());
	return true;
}
