// The full-screen screen you get when you press E on a console.
//
// Brief §4.2: "terminals open a full-screen UMG widget with the player still in
// the world". The player is not teleported to a menu — the game keeps running
// behind the panel, which is why the player controller uses GameAndUI input
// rather than UI-only.
//
// Built in C++ rather than as a Widget Blueprint. A .uasset widget cannot be
// reviewed in a diff, and every terminal in this game is a view onto simulation
// state that also lives in code. M1 ships the frame and the identity of each
// screen; M2 fills them with live FProgramState data.

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"

#include "AresTerminalWidget.generated.h"

class UBorder;
class UButton;
class UTextBlock;
class UVerticalBox;

/** Which room's console this is. Brief §4.2 gives each building exactly one. */
UENUM(BlueprintType)
enum class ETerminalKind : uint8
{
	/** VAB — payload budget, cargo selection, variant, ship assignment. */
	VehicleAssembly,
	/** Mission Control — timeline, telemetry, launch commit, timewarp. */
	MissionControl,
	/** Research Center — tech tree, RP spend, maturity. */
	Research,
	/** Administration — budget, appropriations, contracts, support, press. */
	Administration,
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTerminalCloseRequested);

UCLASS()
class ARESUI_API UAresTerminalWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Sets the screen's identity. Safe to call before or after construction. */
	UFUNCTION(BlueprintCallable, Category = "Ares|Terminal")
	void SetTerminalKind(ETerminalKind InKind);

	/** Raised by the CLOSE button. The player controller listens. */
	UPROPERTY(BlueprintAssignable, Category = "Ares|Terminal")
	FOnTerminalCloseRequested OnCloseRequested;

	/** Headline for a kind, e.g. "FLIGHT DIRECTOR". */
	static FText GetTitleFor(ETerminalKind InKind);

	/** One line describing what the screen will eventually do. */
	static FText GetSubtitleFor(ETerminalKind InKind);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UFUNCTION()
	void HandleCloseClicked();

	void ApplyKindToWidgets();

	UPROPERTY(Transient)
	TObjectPtr<UBorder> RootBorder;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SubtitleText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> BodyText;

	UPROPERTY(Transient)
	TObjectPtr<UButton> CloseButton;

	UPROPERTY(Transient)
	ETerminalKind Kind = ETerminalKind::MissionControl;
};
