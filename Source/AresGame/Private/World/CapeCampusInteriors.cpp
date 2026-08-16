// ACapeCampus — building interiors, the launch stack, and the landscape.
//
// Split out of CapeCampus.cpp because the shell and the fit-out are different
// jobs: the shell is layout the brief specifies, this is set dressing.
//
// Still blockout: every piece is a tinted engine primitive. What it buys is
// legibility — a tiered room full of dark consoles facing a lit wall reads as
// Mission Control instantly, in a way an empty grey box never will, and that
// matters for judging whether the SPACE works before any art exists.

#include "World/CapeCampus.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

#include "World/AresBike.h"

namespace
{
/** Deterministic scatter. Cosmetic only, so it does not touch FAresRng. */
float Hash01(int32 Index, int32 Salt)
{
	uint32 H = static_cast<uint32>(Index) * 374761393u + static_cast<uint32>(Salt) * 668265263u;
	H = (H ^ (H >> 13)) * 1274126177u;
	return static_cast<float>((H ^ (H >> 16)) & 0xFFFFFFu) / static_cast<float>(0xFFFFFF);
}

/** Signed variant in [-1, 1]. */
float HashSigned(int32 Index, int32 Salt)
{
	return Hash01(Index, Salt) * 2.0f - 1.0f;
}
} // namespace

void ACapeCampus::ApplySlot(UInstancedStaticMeshComponent* Component,
	const FCapeMeshSlot& Slot, UStaticMesh* FallbackMesh)
{
	if (!Component)
	{
		return;
	}

	// An assigned mesh always wins; the primitive is only ever a stand-in.
	UStaticMesh* Mesh = Slot.Mesh ? Slot.Mesh.Get() : FallbackMesh;
	Component->SetStaticMesh(Mesh);

	if (Slot.Material)
	{
		Component->SetMaterial(0, Slot.Material);
		return;
	}

	if (Slot.Mesh)
	{
		// A real asset ships with its own material. Overriding it with a flat
		// tint would throw away exactly the thing we imported it for.
		return;
	}

	// Programmer-art path: flat-tinted primitive.
	if (TintBaseMaterial)
	{
		if (UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(TintBaseMaterial, this))
		{
			Mid->SetVectorParameterValue(TEXT("Color"), Slot.FallbackTint);
			// Setting parameters the base material does not declare is a no-op,
			// so this stays correct on the engine's BasicShapeMaterial and only
			// starts doing anything once M_AresTint is in the project.
			Mid->SetScalarParameterValue(TEXT("Roughness"), Slot.FallbackRoughness);
			Mid->SetScalarParameterValue(TEXT("Metallic"), Slot.FallbackMetallic);
			Mid->SetScalarParameterValue(TEXT("Emissive"), Slot.FallbackEmissive);
			Component->SetMaterial(0, Mid);
		}
	}
}

FVector ACapeCampus::NativeSizeOf(const UInstancedStaticMeshComponent* Component) const
{
	if (!Component || !Component->GetStaticMesh())
	{
		return FVector(100.0f);
	}
	// Measured rather than assumed: a Fab asset is whatever size its author
	// made it, and the layout code only ever speaks in centimetres.
	const FVector Size = Component->GetStaticMesh()->GetBoundingBox().GetSize();
	return FVector(
		FMath::Max(Size.X, 1.0f),
		FMath::Max(Size.Y, 1.0f),
		FMath::Max(Size.Z, 1.0f));
}

FVector ACapeCampus::PivotOffsetOf(const UInstancedStaticMeshComponent* Component) const
{
	if (!Component || !Component->GetStaticMesh())
	{
		return FVector::ZeroVector;
	}
	// Engine primitives are centred on their pivot; imported assets frequently
	// are not. Correcting by the bounds centre means an off-pivot mesh still
	// lands where the layout asked for it.
	return Component->GetStaticMesh()->GetBoundingBox().GetCenter();
}

void ACapeCampus::AddFitted(UInstancedStaticMeshComponent* Component, const FCapeMeshSlot& Slot,
	const FVector& Center, const FVector& Size, float YawDegrees, int32 InstanceSeed)
{
	if (!Component || Size.X <= 0.0f || Size.Y <= 0.0f || Size.Z <= 0.0f)
	{
		return;
	}

	const FVector Native = NativeSizeOf(Component);

	FVector Scale;
	switch (Slot.Fit)
	{
	case ECapeFit::Native:
		Scale = FVector::OneVector;
		break;
	case ECapeFit::Uniform:
	{
		// Uniform keeps a prop's proportions. Stretching a tree or a chair to a
		// requested box makes it look broken in a way a wall never does.
		const FVector Ratio = Size / Native;
		Scale = FVector(FMath::Min3(Ratio.X, Ratio.Y, Ratio.Z));
		break;
	}
	case ECapeFit::Stretch:
	default:
		Scale = Size / Native;
		break;
	}

	Scale *= Slot.ExtraScale;

	float Yaw = YawDegrees;
	if (Slot.bRandomYaw)
	{
		Yaw += Hash01(InstanceSeed, 91) * 360.0f;
	}

	const FRotator Rotation(0.0f, Yaw, 0.0f);
	const FVector Offset = Rotation.RotateVector(PivotOffsetOf(Component) * Scale);

	Component->AddInstance(FTransform(Rotation, Center - Offset, Scale));
}

void ACapeCampus::AddBoxTo(UInstancedStaticMeshComponent* Component, const FVector& Center,
	const FVector& Size, float YawDegrees)
{
	AddFitted(Component, SlotForComponent(Component), Center, Size, YawDegrees,
		Component ? Component->GetInstanceCount() : 0);
}

void ACapeCampus::AddCylinderTo(UInstancedStaticMeshComponent* Component,
	const FVector& BaseCenter, float DiameterCm, float HeightCm)
{
	// Cylinders and cones are specified base-up, so lift by half the height to
	// give AddFitted a centre.
	AddFitted(Component, SlotForComponent(Component),
		BaseCenter + FVector(0.0f, 0.0f, HeightCm * 0.5f),
		FVector(DiameterCm, DiameterCm, HeightCm), 0.0f,
		Component ? Component->GetInstanceCount() : 0);
}

void ACapeCampus::AddConeTo(UInstancedStaticMeshComponent* Component,
	const FVector& BaseCenter, float DiameterCm, float HeightCm)
{
	AddFitted(Component, SlotForComponent(Component),
		BaseCenter + FVector(0.0f, 0.0f, HeightCm * 0.5f),
		FVector(DiameterCm, DiameterCm, HeightCm), 0.0f,
		Component ? Component->GetInstanceCount() : 0);
}

const FCapeMeshSlot& ACapeCampus::SlotForComponent(const UInstancedStaticMeshComponent* Component) const
{
	// Small and explicit rather than a map: there are nine of these and the
	// mapping is the API, so making it visible is worth more than making it
	// clever.
	if (Component == Furniture) { return FurnitureSlot; }
	if (Component == Screens) { return ScreenSlot; }
	if (Component == Steel) { return SteelBarrelSlot; }
	if (Component == SteelBox) { return SteelDetailSlot; }
	if (Component == SteelCone) { return NoseconeSlot; }
	if (Component == Grass) { return GroundSlot; }
	if (Component == Cylinders) { return TrunkSlot; }
	if (Component == Foliage) { return CanopySlot; }
	if (Component == GroundCover) { return GroundCoverSlot; }

	// Paint buckets are not art slots and must never inherit one. They are the
	// engine primitive at an exact requested size; falling through to
	// StructureSlot would hand them its fit mode, so assigning a uniform-fit Fab
	// mesh to the walls would quietly bend every stripe and window on the campus.
	for (const TObjectPtr<UInstancedStaticMeshComponent>& Bucket : PaintBuckets)
	{
		if (Bucket.Get() == Component)
		{
			return PaintSlot;
		}
	}

	return StructureSlot;
}

/* -------------------------------------------------------------------------- */
/*  Mission Control                                                            */
/* -------------------------------------------------------------------------- */

void ACapeCampus::BuildMissionControlInterior(const FCapeBuilding& B)
{
	// Reference: JPL and the Firing Room. Tiered rows of consoles all facing a
	// front projection wall, the room stepping down toward the screens so every
	// controller has a sightline. FLIGHT sits at the back where they can see
	// every position at once.
	const FVector C = B.Center;
	const float HalfY = B.Size.Y * 0.5f;

	// Door is on +Y, so the screen wall goes on -Y and the rows face it.
	const float ScreenY = C.Y - HalfY + 120.0f;

	// --- front projection wall: a bank of large panels ---
	constexpr int32 PanelCount = 5;
	const float PanelWidth = (B.Size.X - 800.0f) / PanelCount;
	for (int32 I = 0; I < PanelCount; ++I)
	{
		const float X = C.X - (B.Size.X - 800.0f) * 0.5f + PanelWidth * (I + 0.5f);
		// Centre panel is the big one — the trajectory plot everyone watches.
		const float Height = (I == PanelCount / 2) ? 420.0f : 300.0f;
		AddBoxTo(Screens, FVector(X, ScreenY, 700.0f),
			FVector(PanelWidth - 60.0f, 30.0f, Height));
	}

	// --- tiered console rows ---
	// Four tiers, each raised and set back from the one in front. Twelve to
	// sixteen positions total, per brief §4.2.
	constexpr int32 TierCount = 4;
	constexpr int32 ConsolesPerTier = 4;
	constexpr float TierRise = 45.0f;
	const float TierDepth = (B.Size.Y - 1400.0f) / TierCount;

	for (int32 Tier = 0; Tier < TierCount; ++Tier)
	{
		const float TierZ = Tier * TierRise;
		const float RowY = ScreenY + 700.0f + Tier * TierDepth;

		// The step the row stands on.
		AddBox(FVector(C.X, RowY, TierZ * 0.5f),
			FVector(B.Size.X - 700.0f, TierDepth * 0.9f, FMath::Max(TierZ, 10.0f)));

		const float Span = B.Size.X - 1400.0f;
		for (int32 Seat = 0; Seat < ConsolesPerTier; ++Seat)
		{
			const float X = C.X - Span * 0.5f + Span * (Seat + 0.5f) / ConsolesPerTier;

			// Desk.
			AddBoxTo(Furniture, FVector(X, RowY, TierZ + 37.0f), FVector(320.0f, 90.0f, 74.0f));
			// Two monitors, raked toward the wall.
			AddBoxTo(Screens, FVector(X - 70.0f, RowY - 30.0f, TierZ + 110.0f),
				FVector(120.0f, 12.0f, 70.0f));
			AddBoxTo(Screens, FVector(X + 70.0f, RowY - 30.0f, TierZ + 110.0f),
				FVector(120.0f, 12.0f, 70.0f));
			// Chair, behind the desk.
			AddBoxTo(Furniture, FVector(X, RowY + 70.0f, TierZ + 45.0f),
				FVector(55.0f, 55.0f, 90.0f));
		}
	}
}

/* -------------------------------------------------------------------------- */
/*  Research Center                                                            */
/* -------------------------------------------------------------------------- */

void ACapeCampus::BuildResearchInterior(const FCapeBuilding& B)
{
	// Reference: JPL's High Bay clean room. A tall bright open bay, spacecraft
	// hardware on stands down the middle, equipment and benches around the
	// edges, a gantry stair to one side.
	const FVector C = B.Center;
	const float HalfX = B.Size.X * 0.5f;
	const float HalfY = B.Size.Y * 0.5f;

	// --- benches along both long walls ---
	constexpr int32 BenchCount = 7;
	const float BenchSpan = B.Size.X - 900.0f;
	for (int32 I = 0; I < BenchCount; ++I)
	{
		const float X = C.X - BenchSpan * 0.5f + BenchSpan * (I + 0.5f) / BenchCount;
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			const float Y = C.Y + Side * (HalfY - 260.0f);
			AddBoxTo(Furniture, FVector(X, Y, 45.0f), FVector(220.0f, 90.0f, 90.0f));
			// Equipment rack behind the bench.
			AddBoxTo(Furniture, FVector(X, Y + Side * 70.0f, 110.0f),
				FVector(200.0f, 50.0f, 220.0f));
		}
	}

	// --- spacecraft under assembly, down the centreline ---
	// A lander on a work stand: octagonal bus, legs, and a boom.
	{
		const FVector Bus(C.X, C.Y, 0.0f);
		// Work stand.
		AddBoxTo(Furniture, Bus + FVector(0.0f, 0.0f, 55.0f), FVector(420.0f, 420.0f, 110.0f));
		// Bus.
		AddCylinderTo(Steel, Bus + FVector(0.0f, 0.0f, 110.0f), 340.0f, 200.0f);
		// High-gain dish, as a shallow cone.
		AddConeTo(SteelCone, Bus + FVector(0.0f, 0.0f, 310.0f), 260.0f, 90.0f);
		// Landing legs.
		for (int32 Leg = 0; Leg < 4; ++Leg)
		{
			const float Angle = Leg * 90.0f + 45.0f;
			const float Rad = FMath::DegreesToRadians(Angle);
			AddBoxTo(SteelBox,
				Bus + FVector(FMath::Cos(Rad) * 200.0f, FMath::Sin(Rad) * 200.0f, 150.0f),
				FVector(40.0f, 40.0f, 110.0f), Angle);
		}
	}

	// --- second article further down the bay ---
	{
		const FVector Bus(C.X + 900.0f, C.Y, 0.0f);
		AddBoxTo(Furniture, Bus + FVector(0.0f, 0.0f, 40.0f), FVector(300.0f, 300.0f, 80.0f));
		AddCylinderTo(Steel, Bus + FVector(0.0f, 0.0f, 80.0f), 240.0f, 260.0f);
	}

	// --- access gantry along the -X end ---
	const float GantryX = C.X - HalfX + 500.0f;
	AddBox(FVector(GantryX, C.Y, 240.0f), FVector(300.0f, B.Size.Y - 700.0f, 30.0f));
	for (int32 Post = -1; Post <= 1; Post += 2)
	{
		AddBox(FVector(GantryX, C.Y + Post * (B.Size.Y * 0.5f - 400.0f), 120.0f),
			FVector(40.0f, 40.0f, 240.0f));
	}
	// Handrail.
	AddBox(FVector(GantryX + 140.0f, C.Y, 300.0f), FVector(15.0f, B.Size.Y - 700.0f, 100.0f));
}

/* -------------------------------------------------------------------------- */
/*  Administration                                                             */
/* -------------------------------------------------------------------------- */

void ACapeCampus::BuildAdministrationInterior(const FCapeBuilding& B)
{
	// Reference: the Firing Room's back rows and an ordinary government office.
	// Half the floor is an open-plan desk grid; the other half is the hearing
	// room, where brief §7's committee sessions happen.
	const FVector C = B.Center;
	const float HalfY = B.Size.Y * 0.5f;

	// --- open-plan desks, +Y half ---
	constexpr int32 DeskCols = 4;
	constexpr int32 DeskRows = 3;
	const float GridX = B.Size.X - 1200.0f;
	const float GridY = B.Size.Y * 0.5f - 700.0f;

	for (int32 Col = 0; Col < DeskCols; ++Col)
	{
		for (int32 Row = 0; Row < DeskRows; ++Row)
		{
			const float X = C.X - GridX * 0.5f + GridX * (Col + 0.5f) / DeskCols;
			const float Y = C.Y + 400.0f + GridY * (Row + 0.5f) / DeskRows;

			AddBoxTo(Furniture, FVector(X, Y, 37.0f), FVector(180.0f, 90.0f, 74.0f));
			AddBoxTo(Screens, FVector(X, Y - 25.0f, 105.0f), FVector(110.0f, 10.0f, 60.0f));
			AddBoxTo(Furniture, FVector(X, Y + 75.0f, 45.0f), FVector(55.0f, 55.0f, 90.0f));
			// Low partition, so it reads as cubicles rather than a canteen.
			AddBoxTo(Furniture, FVector(X, Y - 70.0f, 60.0f), FVector(190.0f, 8.0f, 120.0f));
		}
	}

	// --- hearing room, -Y half ---
	const float BenchY = C.Y - HalfY + 600.0f;

	// Raised committee bench, curved into three segments.
	AddBox(FVector(C.X, BenchY, 30.0f), FVector(B.Size.X - 1000.0f, 400.0f, 60.0f));
	for (int32 Seg = -1; Seg <= 1; ++Seg)
	{
		const float Yaw = Seg * 14.0f;
		AddBoxTo(Furniture, FVector(C.X + Seg * 420.0f, BenchY, 115.0f),
			FVector(400.0f, 110.0f, 110.0f), Yaw);
		// Chairs behind the bench.
		AddBoxTo(Furniture, FVector(C.X + Seg * 420.0f, BenchY - 110.0f, 105.0f),
			FVector(60.0f, 60.0f, 90.0f), Yaw);
	}

	// Witness table, facing the bench. This is where you sit.
	AddBoxTo(Furniture, FVector(C.X, BenchY + 700.0f, 37.0f), FVector(300.0f, 100.0f, 74.0f));
	AddBoxTo(Furniture, FVector(C.X, BenchY + 790.0f, 45.0f), FVector(60.0f, 60.0f, 90.0f));

	// Public gallery: three rows of seating behind the witness table.
	for (int32 Row = 0; Row < 3; ++Row)
	{
		const float Y = BenchY + 1100.0f + Row * 130.0f;
		AddBoxTo(Furniture, FVector(C.X, Y, 42.0f), FVector(B.Size.X - 1600.0f, 60.0f, 84.0f));
	}
}

/* -------------------------------------------------------------------------- */
/*  Vehicle Assembly Building                                                  */
/* -------------------------------------------------------------------------- */

void ACapeCampus::BuildVabInterior(const FCapeBuilding& B)
{
	// Reference: the Starship high bay — booster sections standing in a row
	// inside a steel frame, work platforms up the walls, a crane over the top.
	const FVector C = B.Center;
	const float HalfX = B.Size.X * 0.5f;
	const float HalfY = B.Size.Y * 0.5f;
	const float Height = B.Size.Z;

	// --- work platforms stepping up the -X wall ---
	constexpr int32 Levels = 8;
	for (int32 Level = 1; Level <= Levels; ++Level)
	{
		const float Z = Height * Level / (Levels + 1);
		AddBox(FVector(C.X - HalfX + 600.0f, C.Y, Z), FVector(1000.0f, B.Size.Y - 800.0f, 40.0f));
		// Handrail.
		AddBox(FVector(C.X - HalfX + 1080.0f, C.Y, Z + 60.0f),
			FVector(20.0f, B.Size.Y - 800.0f, 110.0f));
		// Support column down to the level below.
		AddBox(FVector(C.X - HalfX + 1050.0f, C.Y, Z - Height / (2.0f * (Levels + 1))),
			FVector(50.0f, 50.0f, Height / (Levels + 1)));
	}

	// --- overhead crane rail and trolley ---
	const float CraneZ = Height - 500.0f;
	for (int32 Side = -1; Side <= 1; Side += 2)
	{
		AddBox(FVector(C.X, C.Y + Side * (HalfY - 300.0f), CraneZ),
			FVector(B.Size.X - 400.0f, 90.0f, 90.0f));
	}
	// Bridge across, with the hook block hanging under it.
	AddBox(FVector(C.X + 800.0f, C.Y, CraneZ + 120.0f), FVector(200.0f, B.Size.Y - 600.0f, 150.0f));
	AddBoxTo(SteelBox, FVector(C.X + 800.0f, C.Y, CraneZ - 400.0f), FVector(120.0f, 120.0f, 300.0f));

    // --- vehicle sections standing in the bay ---
	// Three barrels in a row, in build order: a stacked booster forward section,
	// a bare barrel, and a ship with its nosecone on.
	const float BayY = C.Y + 600.0f;

	// Booster section, part-stacked.
	AddCylinderTo(Steel, FVector(C.X + 1800.0f, BayY, 0.0f), 900.0f, Height * 0.62f);
	// Bare barrel awaiting stack.
	AddCylinderTo(Steel, FVector(C.X + 300.0f, BayY, 0.0f), 900.0f, Height * 0.30f);
	// Ship with nosecone.
	{
		const float ShipBody = Height * 0.38f;
		AddCylinderTo(Steel, FVector(C.X - 1200.0f, BayY, 0.0f), 900.0f, ShipBody);
		AddConeTo(SteelCone, FVector(C.X - 1200.0f, BayY, ShipBody), 900.0f, 1400.0f);
	}

	// --- transport stands and ground equipment ---
	for (int32 I = 0; I < 3; ++I)
	{
		const float X = C.X - 1200.0f + I * 1500.0f;
		AddBoxTo(Furniture, FVector(X, BayY - 900.0f, 60.0f), FVector(500.0f, 300.0f, 120.0f));
	}
}

/* -------------------------------------------------------------------------- */

void ACapeCampus::BuildInteriors()
{
	for (const FCapeBuilding& B : Buildings)
	{
		if (B.Id == TEXT("MCC")) { BuildMissionControlInterior(B); }
		else if (B.Id == TEXT("RND")) { BuildResearchInterior(B); }
		else if (B.Id == TEXT("HQ")) { BuildAdministrationInterior(B); }
		else if (B.Id == TEXT("VAB")) { BuildVabInterior(B); }
	}
}

/* -------------------------------------------------------------------------- */
/*  Launch vehicle                                                             */
/* -------------------------------------------------------------------------- */

void ACapeCampus::BuildLaunchVehicle()
{
	// Real proportions: 9 m across, Super Heavy ~71 m, Starship ~50 m, and the
	// full stack a shade over 120 m. Standing next to it is the point — brief §1
	// puts you physically present, and the scale only lands if the numbers are
	// the actual numbers.
	constexpr float Diameter = 900.0f;
	constexpr float BoosterHeight = 7100.0f;
	constexpr float ShipBody = 3600.0f;
	constexpr float ShipNose = 1400.0f;

	const FVector Base = PadCenter + FVector(0.0f, 0.0f, 600.0f);

	// --- Super Heavy ---
	AddCylinderTo(Steel, Base, Diameter, BoosterHeight);

	// Grid fins near the top, four of them.
	for (int32 Fin = 0; Fin < 4; ++Fin)
	{
		const float Angle = Fin * 90.0f;
		const float Rad = FMath::DegreesToRadians(Angle);
		AddBoxTo(SteelBox,
			Base + FVector(FMath::Cos(Rad) * (Diameter * 0.5f + 90.0f),
				FMath::Sin(Rad) * (Diameter * 0.5f + 90.0f),
				BoosterHeight - 500.0f),
			FVector(180.0f, 60.0f, 260.0f), Angle);
	}

	// Engine skirt.
	AddCylinderTo(Steel, PadCenter + FVector(0.0f, 0.0f, 300.0f), Diameter + 40.0f, 320.0f);

	// --- Starship ---
	const float ShipBase = Base.Z + BoosterHeight;
	AddCylinderTo(Steel, FVector(Base.X, Base.Y, ShipBase), Diameter, ShipBody);
	AddConeTo(SteelCone, FVector(Base.X, Base.Y, ShipBase + ShipBody), Diameter, ShipNose);

	// Forward and aft flaps.
	for (int32 Side = -1; Side <= 1; Side += 2)
	{
		AddBoxTo(SteelBox,
			FVector(Base.X + Side * (Diameter * 0.5f + 120.0f), Base.Y, ShipBase + ShipBody - 300.0f),
			FVector(240.0f, 70.0f, 700.0f));
		AddBoxTo(SteelBox,
			FVector(Base.X + Side * (Diameter * 0.5f + 150.0f), Base.Y, ShipBase + 500.0f),
			FVector(300.0f, 80.0f, 900.0f));
	}

	// Black heat shield down the windward side. A bare steel cylinder reads as a
	// grain silo; the tile line is what makes it Starship from a kilometre out.
	// Faced back toward the campus, so it is the side you see on the walk in.
	if (bDressExteriors)
	{
		AddHeatTiles(FVector(Base.X, Base.Y, 0.0f), Diameter,
			ShipBase, ShipBase + ShipBody, /*YawCenter=*/180.0f);
	}
}

/* -------------------------------------------------------------------------- */
/*  Landscape                                                                  */
/* -------------------------------------------------------------------------- */

bool ACapeCampus::IsOpenGround(const FVector& P) const
{
	// Road corridor.
	if (FMath::Abs(P.Y) < RoadWidth * 1.6f && P.X > -4000.0f && P.X < PadCenter.X + 14000.0f)
	{
		return false;
	}
	// Plaza.
	if (P.SizeSquared2D() < 9000.0f * 9000.0f)
	{
		return false;
	}
	// Pad and its flame area.
	if (FVector::DistSquared2D(P, PadCenter) < 20000.0f * 20000.0f)
	{
		return false;
	}
	// Buildings, with a margin so nothing grows through a wall.
	for (const FCapeBuilding& B : Buildings)
	{
		const FVector D = (P - B.Center).GetAbs();
		if (D.X < B.Size.X * 0.5f + 400.0f && D.Y < B.Size.Y * 0.5f + 400.0f)
		{
			return false;
		}
	}
	return true;
}

void ACapeCampus::BuildGroundCover()
{
	// Nothing to scatter until a real mesh is assigned. Thirty thousand grey
	// cubes would be worse than bare ground, so this stays off rather than
	// falling back to a primitive the way every other slot does.
	if (!GroundCover || !GroundCoverSlot.Mesh || GroundCoverCount <= 0)
	{
		return;
	}

	GroundCover->SetCullDistances(0, static_cast<int32>(GroundCoverCullDistanceM * 100.0f));

	const float RadiusCm = GroundCoverRadiusM * 100.0f;
	int32 Placed = 0;

	// Rejection sampling against the same keep-out the trees use. Over-sampling
	// 3x covers the area the campus occupies without an unbounded loop.
	for (int32 I = 0; I < GroundCoverCount * 3 && Placed < GroundCoverCount; ++I)
	{
		// Uniform-in-disc: sqrt on the radius, or everything piles up at the
		// centre and the far field stays bald.
		const float Angle = Hash01(I, 401) * 2.0f * PI;
		const float Radius = FMath::Sqrt(Hash01(I, 409)) * RadiusCm;

		const FVector P(
			FMath::Cos(Angle) * Radius,
			FMath::Sin(Angle) * Radius,
			4.0f);

		if (!IsOpenGround(P))
		{
			continue;
		}

		const float Scale = 0.65f + Hash01(I, 417) * 0.7f;
		const float Size = GroundCoverSizeCm * Scale;

		AddFitted(GroundCover, GroundCoverSlot, P, FVector(Size, Size, Size), 0.0f, I);
		++Placed;
	}

	UE_LOG(LogTemp, Log, TEXT("CapeCampus: scattered %d ground-cover instances."), Placed);
}

void ACapeCampus::AddTiledSurface(UInstancedStaticMeshComponent* Component, const FVector& Center,
	const FVector2D& AreaSize, float Thickness, float TileSizeCm)
{
	if (!Component || AreaSize.X <= 0.0f || AreaSize.Y <= 0.0f || TileSizeCm <= 0.0f)
	{
		return;
	}

	// Whole tiles only, then the tile size is adjusted so they exactly fill the
	// area — a partial tile at the edge would either overhang or leave a seam.
	const int32 CountX = FMath::Max(1, FMath::RoundToInt(AreaSize.X / TileSizeCm));
	const int32 CountY = FMath::Max(1, FMath::RoundToInt(AreaSize.Y / TileSizeCm));

	const float StepX = AreaSize.X / CountX;
	const float StepY = AreaSize.Y / CountY;

	const float OriginX = Center.X - AreaSize.X * 0.5f + StepX * 0.5f;
	const float OriginY = Center.Y - AreaSize.Y * 0.5f + StepY * 0.5f;

	for (int32 IX = 0; IX < CountX; ++IX)
	{
		for (int32 IY = 0; IY < CountY; ++IY)
		{
			AddBoxTo(Component,
				FVector(OriginX + IX * StepX, OriginY + IY * StepY, Center.Z),
				FVector(StepX, StepY, Thickness));
		}
	}
}

void ACapeCampus::BuildLandscape()
{
	// A green apron over the concrete, then trees scattered outside the built
	// area. Placement is hashed rather than random so the campus looks the same
	// every time you open it — a tree that moves between sessions is a bug you
	// will chase.
	const FVector GroundCenter(PadCenter.X * 0.5f, 0.0f, 0.0f);

	// Grass sits a hair above the ground slab to avoid z-fighting with it, and
	// is TILED rather than being one enormous quad — see GroundTileSizeM. The
	// collision slab underneath stays a single box, because 3000 collision
	// primitives to stand on would be absurd when one does the job.
	AddTiledSurface(Grass, GroundCenter + FVector(0.0f, 0.0f, 2.0f),
		FVector2D(GroundSize.X * 0.98f, GroundSize.Y * 0.98f),
		4.0f, GroundTileSizeM * 100.0f);

	int32 Placed = 0;
	for (int32 I = 0; I < TreeCount * 4 && Placed < TreeCount; ++I)
	{
		const FVector P(
			HashSigned(I, 11) * GroundSize.X * 0.44f + GroundCenter.X,
			HashSigned(I, 23) * GroundSize.Y * 0.44f,
			0.0f);

		if (!IsOpenGround(P))
		{
			continue;
		}

		const float Scale = 0.7f + Hash01(I, 37) * 0.9f;
		const float TrunkHeight = 380.0f * Scale;
		const float CanopyHeight = 620.0f * Scale;

		if (bTrunkSlotIsWholeTree)
		{
			// A complete Fab tree: one instance, whole height, random yaw so a
			// few hundred of them do not read as a repeated stamp.
			AddFitted(Cylinders, TrunkSlot, P, FVector(600.0f, 600.0f, TrunkHeight + CanopyHeight),
				0.0f, I);
		}
		else
		{
			AddCylinderTo(Cylinders, P, 60.0f * Scale, TrunkHeight);
			AddConeTo(Foliage, P + FVector(0.0f, 0.0f, TrunkHeight * 0.55f),
				500.0f * Scale, CanopyHeight);
			// Second, smaller cone above for a layered silhouette.
			AddConeTo(Foliage, P + FVector(0.0f, 0.0f, TrunkHeight * 0.55f + CanopyHeight * 0.45f),
				360.0f * Scale, CanopyHeight * 0.75f);
		}

		++Placed;
	}
}

/* -------------------------------------------------------------------------- */

void ACapeCampus::SpawnBikes()
{
	UWorld* World = GetWorld();
	if (!World || BikeCount <= 0)
	{
		return;
	}

	// A rack at the edge of the plaza, on the pad side. The walk out is 650 m;
	// the bikes are the answer to "do I really have to walk that every time".
	const FVector RackOrigin(3400.0f, -900.0f, 0.0f);

	for (int32 I = 0; I < BikeCount; ++I)
	{
		FActorSpawnParameters Params;
		Params.Owner = this;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		const FVector Local = RackOrigin + FVector(0.0f, I * 160.0f, 0.0f);
		const FVector Location = GetActorTransform().TransformPosition(Local);

		World->SpawnActor<AAresBike>(AAresBike::StaticClass(), Location, FRotator::ZeroRotator, Params);
	}

	UE_LOG(LogTemp, Log, TEXT("CapeCampus: spawned %d bike(s)."), BikeCount);
}
