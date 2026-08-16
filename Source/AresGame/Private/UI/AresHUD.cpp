#include "UI/AresHUD.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "GameFramework/Pawn.h"

#include "Interaction/AresInteractionComponent.h"

void AAresHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas)
	{
		return;
	}

	const float CentreX = Canvas->SizeX * 0.5f;
	const float CentreY = Canvas->SizeY * 0.5f;

	// --- Crosshair ---------------------------------------------------------
	// A plain cross. It marks where the interaction trace originates, so the
	// player can tell what they are about to use.
	DrawRect(CrosshairColor, CentreX - CrosshairSize, CentreY - 0.5f, CrosshairSize * 2.0f, 1.0f);
	DrawRect(CrosshairColor, CentreX - 0.5f, CentreY - CrosshairSize, 1.0f, CrosshairSize * 2.0f);

	// --- Interaction prompt ------------------------------------------------
	const APawn* OwningPawn = GetOwningPawn();
	if (!OwningPawn)
	{
		return;
	}

	const UAresInteractionComponent* Interaction =
		OwningPawn->FindComponentByClass<UAresInteractionComponent>();
	if (!Interaction)
	{
		return;
	}

	const FText Prompt = Interaction->GetCurrentPrompt();
	if (Prompt.IsEmpty())
	{
		return;
	}

	UFont* Font = GEngine ? GEngine->GetMediumFont() : nullptr;
	if (!Font)
	{
		return;
	}

	const FString PromptString = Prompt.ToString();

	float TextWidth = 0.0f;
	float TextHeight = 0.0f;
	GetTextSize(PromptString, TextWidth, TextHeight, Font, 1.0f);

	const float TextX = CentreX - TextWidth * 0.5f;
	const float TextY = CentreY + PromptOffsetY;

	// A dim plate behind the text so the prompt stays legible against a bright
	// sky or a white wall.
	DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.45f),
		TextX - 10.0f, TextY - 4.0f, TextWidth + 20.0f, TextHeight + 8.0f);

	DrawText(PromptString, PromptColor, TextX, TextY, Font, 1.0f, false);
}
