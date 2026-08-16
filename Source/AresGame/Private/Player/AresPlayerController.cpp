#include "Player/AresPlayerController.h"

#include "Blueprint/UserWidget.h"

#include "Terminals/AresTerminalWidget.h"

AAresPlayerController::AAresPlayerController()
{
	bShowMouseCursor = false;
	TerminalWidgetClass = UAresTerminalWidget::StaticClass();
}

void AAresPlayerController::BeginPlay()
{
	Super::BeginPlay();
	EnterWalkingInputMode();
}

void AAresPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	HideTerminal();
	Super::EndPlay(EndPlayReason);
}

void AAresPlayerController::EnterWalkingInputMode()
{
	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());
}

void AAresPlayerController::EnterTerminalInputMode()
{
	bShowMouseCursor = true;

	// GameAndUI rather than UIOnly: the world behind the screen keeps running,
	// and the character still receives the interact key so E closes the panel.
	FInputModeGameAndUI Mode;
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	Mode.SetHideCursorDuringCapture(false);
	SetInputMode(Mode);
}

void AAresPlayerController::ShowTerminal(ETerminalKind Kind)
{
	if (ActiveTerminal)
	{
		// Already looking at a screen; retarget rather than stacking widgets.
		ActiveTerminal->SetTerminalKind(Kind);
		return;
	}

	if (!TerminalWidgetClass)
	{
		return;
	}

	ActiveTerminal = CreateWidget<UAresTerminalWidget>(this, TerminalWidgetClass);
	if (!ActiveTerminal)
	{
		return;
	}

	ActiveTerminal->SetTerminalKind(Kind);
	ActiveTerminal->OnCloseRequested.AddDynamic(this, &AAresPlayerController::HandleTerminalCloseRequested);
	ActiveTerminal->AddToViewport(10);

	EnterTerminalInputMode();
}

void AAresPlayerController::HideTerminal()
{
	if (!ActiveTerminal)
	{
		return;
	}

	ActiveTerminal->OnCloseRequested.RemoveDynamic(this, &AAresPlayerController::HandleTerminalCloseRequested);
	ActiveTerminal->RemoveFromParent();
	ActiveTerminal = nullptr;

	EnterWalkingInputMode();
}

void AAresPlayerController::HandleTerminalCloseRequested()
{
	HideTerminal();
}
