#include "World/CapeCampus.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInterface.h"
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
const TCHAR* ConePath = TEXT("/Engine/BasicShapes/Cone.Cone");
const TCHAR* TintMaterialPath = TEXT("/Engine/BasicShapes/BasicShapeMaterial");

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
	// Mobility must not be stricter than the root's. A Static component cannot
	// attach to a Movable parent — Unreal aborts the attach with a warning and
	// the component silently renders in world space instead of actor space,
	// putting the geometry in a different frame from anything spawned through
	// GetActorTransform(). Leaving these Movable keeps one frame for everything.
	// Nothing here needs baked lighting: the interior lights are Movable and
	// Lumen handles the rest.

	Cylinders = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Cylinders"));
	Cylinders->SetupAttachment(Root);
	Cylinders->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Cylinders->SetCollisionResponseToAllChannels(ECR_Block);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(CubePath);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(CylinderPath);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(ConePath);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> TintMaterial(TintMaterialPath);

	if (CubeMesh.Succeeded()) { Boxes->SetStaticMesh(CubeMesh.Object); }
	if (CylinderMesh.Succeeded()) { Cylinders->SetStaticMesh(CylinderMesh.Object); }
	if (TintMaterial.Succeeded()) { TintBaseMaterial = TintMaterial.Object; }

	// The remaining components exist only to carry a different material — an
	// instanced mesh draws with one material, so colour means another component.
	auto MakeInstanced = [&](const TCHAR* Name, UStaticMesh* Mesh, bool bCollides)
		-> UInstancedStaticMeshComponent*
	{
		UInstancedStaticMeshComponent* Component =
			CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
		Component->SetupAttachment(Root);
		if (Mesh) { Component->SetStaticMesh(Mesh); }
		if (bCollides)
		{
			Component->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			Component->SetCollisionResponseToAllChannels(ECR_Block);
		}
		else
		{
			// Trees and grass should not be things you walk into or that block
			// the interaction trace.
			Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		return Component;
	};

	UStaticMesh* Cube = CubeMesh.Succeeded() ? CubeMesh.Object : nullptr;
	UStaticMesh* Cyl = CylinderMesh.Succeeded() ? CylinderMesh.Object : nullptr;
	UStaticMesh* Cone = ConeMesh.Succeeded() ? ConeMesh.Object : nullptr;

	Furniture = MakeInstanced(TEXT("Furniture"), Cube, true);
	Screens = MakeInstanced(TEXT("Screens"), Cube, false);
	Steel = MakeInstanced(TEXT("Steel"), Cyl, true);
	SteelBox = MakeInstanced(TEXT("SteelBox"), Cube, true);
	SteelCone = MakeInstanced(TEXT("SteelCone"), Cone, true);
	Grass = MakeInstanced(TEXT("Grass"), Cube, false);
	Foliage = MakeInstanced(TEXT("Foliage"), Cone, false);

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
	AddCylinderTo(Cylinders, BaseCenter, DiameterCm, HeightCm);
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

	// The vehicle itself is built by BuildLaunchVehicle, so the pad only
	// provides the mount and the tower.
}

void ACapeCampus::ClearGenerated()
{
	for (USceneComponent* Component : Generated)
	{
		if (Component)
		{
			Component->DestroyComponent();
		}
	}
	Generated.Reset();
}

void ACapeCampus::AddInteriorLight(const FCapeBuilding& Building)
{
	UPointLightComponent* Light = NewObject<UPointLightComponent>(this);
	if (!Light)
	{
		return;
	}

	Light->SetupAttachment(GetRootComponent());
	Light->RegisterComponent();
	AddInstanceComponent(Light);

	// Movable, so dropping the campus into a level needs no lighting build.
	Light->SetMobility(EComponentMobility::Movable);

	// Just under the ceiling, or 12 m up in a tall volume like the VAB where a
	// single fixture at the apex would not reach the floor usefully.
	const float Height = FMath::Min(Building.Size.Z * 0.8f, 1200.0f);
	Light->SetRelativeLocation(Building.Center + FVector(0.0f, 0.0f, Height));

	// Reach the far corners: half the diagonal, with headroom.
	const float Radius = FVector(Building.Size.X, Building.Size.Y, Building.Size.Z).Size() * 0.75f;
	Light->SetAttenuationRadius(Radius);

	// Bigger rooms need proportionally more light, referenced to a 40 x 30 m box.
	const float AreaScale = FMath::Clamp(
		(Building.Size.X * Building.Size.Y) / (4000.0f * 3000.0f), 1.0f, 6.0f);
	Light->SetIntensity(InteriorLightIntensity * AreaScale);

	// Cool fluorescent. Institutional, per brief §1's tone.
	Light->SetLightColor(FLinearColor(0.86f, 0.91f, 1.0f));

	// Shadows off: this is a blockout light whose only job is legibility, and
	// a large-radius shadowcaster in every building is not worth the cost.
	Light->SetCastShadows(false);

	Generated.Add(Light);
}

void ACapeCampus::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (Boxes)
	{
		Boxes->ClearInstances();
	}
	for (UInstancedStaticMeshComponent* Component :
		{ Cylinders.Get(), Furniture.Get(), Screens.Get(), Steel.Get(),
		  SteelBox.Get(), SteelCone.Get(), Grass.Get(), Foliage.Get() })
	{
		if (Component)
		{
			Component->ClearInstances();
		}
	}
	ClearGenerated();

	// Tint before filling, so a freshly opened level is already coloured.
	ApplyTint(Boxes, ConcreteColor);
	ApplyTint(Cylinders, TrunkColor);
	ApplyTint(Furniture, FurnitureColor);
	ApplyTint(Screens, ScreenColor);
	ApplyTint(Steel, SteelColor);
	ApplyTint(SteelBox, SteelColor);
	ApplyTint(SteelCone, SteelColor);
	ApplyTint(Grass, GrassColor);
	ApplyTint(Foliage, FoliageColor);

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

		Generated.Add(Sign);
	}

	if (bInteriorLights)
	{
		for (const FCapeBuilding& Building : Buildings)
		{
			AddInteriorLight(Building);
		}
	}

	BuildPad();

	if (bBuildLaunchVehicle)
	{
		BuildLaunchVehicle();
	}
	if (bBuildInteriors)
	{
		BuildInteriors();
	}
	if (bBuildLandscape)
	{
		BuildLandscape();
	}
}

void ACapeCampus::BeginPlay()
{
	Super::BeginPlay();
	SpawnTerminals();
	SpawnBikes();
}

void ACapeCampus::SpawnTerminals()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	int32 Spawned = 0;

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

		// Extent along the door axis, so placement scales with the room.
		const float AxisExtent = FMath::Abs(Inward.X) > FMath::Abs(Inward.Y)
			? Building.Size.X
			: Building.Size.Y;
		const float HalfExtent = AxisExtent * 0.5f;

		// Stand the console a short walk INSIDE the doorway, not against the far
		// wall. These rooms are 35-50 m deep; putting the console opposite the
		// door meant walking into Administration and finding nothing, because
		// the thing you came for was 40 m away in the dark.
		//
		// Setback is measured from the room centre toward the back wall, so it
		// goes negative in a deep room — the console ends up on the door side of
		// centre, which is what you want.
		const float TargetFromDoor = 1000.0f;               // 10 m inside
		const float AgainstBackWall = HalfExtent - 500.0f;  // small rooms only
		const float Setback = FMath::Min(AgainstBackWall, TargetFromDoor - HalfExtent);

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
			++Spawned;

			// Report containment directly rather than leaving it to be worked
			// out from coordinates by hand. A console outside its room is
			// indistinguishable from one that never spawned.
			const FVector LocalPos = Building.Center + Inward * Setback;
			const FVector HalfInner = Building.Size * 0.5f
				- FVector(Building.WallThickness, Building.WallThickness, 0.0f);
			const bool bInside =
				FMath::Abs(LocalPos.X - Building.Center.X) < HalfInner.X &&
				FMath::Abs(LocalPos.Y - Building.Center.Y) < HalfInner.Y;

			UE_LOG(LogTemp, Log,
				TEXT("CapeCampus: %s terminal world=%s local=%s inside=%s"),
				*Building.Id.ToString(), *Location.ToCompactString(),
				*LocalPos.ToCompactString(), bInside ? TEXT("YES") : TEXT("NO"));
		}
	}

	UE_LOG(LogTemp, Log, TEXT("CapeCampus: spawned %d terminal(s)."), Spawned);
}
