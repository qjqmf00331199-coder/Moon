#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MoonGameMode.generated.h"

// AMoonGameMode (Story 002 closeout): without a GameMode setting PlayerControllerClass, the engine
// spawns the stock APlayerController, which spawns the stock APlayerCameraManager — AMoonPlayerCameraManager's
// pitch clamp (AMoonPlayerCameraManager::InitializeFor) never runs. This is the only reason this
// class exists; do not add gameplay logic here (this project's ability/attribute setup lives on
// AMoonCharacterBase, not the GameMode).
UCLASS()
class MOON_API AMoonGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMoonGameMode();
};
