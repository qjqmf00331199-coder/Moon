#include "MoonCameraComponent.h"
#include "../Character/MoonCharacterBase.h"

UMoonCameraComponent::UMoonCameraComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void UMoonCameraComponent::GetCameraView(float DeltaTime, FMinimalViewInfo& DesiredView)
{
	Super::GetCameraView(DeltaTime, DesiredView);

	if (NearClipPlaneOverride > 0.0f)
	{
		DesiredView.PerspectiveNearClipPlane = NearClipPlaneOverride;
	}
}

void UMoonCameraComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (AMoonCharacterBase* Character = Cast<AMoonCharacterBase>(GetOwner()))
	{
		Character->UpdateCameraCornerDither(DeltaTime);
	}
}
