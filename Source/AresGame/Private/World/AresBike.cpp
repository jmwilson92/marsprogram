#include "World/AresBike.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"

#include "Player/AresCharacter.h"

#define LOCTEXT_NAMESPACE "AresBike"

AAresBike::AAresBike()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Parts = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Parts"));
	Parts->SetupAttachment(Root);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Parts->SetStaticMesh(CubeMesh.Object);
	}

	// A parked bike should not be something you trip over, and a ridden one is
	// attached to the player — so it never needs to block anything.
	Parts->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Parts->SetCollisionResponseToAllChannels(ECR_Ignore);
	Parts->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	BuildFrame();
}

void AAresBike::BuildFrame()
{
	if (!Parts)
	{
		return;
	}

	auto Add = [this](const FVector& Center, const FVector& Size, float Pitch = 0.0f)
	{
		Parts->AddInstance(FTransform(FRotator(Pitch, 0.0f, 0.0f), Center, Size / 100.0f));
	};

	// Roughly a 1.7 m bike, wheels at each end, ridden facing +X.
	Add(FVector(52.0f, 0.0f, 34.0f), FVector(8.0f, 6.0f, 68.0f));    // front wheel
	Add(FVector(-52.0f, 0.0f, 34.0f), FVector(8.0f, 6.0f, 68.0f));   // rear wheel
	Add(FVector(0.0f, 0.0f, 58.0f), FVector(96.0f, 6.0f, 8.0f));     // top tube
	Add(FVector(-6.0f, 0.0f, 38.0f), FVector(80.0f, 6.0f, 6.0f), 12.0f); // down tube
	Add(FVector(48.0f, 0.0f, 82.0f), FVector(8.0f, 46.0f, 6.0f));    // handlebars
	Add(FVector(-40.0f, 0.0f, 76.0f), FVector(26.0f, 12.0f, 6.0f));  // saddle
}

FText AAresBike::GetInteractionLabel() const
{
	return LOCTEXT("RideBike", "Bike");
}

bool AAresBike::CanInteract(AActor* Interactor) const
{
	return Rider == nullptr;
}

void AAresBike::Interact(AActor* Interactor)
{
	if (Rider)
	{
		return;
	}
	if (AAresCharacter* Character = Cast<AAresCharacter>(Interactor))
	{
		Mount(Character);
	}
}

void AAresBike::Mount(AAresCharacter* Character)
{
	if (!Character || Rider)
	{
		return;
	}

	Rider = Character;

	// Ride it rather than carry it: the frame hangs under the camera so it is
	// visible in first person without filling the screen.
	AttachToActor(Character, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	SetActorRelativeLocation(FVector(10.0f, 0.0f, -78.0f));
	SetActorRelativeRotation(FRotator::ZeroRotator);

	Character->SetMovementProfile(RideSpeed, RideSprintSpeed);
	Character->SetRiddenBike(this);
}

void AAresBike::Dismount()
{
	if (!Rider)
	{
		return;
	}

	AAresCharacter* Character = Rider;
	Rider = nullptr;

	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	// Park it beside the rider, upright and on the ground, rather than leaving
	// it floating at camera height where they let go of it.
	const FVector Beside = Character->GetActorLocation()
		+ Character->GetActorRightVector() * 90.0f
		- FVector(0.0f, 0.0f, 88.0f);
	SetActorLocation(Beside);
	SetActorRotation(FRotator(0.0f, Character->GetActorRotation().Yaw, 0.0f));

	Character->ResetMovementProfile();
	Character->SetRiddenBike(nullptr);
}

#undef LOCTEXT_NAMESPACE
