#include "Player/AresPlayerController.h"

AAresPlayerController::AAresPlayerController()
{
	bShowMouseCursor = false;
}

void AAresPlayerController::BeginPlay()
{
	Super::BeginPlay();
	EnterWalkingInputMode();
}

void AAresPlayerController::EnterWalkingInputMode()
{
	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());
}

void AAresPlayerController::EnterTerminalInputMode()
{
	bShowMouseCursor = true;

	// The player stays in the world behind the terminal (brief §4.2), so the
	// game keeps rendering and simulating; only input focus moves to the UI.
	FInputModeGameAndUI Mode;
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	Mode.SetHideCursorDuringCapture(false);
	SetInputMode(Mode);
}
