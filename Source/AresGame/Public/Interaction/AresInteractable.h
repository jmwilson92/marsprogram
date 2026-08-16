// Anything the player can look at and press E on.
//
// Brief §4.2: "an UInteractionComponent on the player, IInteractable on
// terminals/doors/elevators, one prompt line ('[E] Flight Director's console')".
//
// Kept deliberately small. An interactable answers three questions: what should
// the prompt say, is it available right now, and what happens on use. Anything
// richer than that belongs in the implementing actor, not in this contract.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "AresInteractable.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UAresInteractable : public UInterface
{
	GENERATED_BODY()
};

class ARESGAME_API IAresInteractable
{
	GENERATED_BODY()

public:
	/**
	 * The noun shown after the key hint — "Flight Director's console",
	 * "VAB personnel door". The component supplies the "[E] " prefix, so every
	 * prompt in the game reads the same way.
	 */
	virtual FText GetInteractionLabel() const { return FText::GetEmpty(); }

	/**
	 * False hides the prompt entirely. Use for a terminal that is powered down
	 * or a door that is locked out during a launch hold.
	 */
	virtual bool CanInteract(AActor* Interactor) const { return true; }

	/** Called once per E press. */
	virtual void Interact(AActor* Interactor) {}

	/**
	 * Optional: how close the player must be, in cm. Zero means use the
	 * interaction component's default reach.
	 */
	virtual float GetInteractionReachOverride() const { return 0.0f; }
};
