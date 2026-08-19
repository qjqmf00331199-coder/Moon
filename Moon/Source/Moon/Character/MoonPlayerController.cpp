#include "MoonPlayerController.h"
#include "../Camera/MoonPlayerCameraManager.h"

AMoonPlayerController::AMoonPlayerController()
{
	PlayerCameraManagerClass = AMoonPlayerCameraManager::StaticClass();
}
