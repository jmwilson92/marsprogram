// The player controller.
//
// Deliberately thin. It owns view input mode and the currently open terminal
// screen. It does NOT own program state; that lives in UProgramSubsystem on the
// GameInstance so it survives level travel (brief §3.2).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

#include "Terminals/AresTerminalWidget.h"

#include "AresPlayerController.generated.h"

class UAresTerminalWidget;

UCLASS()
class ARESGAME_API AAresPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AAresPlayerController();

	/**
	 * Opens a terminal screen. The player stays in the world behind it
	 * (brief §4.2), so the game keeps rendering and simulating.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ares|Terminal")
	void ShowTerminal(ETerminalKind Kind);

	UFUNCTION(BlueprintCallable, Category = "Ares|Terminal")
	void HideTerminal();

	UFUNCTION(BlueprintPure, Category = "Ares|Terminal")
	bool IsTerminalOpen() const { return ActiveTerminal != nullptr; }

	/** Captures the mouse for first-person look. */
	UFUNCTION(BlueprintCallable, Category = "Ares|Input")
	void EnterWalkingInputMode();

	/** Releases the mouse for a full-screen terminal widget. */
	UFUNCTION(BlueprintCallable, Category = "Ares|Input")
	void EnterTerminalInputMode();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void HandleTerminalCloseRequested();

	/** Widget class to instantiate. Overridable if a screen ever needs its own. */
	UPROPERTY(EditDefaultsOnly, Category = "Ares|Terminal")
	TSubclassOf<UAresTerminalWidget> TerminalWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UAresTerminalWidget> ActiveTerminal;
};
