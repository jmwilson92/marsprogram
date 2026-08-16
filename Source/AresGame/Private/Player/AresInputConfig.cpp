#include "Player/AresInputConfig.h"

#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"

UInputAction* UAresInputConfig::MakeAction(FName Name, EInputActionValueType ValueType)
{
	UInputAction* Action = NewObject<UInputAction>(this, Name);
	Action->ValueType = ValueType;
	return Action;
}

void UAresInputConfig::BuildDefaults()
{
	if (MappingContext)
	{
		return;
	}

	MoveAction = MakeAction(TEXT("IA_Move"), EInputActionValueType::Axis2D);
	LookAction = MakeAction(TEXT("IA_Look"), EInputActionValueType::Axis2D);
	JumpAction = MakeAction(TEXT("IA_Jump"), EInputActionValueType::Boolean);
	SprintAction = MakeAction(TEXT("IA_Sprint"), EInputActionValueType::Boolean);
	InteractAction = MakeAction(TEXT("IA_Interact"), EInputActionValueType::Boolean);

	MappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_AresDefault"));

	// --- Movement -----------------------------------------------------------
	//
	// A single key produces a 1D value on X. To drive an Axis2D action, W and S
	// are swizzled so that value lands on Y (forward/back), while A and D stay
	// on X (strafe). Negate flips the sign for the "backwards" halves.
	//
	// Modifiers apply in array order, so Swizzle must precede Negate.
	{
		UInputModifierSwizzleAxis* SwizzleToY = NewObject<UInputModifierSwizzleAxis>(this);
		SwizzleToY->Order = EInputAxisSwizzle::YXZ;

		UInputModifierSwizzleAxis* SwizzleToYBack = NewObject<UInputModifierSwizzleAxis>(this);
		SwizzleToYBack->Order = EInputAxisSwizzle::YXZ;

		// W — forward
		FEnhancedActionKeyMapping& Forward = MappingContext->MapKey(MoveAction, EKeys::W);
		Forward.Modifiers.Add(SwizzleToY);

		// S — back
		FEnhancedActionKeyMapping& Back = MappingContext->MapKey(MoveAction, EKeys::S);
		Back.Modifiers.Add(SwizzleToYBack);
		Back.Modifiers.Add(NewObject<UInputModifierNegate>(this));

		// A — strafe left
		FEnhancedActionKeyMapping& Left = MappingContext->MapKey(MoveAction, EKeys::A);
		Left.Modifiers.Add(NewObject<UInputModifierNegate>(this));

		// D — strafe right, the unmodified case
		MappingContext->MapKey(MoveAction, EKeys::D);
	}

	// --- Look ---------------------------------------------------------------
	//
	// Mouse2D already delivers an Axis2D, so no modifier is needed. Pitch is
	// inverted in AAresCharacter::Look rather than by a Negate modifier, so the
	// sign convention is visible where it is used.
	MappingContext->MapKey(LookAction, EKeys::Mouse2D);

	// --- Actions ------------------------------------------------------------
	MappingContext->MapKey(JumpAction, EKeys::SpaceBar);
	MappingContext->MapKey(SprintAction, EKeys::LeftShift);
	MappingContext->MapKey(InteractAction, EKeys::E);
}
