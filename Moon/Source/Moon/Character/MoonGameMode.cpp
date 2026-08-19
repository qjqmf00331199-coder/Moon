#include "MoonGameMode.h"
#include "MoonPlayerController.h"

AMoonGameMode::AMoonGameMode()
{
	PlayerControllerClass = AMoonPlayerController::StaticClass();
}
