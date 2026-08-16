#include "Interaction/AresInteractionComponent.h"

#include "Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

#include "Interaction/AresInteractable.h"

UAresInteractionComponent::UAresInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// The prompt only needs to be right by the time the frame is drawn, so this
	// runs in the normal tick group rather than pre-physics.
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

bool UAresInteractionComponent::GetViewPoint(FVector& OutLocation, FVector& OutForward) const
{
	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return false;
	}

	// Found by class rather than by asking for AAresCharacter, so this component
	// works on any pawn with a camera — including the ship-interior pawns in M4.
	if (const UCameraComponent* Camera = Owner->FindComponentByClass<UCameraComponent>())
	{
		OutLocation = Camera->GetComponentLocation();
		OutForward = Camera->GetForwardVector();
		return true;
	}

	return false;
}

void UAresInteractionComponent::SetFocus(AActor* NewFocus, const FText& NewPrompt)
{
	if (FocusedActor.Get() == NewFocus && CurrentPrompt.EqualTo(NewPrompt))
	{
		return;
	}

	FocusedActor = NewFocus;
	CurrentPrompt = NewPrompt;
	OnFocusChanged.Broadcast(NewFocus, CurrentPrompt);
}

void UAresInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	FVector Start;
	FVector Forward;
	if (!GetViewPoint(Start, Forward))
	{
		SetFocus(nullptr, FText::GetEmpty());
		return;
	}

	const FVector End = Start + Forward * Reach;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(AresInteraction), false, GetOwner());
	FHitResult Hit;

	// Sphere sweep rather than a bare line: aiming at a console edge or a
	// doorknob should not demand pixel accuracy.
	const bool bHit = GetWorld()->SweepSingleByChannel(
		Hit, Start, End, FQuat::Identity, ECC_Visibility,
		FCollisionShape::MakeSphere(TraceRadius), Params);

	if (bDrawDebug)
	{
		DrawDebugLine(GetWorld(), Start, End, bHit ? FColor::Green : FColor::Red, false, -1.0f, 0, 0.5f);
	}

	AActor* HitActor = bHit ? Hit.GetActor() : nullptr;
	if (!HitActor || !HitActor->Implements<UAresInteractable>())
	{
		SetFocus(nullptr, FText::GetEmpty());
		return;
	}

	const IAresInteractable* Interactable = Cast<IAresInteractable>(HitActor);
	if (!Interactable)
	{
		SetFocus(nullptr, FText::GetEmpty());
		return;
	}

	// An interactable may demand a shorter reach than the default.
	const float Override = Interactable->GetInteractionReachOverride();
	if (Override > 0.0f && Hit.Distance > Override)
	{
		SetFocus(nullptr, FText::GetEmpty());
		return;
	}

	if (!Interactable->CanInteract(GetOwner()))
	{
		SetFocus(nullptr, FText::GetEmpty());
		return;
	}

	// The "[E] " prefix is applied here, once, so every prompt in the game
	// reads identically no matter which actor supplied the label.
	const FText Prompt = FText::Format(
		NSLOCTEXT("Ares", "InteractPrompt", "[E] {0}"), Interactable->GetInteractionLabel());

	SetFocus(HitActor, Prompt);
}

void UAresInteractionComponent::TryInteract()
{
	AActor* Target = FocusedActor.Get();
	if (!Target)
	{
		return;
	}

	if (IAresInteractable* Interactable = Cast<IAresInteractable>(Target))
	{
		if (Interactable->CanInteract(GetOwner()))
		{
			Interactable->Interact(GetOwner());
		}
	}
}
