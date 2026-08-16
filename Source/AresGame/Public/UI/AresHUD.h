// Minimal canvas HUD: a crosshair and the interaction prompt.
//
// Deliberately not UMG. M1's job is proving the interaction loop, and a canvas
// HUD needs no widget asset, so the whole thing stays reviewable as text. The
// real HUD — suit readouts, telemetry, terminal screens — is UMG in AresUI and
// arrives with the systems that need it.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"

#include "AresHUD.generated.h"

UCLASS()
class ARESGAME_API AAresHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

protected:
	/** Half-length of each crosshair arm, in pixels. */
	UPROPERTY(EditDefaultsOnly, Category = "Ares|HUD")
	float CrosshairSize = 3.0f;

	/** Prompt baseline offset below screen centre, in pixels. */
	UPROPERTY(EditDefaultsOnly, Category = "Ares|HUD")
	float PromptOffsetY = 90.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Ares|HUD")
	FLinearColor PromptColor = FLinearColor(0.85f, 0.92f, 1.0f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Ares|HUD")
	FLinearColor CrosshairColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.55f);
};
