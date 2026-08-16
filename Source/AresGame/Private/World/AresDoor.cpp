#include "World/AresDoor.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
// Engine basic shapes ship with the editor and are not marketplace content, so
// using them for blockout is consistent with brief §2. The cube is 100 uu on a
// side with its pivot at the centre.
const TCHAR* BasicCubePath = TEXT("/Engine/BasicShapes/Cube.Cube");

constexpr float DoorWidthCm = 100.0f;
constexpr float DoorThicknessCm = 10.0f;
constexpr float DoorHeightCm = 220.0f;
} // namespace

AAresDoor::AAresDoor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false; // Only ticks while swinging.

	Hinge = CreateDefaultSubobject<USceneComponent>(TEXT("Hinge"));
	SetRootComponent(Hinge);

	Panel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Panel"));
	Panel->SetupAttachment(Hinge);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(BasicCubePath);
	if (CubeMesh.Succeeded())
	{
		Panel->SetStaticMesh(CubeMesh.Object);
	}

	// Offset the panel so its inboard edge sits on the hinge; rotating the
	// hinge then swings the door rather than spinning it about its middle.
	Panel->SetRelativeLocation(FVector(DoorWidthCm * 0.5f, 0.0f, DoorHeightCm * 0.5f));
	Panel->SetRelativeScale3D(FVector(
		DoorWidthCm / 100.0f,
		DoorThicknessCm / 100.0f,
		DoorHeightCm / 100.0f));

	Panel->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Panel->SetCollisionResponseToAllChannels(ECR_Block);
}

void AAresDoor::BeginPlay()
{
	Super::BeginPlay();

	bWantsOpen = bStartsOpen;
	SwingAlpha = bStartsOpen ? 1.0f : 0.0f;
	Hinge->SetRelativeRotation(FRotator(0.0f, SwingAlpha * OpenAngleDegrees, 0.0f));
}

FText AAresDoor::GetInteractionLabel() const
{
	// The prompt should describe the outcome, not the object's state, so the
	// player reads it as an instruction.
	return bWantsOpen
		? FText::Format(NSLOCTEXT("Ares", "CloseThing", "Close {0}"), Label)
		: FText::Format(NSLOCTEXT("Ares", "OpenThing", "Open {0}"), Label);
}

bool AAresDoor::CanInteract(AActor* Interactor) const
{
	return !bLocked;
}

void AAresDoor::Interact(AActor* Interactor)
{
	if (bLocked)
	{
		return;
	}

	bWantsOpen = !bWantsOpen;
	// Ticking is off at rest; a door that is not moving costs nothing.
	SetActorTickEnabled(true);
}

void AAresDoor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const float Target = bWantsOpen ? 1.0f : 0.0f;
	if (FMath::IsNearlyEqual(SwingAlpha, Target, 1e-4f))
	{
		SwingAlpha = Target;
		Hinge->SetRelativeRotation(FRotator(0.0f, SwingAlpha * OpenAngleDegrees, 0.0f));
		SetActorTickEnabled(false);
		return;
	}

	const float Step = (SecondsToSwing > KINDA_SMALL_NUMBER)
		? DeltaSeconds / SecondsToSwing
		: 1.0f;

	SwingAlpha = FMath::Clamp(
		SwingAlpha + (bWantsOpen ? Step : -Step), 0.0f, 1.0f);

	// Ease in/out so a heavy blast door does not read as a light switch.
	const float Eased = FMath::InterpEaseInOut(0.0f, 1.0f, SwingAlpha, 2.0f);
	Hinge->SetRelativeRotation(FRotator(0.0f, Eased * OpenAngleDegrees, 0.0f));
}
