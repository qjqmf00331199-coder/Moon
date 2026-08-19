#include "MoonCameraComponent.h"

void UMoonCameraComponent::GetCameraView(float DeltaTime, FMinimalViewInfo& DesiredView)
{
	Super::GetCameraView(DeltaTime, DesiredView);

	if (NearClipPlaneOverride > 0.0f)
	{
		DesiredView.PerspectiveNearClipPlane = NearClipPlaneOverride;
	}
}
