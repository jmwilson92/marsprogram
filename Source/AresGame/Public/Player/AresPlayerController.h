// The player controller.
//
// Deliberately thin. It owns view input mode only — mouse captured while
// walking, released while a terminal is open (slice 3). It does NOT own
// program state; that lives in UProgramSubsystem on the GameInstance so it
// survives level travel (brief §3.2).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

#include "AresPlayerController.generated.h"

UCLASS()
class ARESGAME_API AAresPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AAresPlayerController();

	/**
	 * Captures the mouse for first-person look. Slice 3 calls the UI variant
	 * when a terminal opens, so the cursor can be used on the screen.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ares|Input")
	void EnterWalkingInputMode();

	/** Releases the mouse for a full-screen terminal widget. */
	UFUNCTION(BlueprintCallable, Category = "Ares|Input")
	void EnterTerminalInputMode();

protected:
	virtual void BeginPlay() override;
};
