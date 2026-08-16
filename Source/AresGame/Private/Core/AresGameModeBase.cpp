#include "Core/AresGameModeBase.h"

#include "Player/AresCharacter.h"
#include "Player/AresPlayerController.h"

AAresGameModeBase::AAresGameModeBase()
{
	DefaultPawnClass = AAresCharacter::StaticClass();
	PlayerControllerClass = AAresPlayerController::StaticClass();
}
