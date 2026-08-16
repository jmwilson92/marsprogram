#include "World/AresTerminal.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"
#include "UObject/ConstructorHelpers.h"

#include "Player/AresPlayerController.h"

#define LOCTEXT_NAMESPACE "AresTerminal"

namespace
{
const TCHAR* CubePath = TEXT("/Engine/BasicShapes/Cube.Cube");
}

AAresTerminal::AAresTerminal()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(CubePath);

	Desk = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Desk"));
	Desk->SetupAttachment(Root);
	if (CubeMesh.Succeeded())
	{
		Desk->SetStaticMesh(CubeMesh.Object);
	}
	// 160 x 70 x 95 cm — a standing console.
	Desk->SetRelativeLocation(FVector(0.0f, 0.0f, 47.5f));
	Desk->SetRelativeScale3D(FVector(0.7f, 1.6f, 0.95f));
	Desk->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Desk->SetCollisionResponseToAllChannels(ECR_Block);

	Screen = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Screen"));
	Screen->SetupAttachment(Root);
	if (CubeMesh.Succeeded())
	{
		Screen->SetStaticMesh(CubeMesh.Object);
	}
	// Raked back 20 degrees so it reads as a screen rather than a slab.
	Screen->SetRelativeLocation(FVector(-5.0f, 0.0f, 115.0f));
	Screen->SetRelativeRotation(FRotator(-20.0f, 0.0f, 0.0f));
	Screen->SetRelativeScale3D(FVector(0.06f, 1.4f, 0.5f));
	Screen->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Screen->SetCollisionResponseToAllChannels(ECR_Block);
}

FText AAresTerminal::GetInteractionLabel() const
{
	if (!LabelOverride.IsEmpty())
	{
		return LabelOverride;
	}

	switch (Kind)
	{
	case ETerminalKind::VehicleAssembly:
		return LOCTEXT("VabPrompt", "Vehicle configuration console");
	case ETerminalKind::MissionControl:
		return LOCTEXT("MccPrompt", "Flight Director's console");
	case ETerminalKind::Research:
		return LOCTEXT("RndPrompt", "Research console");
	case ETerminalKind::Administration:
		return LOCTEXT("HqPrompt", "Program administration console");
	default:
		return LOCTEXT("GenericPrompt", "Console");
	}
}

void AAresTerminal::Interact(AActor* Interactor)
{
	const APawn* Pawn = Cast<APawn>(Interactor);
	if (!Pawn)
	{
		return;
	}

	if (AAresPlayerController* PC = Cast<AAresPlayerController>(Pawn->GetController()))
	{
		PC->ShowTerminal(Kind);
	}
}

#undef LOCTEXT_NAMESPACE
