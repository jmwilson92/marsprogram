// The player. First person, on foot, always.
//
// Brief §1: "You are physically present. You walk your space center on foot."
// And §5: the player is never a pilot. This pawn walks and looks; it never
// flies anything.
//
// M1 uses the stock UCharacterMovementComponent for a plain Earth-gravity
// walk. M4 replaces it with UAresMovementComponent, which adds the HighG /
// ZeroG / ThrustG / LowG modes driven by the flight director's phase (§6).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

#include "AresCharacter.generated.h"

class UAresInputConfig;
class UAresInteractionComponent;
class UCameraComponent;
struct FInputActionValue;

UCLASS()
class ARESGAME_API AAresCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AAresCharacter();

	virtual void BeginPlay() override;

	/** Eye-level camera. The interaction component traces from here. */
	UCameraComponent* GetFirstPersonCamera() const { return FirstPersonCamera; }

	UAresInteractionComponent* GetInteraction() const { return Interaction; }

protected:
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void SprintStart();
	void SprintStop();
	void Interact();

	/** Creates the input config if it does not exist yet. Idempotent. */
	void EnsureInputConfig();

	/** Registers the mapping context with the local player's input subsystem. */
	void ApplyMappingContext();

	UPROPERTY(VisibleAnywhere, Category = "Ares|Player")
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	UPROPERTY(Transient)
	TObjectPtr<UAresInputConfig> InputConfig;

	UPROPERTY(VisibleAnywhere, Category = "Ares|Player")
	TObjectPtr<UAresInteractionComponent> Interaction;

	/** Brisk walk, cm/s. The Cape is deliberately a place with distances. */
	UPROPERTY(EditDefaultsOnly, Category = "Ares|Player")
	float WalkSpeed = 400.0f;

	/** The pad is ~600 m out; sprint keeps blockout testing tolerable. */
	UPROPERTY(EditDefaultsOnly, Category = "Ares|Player")
	float SprintSpeed = 750.0f;

	/** Camera height above the capsule centre, cm. */
	UPROPERTY(EditDefaultsOnly, Category = "Ares|Player")
	float EyeHeight = 64.0f;
};
