#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MoonPlayerController.generated.h"

// AMoonPlayerController (Story 002): the only job of this class is pointing PlayerCameraManagerClass
// at AMoonPlayerCameraManager. The engine spawns a controller's camera manager in
// PostInitializeComponents, before Possess/PossessedBy — too late to assign it from
// AMoonCharacterBase::PossessedBy, so this has to live on the controller, set in the constructor.
//
// AMoonGameMode selects this controller. L_CombatTest's GM_MoonCombat Blueprint derives from that
// native GameMode so its map-specific DefaultPawnClass override is preserved while inheriting this
// PlayerControllerClass.
UCLASS()
class MOON_API AMoonPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AMoonPlayerController();
};
