// A console you walk up to and use.
//
// One per building (brief §4.2). The actor is just a blockout desk plus an
// identity; the screen it opens lives in AresUI, and the input-mode juggling
// lives on the player controller. This keeps the actor free of UI knowledge
// beyond "which kind of terminal am I".

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Interaction/AresInteractable.h"
#include "Terminals/AresTerminalWidget.h"

#include "AresTerminal.generated.h"

class UPointLightComponent;
class UStaticMeshComponent;

UCLASS()
class ARESGAME_API AAresTerminal : public AActor, public IAresInteractable
{
	GENERATED_BODY()

public:
	AAresTerminal();

	// --- IAresInteractable ---
	virtual FText GetInteractionLabel() const override;
	virtual void Interact(AActor* Interactor) override;

	UFUNCTION(BlueprintPure, Category = "Ares|Terminal")
	ETerminalKind GetKind() const { return Kind; }

	UFUNCTION(BlueprintCallable, Category = "Ares|Terminal")
	void SetKind(ETerminalKind InKind) { Kind = InKind; }

protected:
	UPROPERTY(VisibleAnywhere, Category = "Ares|Terminal")
	TObjectPtr<UStaticMeshComponent> Desk;

	/** Angled panel, so the console reads as something you stand at. */
	UPROPERTY(VisibleAnywhere, Category = "Ares|Terminal")
	TObjectPtr<UStaticMeshComponent> Screen;

	/**
	 * Screen glow. A dark console in a large grey room is genuinely hard to
	 * find; a lit screen reads as "this is the thing you came here for" from
	 * across the floor. Cheap, and it is what a real console looks like.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Ares|Terminal")
	TObjectPtr<UPointLightComponent> ScreenGlow;

	UPROPERTY(EditAnywhere, Category = "Ares|Terminal")
	ETerminalKind Kind = ETerminalKind::MissionControl;

	/**
	 * Overrides the prompt noun. Empty uses the kind's default, so a level can
	 * say "Flight Director's console" where the generic name is too vague.
	 */
	UPROPERTY(EditAnywhere, Category = "Ares|Terminal")
	FText LabelOverride;
};
