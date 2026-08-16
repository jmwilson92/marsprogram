#include "Core/AresGameModeBase.h"

#include "Player/AresCharacter.h"
#include "Player/AresPlayerController.h"
#include "UI/AresHUD.h"

AAresGameModeBase::AAresGameModeBase()
{
	DefaultPawnClass = AAresCharacter::StaticClass();
	PlayerControllerClass = AAresPlayerController::StaticClass();
	HUDClass = AAresHUD::StaticClass();
}
