// Finds what the player is looking at and reports a prompt for it.
//
// Lives on AAresCharacter. Traces forward from the first-person camera every
// frame, tracks the focused interactable, and raises a delegate when focus
// changes so the HUD can update without polling.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "AresInteractionComponent.generated.h"

class AActor;

/** Broadcast when the focused interactable changes. Empty text means nothing focused. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnInteractionFocusChanged, AActor*, FocusedActor, const FText&, Prompt);

UCLASS(ClassGroup = (Ares), meta = (BlueprintSpawnableComponent))
class ARESGAME_API UAresInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAresInteractionComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	/** Uses whatever is currently focused. Bound to IA_Interact. */
	UFUNCTION(BlueprintCallable, Category = "Ares|Interaction")
	void TryInteract();

	/** The actor under the crosshair, or null. */
	UFUNCTION(BlueprintPure, Category = "Ares|Interaction")
	AActor* GetFocusedActor() const { return FocusedActor.Get(); }

	/** Fully formatted, e.g. "[E] Flight Director's console". Empty if nothing focused. */
	UFUNCTION(BlueprintPure, Category = "Ares|Interaction")
	FText GetCurrentPrompt() const { return CurrentPrompt; }

	UPROPERTY(BlueprintAssignable, Category = "Ares|Interaction")
	FOnInteractionFocusChanged OnFocusChanged;

protected:
	/** Default reach in cm. An interactable may ask for less via its override. */
	UPROPERTY(EditAnywhere, Category = "Ares|Interaction")
	float Reach = 250.0f;

	/**
	 * Trace radius in cm. A small sphere rather than a bare line, so a doorknob
	 * or a console corner does not require pixel-accurate aim.
	 */
	UPROPERTY(EditAnywhere, Category = "Ares|Interaction")
	float TraceRadius = 8.0f;

	/** Draws the trace in-world. Set via `Ares.Interaction.Debug 1`. */
	UPROPERTY(EditAnywhere, Category = "Ares|Interaction")
	bool bDrawDebug = false;

private:
	/** Resolves the eye viewpoint from the owner's camera component. */
	bool GetViewPoint(FVector& OutLocation, FVector& OutForward) const;

	void SetFocus(AActor* NewFocus, const FText& NewPrompt);

	TWeakObjectPtr<AActor> FocusedActor;
	FText CurrentPrompt;
};
