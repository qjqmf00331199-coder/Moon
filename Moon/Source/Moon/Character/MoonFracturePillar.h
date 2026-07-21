#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MoonFracturePillar.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;

/**
 * Disposable engine-spike environment target for the signature combat chain.
 * Fire can fracture it only as part of a Blackhole-prepared target interaction.
 */
UCLASS()
class MOON_API AMoonFracturePillar : public AActor
{
	GENERATED_BODY()

public:
	AMoonFracturePillar();

	bool Fracture();
	bool IsFractured() const { return bFractured; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Moon|Signature Chain|Spike")
	TObjectPtr<UStaticMeshComponent> PillarMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Moon|Signature Chain|Spike")
	TObjectPtr<UPointLightComponent> FractureLight;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Moon|Signature Chain|Spike")
	bool bFractured = false;
};
