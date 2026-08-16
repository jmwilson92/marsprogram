// A door. The first thing in the game you can walk up to and use.
//
// This is blockout, not art: an engine basic-shape cube scaled to a door and
// swung on a hinge. Brief §9 — "blockout geometry and engine-default materials
// until a milestone is functionally accepted". Its real job in M1 is proving
// the IAresInteractable / UAresInteractionComponent loop works, which is the
// same loop every terminal, elevator and airlock hatch will use.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Interaction/AresInteractable.h"

#include "AresDoor.generated.h"

class UStaticMeshComponent;

UCLASS()
class ARESGAME_API AAresDoor : public AActor, public IAresInteractable
{
	GENERATED_BODY()

public:
	AAresDoor();

	virtual void Tick(float DeltaSeconds) override;

	// --- IAresInteractable ---
	virtual FText GetInteractionLabel() const override;
	virtual bool CanInteract(AActor* Interactor) const override;
	virtual void Interact(AActor* Interactor) override;

protected:
	virtual void BeginPlay() override;

	/** Hinge. The panel is offset from this, so rotating it swings the door. */
	UPROPERTY(VisibleAnywhere, Category = "Ares|Door")
	TObjectPtr<USceneComponent> Hinge;

	UPROPERTY(VisibleAnywhere, Category = "Ares|Door")
	TObjectPtr<UStaticMeshComponent> Panel;

	/** Shown after the "[E] " prefix. */
	UPROPERTY(EditAnywhere, Category = "Ares|Door")
	FText Label = NSLOCTEXT("Ares", "DoorLabel", "Door");

	UPROPERTY(EditAnywhere, Category = "Ares|Door")
	float OpenAngleDegrees = 95.0f;

	UPROPERTY(EditAnywhere, Category = "Ares|Door")
	float SecondsToSwing = 1.1f;

	/** Locked doors show no prompt at all, rather than a prompt that fails. */
	UPROPERTY(EditAnywhere, Category = "Ares|Door")
	bool bLocked = false;

	UPROPERTY(EditAnywhere, Category = "Ares|Door")
	bool bStartsOpen = false;

private:
	/** 0 = closed, 1 = fully open. Interpolated in Tick. */
	float SwingAlpha = 0.0f;
	bool bWantsOpen = false;
};
