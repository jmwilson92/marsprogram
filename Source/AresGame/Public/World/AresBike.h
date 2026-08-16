// A bicycle. Genuinely a NASA thing — Kennedy has had bikes since Apollo.
//
// Not a vehicle simulation, and deliberately so. Brief §5 says the player never
// pilots anything; a bike that needed steering and balance would be the thin end
// of that wedge. This is a movement profile with a frame attached to it: mount,
// go faster, dismount. The 650 m to the pad stops being a chore without becoming
// a driving game.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Interaction/AresInteractable.h"

#include "AresBike.generated.h"

class AAresCharacter;
class UInstancedStaticMeshComponent;

UCLASS()
class ARESGAME_API AAresBike : public AActor, public IAresInteractable
{
	GENERATED_BODY()

public:
	AAresBike();

	// --- IAresInteractable ---
	virtual FText GetInteractionLabel() const override;
	virtual bool CanInteract(AActor* Interactor) const override;
	virtual void Interact(AActor* Interactor) override;

	/** Puts the rider back on their feet and leaves the bike where they stopped. */
	UFUNCTION(BlueprintCallable, Category = "Ares|Bike")
	void Dismount();

	UFUNCTION(BlueprintPure, Category = "Ares|Bike")
	bool IsRidden() const { return Rider != nullptr; }

protected:
	void Mount(AAresCharacter* Character);

	/** Frame, wheels and bars, as one instanced component. */
	UPROPERTY(VisibleAnywhere, Category = "Ares|Bike")
	TObjectPtr<UInstancedStaticMeshComponent> Parts;

	UPROPERTY(EditAnywhere, Category = "Ares|Bike")
	float RideSpeed = 900.0f;

	/** Standing on the pedals. Still nowhere near a vehicle. */
	UPROPERTY(EditAnywhere, Category = "Ares|Bike")
	float RideSprintSpeed = 1500.0f;

	UPROPERTY(Transient)
	TObjectPtr<AAresCharacter> Rider;

private:
	void BuildFrame();
};
