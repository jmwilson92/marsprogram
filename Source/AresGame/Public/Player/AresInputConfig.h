// Enhanced Input configuration, built entirely in C++.
//
// The usual UE workflow puts Input Actions and the Input Mapping Context in
// .uasset files authored in the editor. This project builds them in code
// instead, for two reasons:
//
//  1. Binary assets cannot be reviewed in a diff. An input binding is a design
//     decision, and design decisions belong in version control as text.
//  2. Brief §2 forbids marketplace content and favours things we generate. The
//     same argument applies to the blockout geometry (see ACapeCampus).
//
// Nothing here is engine-magic: UInputAction and UInputMappingContext are
// plain UObjects, so NewObject builds them the same way the asset loader would.

#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "UObject/Object.h"

#include "AresInputConfig.generated.h"

class UInputAction;
class UInputMappingContext;

/**
 * Owns the player's input actions and the mapping context that binds them to
 * keys. Created on demand by AAresCharacter; there is exactly one per pawn.
 */
UCLASS()
class ARESGAME_API UAresInputConfig : public UObject
{
	GENERATED_BODY()

public:
	/** Builds every action and the mapping context. Safe to call once per instance. */
	void BuildDefaults();

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> MappingContext;

	/** Axis2D. WASD, swizzled so W/S drive Y and A/D drive X. */
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> MoveAction;

	/** Axis2D. Mouse XY. Pitch is negated in the handler, not by a modifier. */
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> JumpAction;

	/** Held. The Cape is ~600 m end to end; walking all of it is a chore. */
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> SprintAction;

	/**
	 * Bound now, used in slice 2 by UInteractionComponent. Declared here so the
	 * key map is complete in one place rather than accreting across milestones.
	 */
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> InteractAction;

private:
	UInputAction* MakeAction(FName Name, EInputActionValueType ValueType);
};
