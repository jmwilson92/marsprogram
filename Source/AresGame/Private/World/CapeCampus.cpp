#include "World/CapeCampus.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

#include "World/AresTerminal.h"

namespace
{
// Engine basic shapes: editor content, not marketplace content (brief §2).
// Both are 100 uu across with the pivot at the centre, so an instance scale of
// S/100 produces a box or cylinder S centimetres wide.
const TCHAR* CubePath = TEXT("/Engine/BasicShapes/Cube.Cube");
const TCHAR* CylinderPath = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");

constexpr float UnitCm = 100.0f;
} // namespace

ACapeCampus::ACapeCampus()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Boxes = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Boxes"));
	Boxes->SetupAttachment(Root);
	Boxes->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Boxes->SetCollisionResponseToAllChannels(ECR_Block);
	Boxes->SetMobility(EComponentMobility::Static);

	Cylinders = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Cylinders"));
	Cylinders->SetupAttachment(Root);
	Cylinders->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Cylinders->SetCollisionResponseToAllChannels(ECR_Block);
	Cylinders->SetMobility(EComponentMobility::Static);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(CubePath);
	if (CubeMesh.Succeeded())
	{
		Boxes->SetStaticMesh(CubeMesh.Object);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(CylinderPath);
	if (CylinderMesh.Succeeded())
	{
		Cylinders->SetStaticMesh(CylinderMesh.Object);
	}

	// --- Default layout -----------------------------------------------------
	//
	// Four buildings around a plaza at the origin, each with its doorway facing
	// the plaza. Distances are chosen so crossing the campus takes roughly 90
	// seconds at the character's 400 cm/s walk (brief §4.2), with the pad a
	// further 650 m out along +X.

	{
		// VAB — by far the largest volume. Open-topped so the interior reads as
		// the cavern it is; a Starship stack lives here from M3.
		FCapeBuilding Vab;
		Vab.Id = TEXT("VAB");
		Vab.DisplayName = NSLOCTEXT("Ares", "VAB", "VEHICLE ASSEMBLY BUILDING");
		Vab.Center = FVector(9000.0f, -8000.0f, 0.0f);
		Vab.Size = FVector(8000.0f, 6000.0f, 9000.0f);
		Vab.DoorWidth = 2400.0f;
		Vab.DoorHeight = 4000.0f;
		Vab.DoorSide = ECapeDoorSide::MinusY;
		Vab.WallThickness = 100.0f;
		Vab.bRoof = false;
		Vab.TerminalKind = ETerminalKind::VehicleAssembly;
		Buildings.Add(Vab);
	}
	{
		// Mission Control — the room you watch flights from.
		FCapeBuilding Mcc;
		Mcc.Id = TEXT("MCC");
		Mcc.DisplayName = NSLOCTEXT("Ares", "MCC", "MISSION CONTROL");
		Mcc.Center = FVector(-8000.0f, -7000.0f, 0.0f);
		Mcc.Size = FVector(5000.0f, 4000.0f, 1600.0f);
		Mcc.DoorSide = ECapeDoorSide::PlusY;
		Mcc.TerminalKind = ETerminalKind::MissionControl;
		Buildings.Add(Mcc);
	}
	{
		// Research Center — tech tree wall, labs, clean room.
		FCapeBuilding Research;
		Research.Id = TEXT("RND");
		Research.DisplayName = NSLOCTEXT("Ares", "RND", "RESEARCH CENTER");
		Research.Center = FVector(-8000.0f, 7000.0f, 0.0f);
		Research.Size = FVector(5000.0f, 4000.0f, 1400.0f);
		Research.DoorSide = ECapeDoorSide::MinusY;
		Research.TerminalKind = ETerminalKind::Research;
		Buildings.Add(Research);
	}
	{
		// Administration — budget, appropriations, the hearing room.
		FCapeBuilding Hq;
		Hq.Id = TEXT("HQ");
		Hq.DisplayName = NSLOCTEXT("Ares", "HQ", "ADMINISTRATION");
		Hq.Center = FVector(9000.0f, 8000.0f, 0.0f);
		Hq.Size = FVector(4500.0f, 3500.0f, 1800.0f);
		Hq.DoorSide = ECapeDoorSide::MinusX;
		Hq.TerminalKind = ETerminalKind::Administration;
		Buildings.Add(Hq);
	}
}

void ACapeCampus::AddBox(const FVector& Center, const FVector& Size)
{
	if (!Boxes || Size.X <= 0.0f || Size.Y <= 0.0f || Size.Z <= 0.0f)
	{
		return;
	}

	const FTransform Xf(FRotator::ZeroRotator, Center, Size / UnitCm);
	Boxes->AddInstance(Xf);
}

void ACapeCampus::AddCylinder(const FVector& BaseCenter, float DiameterCm, float HeightCm)
{
	if (!Cylinders || DiameterCm <= 0.0f || HeightCm <= 0.0f)
	{
		return;
	}

	// The basic cylinder's pivot is at its centre, so lift it by half its height
	// to sit the base on BaseCenter.
	const FVector Center = BaseCenter + FVector(0.0f, 0.0f, HeightCm * 0.5f);
	const FTransform Xf(FRotator::ZeroRotator, Center,
		FVector(DiameterCm / UnitCm, DiameterCm / UnitCm, HeightCm / UnitCm));
	Cylinders->AddInstance(Xf);
}

void ACapeCampus::AddWall(const FVector& Center, const FVector& Size, bool bAlongY,
	bool bWithDoor, float DoorWidth, float DoorHeight)
{
	if (!bWithDoor)
	{
		AddBox(Center, Size);
		return;
	}

	// The doorway is two jambs and a lintel. Cheaper and more robust than any
	// kind of boolean, and it keeps every piece an axis-aligned box.
	const float SpanLength = bAlongY ? Size.Y : Size.X;
	const float Clear = FMath::Min(DoorWidth, SpanLength - 200.0f);
	const float JambLength = FMath::Max((SpanLength - Clear) * 0.5f, 0.0f);
	const float Head = FMath::Min(DoorHeight, Size.Z);

	if (JambLength > 0.0f)
	{
		const float Offset = (SpanLength - JambLength) * 0.5f;
		const FVector Axis = bAlongY ? FVector(0, 1, 0) : FVector(1, 0, 0);
		const FVector JambSize = bAlongY
			? FVector(Size.X, JambLength, Size.Z)
			: FVector(JambLength, Size.Y, Size.Z);

		AddBox(Center + Axis * Offset, JambSize);
		AddBox(Center - Axis * Offset, JambSize);
	}

	// Lintel above the opening.
	const float LintelHeight = Size.Z - Head;
	if (LintelHeight > 0.0f)
	{
		const FVector LintelSize = bAlongY
			? FVector(Size.X, Clear, LintelHeight)
			: FVector(Clear, Size.Y, LintelHeight);
		const FVector LintelCenter = Center + FVector(0.0f, 0.0f, (Size.Z - LintelHeight) * 0.5f);
		AddBox(LintelCenter, LintelSize);
	}
}

void ACapeCampus::BuildBuilding(const FCapeBuilding& Building)
{
	const FVector& C = Building.Center;
	const FVector& S = Building.Size;
	const float T = Building.WallThickness;

	// Floor slab, sitting just under z=0 so the interior floor is walkable.
	AddBox(C + FVector(0.0f, 0.0f, -T * 0.5f), FVector(S.X, S.Y, T));

	const float WallZ = S.Z * 0.5f;

	// +X and -X walls span the full Y extent.
	for (int32 Sign = -1; Sign <= 1; Sign += 2)
	{
		const bool bDoorHere =
			(Sign > 0 && Building.DoorSide == ECapeDoorSide::PlusX) ||
			(Sign < 0 && Building.DoorSide == ECapeDoorSide::MinusX);

		AddWall(C + FVector(Sign * (S.X - T) * 0.5f, 0.0f, WallZ),
			FVector(T, S.Y, S.Z), /*bAlongY=*/true,
			bDoorHere, Building.DoorWidth, Building.DoorHeight);
	}

	// +Y and -Y walls span the X extent, inset so corners do not double up.
	for (int32 Sign = -1; Sign <= 1; Sign += 2)
	{
		const bool bDoorHere =
			(Sign > 0 && Building.DoorSide == ECapeDoorSide::PlusY) ||
			(Sign < 0 && Building.DoorSide == ECapeDoorSide::MinusY);

		AddWall(C + FVector(0.0f, Sign * (S.Y - T) * 0.5f, WallZ),
			FVector(S.X - 2.0f * T, T, S.Z), /*bAlongY=*/false,
			bDoorHere, Building.DoorWidth, Building.DoorHeight);
	}

	if (Building.bRoof)
	{
		AddBox(C + FVector(0.0f, 0.0f, S.Z + T * 0.5f), FVector(S.X, S.Y, T));
	}
}

FVector ACapeCampus::EntranceOffsetFor(const FCapeBuilding& Building) const
{
	// Two metres clear of the doorway, outside the wall.
	const float Clearance = 200.0f;
	switch (Building.DoorSide)
	{
	case ECapeDoorSide::PlusX:  return FVector(Building.Size.X * 0.5f + Clearance, 0.0f, 0.0f);
	case ECapeDoorSide::MinusX: return FVector(-(Building.Size.X * 0.5f + Clearance), 0.0f, 0.0f);
	case ECapeDoorSide::PlusY:  return FVector(0.0f, Building.Size.Y * 0.5f + Clearance, 0.0f);
	case ECapeDoorSide::MinusY: return FVector(0.0f, -(Building.Size.Y * 0.5f + Clearance), 0.0f);
	default: return FVector::ZeroVector;
	}
}

FVector ACapeCampus::GetBuildingEntrance(FName BuildingId) const
{
	for (const FCapeBuilding& Building : Buildings)
	{
		if (Building.Id == BuildingId)
		{
			return GetActorTransform().TransformPosition(
				Building.Center + EntranceOffsetFor(Building));
		}
	}
	return GetActorLocation();
}

void ACapeCampus::BuildGroundAndRoad()
{
	// Ground slab, centred between the plaza and the pad so both sit on it.
	const FVector GroundCenter(PadCenter.X * 0.5f, 0.0f, -GroundSize.Z * 0.5f);
	AddBox(GroundCenter, GroundSize);

	// Plaza — a slightly raised apron marking the campus centre.
	AddBox(FVector(0.0f, 0.0f, 5.0f), FVector(6000.0f, 6000.0f, 10.0f));

	// Road from the plaza edge out to the pad. One long slab; the walk is
	// supposed to feel like a distance, not a corridor.
	const float RoadStartX = 3000.0f;
	const float RoadLength = PadCenter.X - RoadStartX;
	if (RoadLength > 0.0f)
	{
		AddBox(FVector(RoadStartX + RoadLength * 0.5f, 0.0f, 4.0f),
			FVector(RoadLength, RoadWidth, 8.0f));
	}
}

void ACapeCampus::BuildPad()
{
	// Flame trench apron.
	AddBox(PadCenter + FVector(0.0f, 0.0f, 25.0f), FVector(12000.0f, 12000.0f, 50.0f));

	// Launch mount.
	AddBox(PadCenter + FVector(0.0f, 0.0f, 300.0f), FVector(3000.0f, 3000.0f, 600.0f));

	// Tower, offset to the -Y side, with a crew access arm reaching the ship.
	const FVector TowerBase = PadCenter + FVector(0.0f, -4000.0f, 0.0f);
	AddBox(TowerBase + FVector(0.0f, 0.0f, 6000.0f), FVector(1200.0f, 1200.0f, 12000.0f));

	// Crew access arm at ~65 m — where the ship's airlock sits (brief §4.2).
	AddBox(TowerBase + FVector(0.0f, 2000.0f, 6500.0f), FVector(400.0f, 4000.0f, 300.0f));

	// Starship stack placeholder: 9 m across, 120 m tall on the mount.
	AddCylinder(PadCenter + FVector(0.0f, 0.0f, 600.0f), 900.0f, 12000.0f);
}

void ACapeCampus::ClearSigns()
{
	for (UTextRenderComponent* Sign : Signs)
	{
		if (Sign)
		{
			Sign->DestroyComponent();
		}
	}
	Signs.Reset();
}

void ACapeCampus::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (Boxes)
	{
		Boxes->ClearInstances();
	}
	if (Cylinders)
	{
		Cylinders->ClearInstances();
	}
	ClearSigns();

	BuildGroundAndRoad();

	for (const FCapeBuilding& Building : Buildings)
	{
		BuildBuilding(Building);

		if (!bShowSigns)
		{
			continue;
		}

		// A floating name over each entrance. Everything here is the same shade
		// of grey; without labels the campus is genuinely hard to navigate.
		UTextRenderComponent* Sign = NewObject<UTextRenderComponent>(this);
		if (!Sign)
		{
			continue;
		}

		Sign->SetupAttachment(GetRootComponent());
		Sign->RegisterComponent();
		AddInstanceComponent(Sign);

		Sign->SetText(Building.DisplayName);
		Sign->SetWorldSize(220.0f);
		Sign->SetHorizontalAlignment(EHTA_Center);
		Sign->SetTextRenderColor(FColor(235, 240, 255));
		Sign->SetRelativeLocation(
			Building.Center + EntranceOffsetFor(Building) + FVector(0.0f, 0.0f, Building.Size.Z + 400.0f));

		Signs.Add(Sign);
	}

	BuildPad();
}

void ACapeCampus::BeginPlay()
{
	Super::BeginPlay();
	SpawnTerminals();
}

void ACapeCampus::SpawnTerminals()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (const FCapeBuilding& Building : Buildings)
	{
		if (!Building.bHasTerminal)
		{
			continue;
		}

		// Stand the console against the wall opposite the doorway, facing the
		// way the player will come in. Walking through the door should put the
		// screen in front of you without hunting for it.
		const FVector Outward = EntranceOffsetFor(Building).GetSafeNormal();
		const FVector Inward = -Outward;

		// Extent along the door axis, so the setback scales with the room.
		const float AxisExtent = FMath::Abs(Inward.X) > FMath::Abs(Inward.Y)
			? Building.Size.X
			: Building.Size.Y;
		const float Setback = FMath::Max(AxisExtent * 0.5f - 300.0f, 150.0f);

		FActorSpawnParameters Params;
		Params.Owner = this;
		Params.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		const FVector Location =
			GetActorTransform().TransformPosition(Building.Center + Inward * Setback);
		// The console's front face is -X, so aiming +X inward turns it to face
		// back toward the door.
		const FRotator Rotation = Inward.Rotation();

		if (AAresTerminal* Terminal =
			World->SpawnActor<AAresTerminal>(AAresTerminal::StaticClass(), Location, Rotation, Params))
		{
			Terminal->SetKind(Building.TerminalKind);
		}
	}
}
