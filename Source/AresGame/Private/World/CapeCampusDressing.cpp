// ACapeCampus — Fortnite-style NASA dressing.
//
// The shell in CapeCampus.cpp is layout. This file is identity: colour-blocked
// facades so Mission Control, JPL, Admin and the VAB read from the plaza
// without a single authored mesh. Drop a Fab kit into the art slots later and
// the same layout still holds; until then the paint is the art.

#include "World/CapeCampus.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace
{
constexpr int32 BrushCount = static_cast<int32>(ECapeBrush::Cone) + 1;
constexpr int32 PaintCount = static_cast<int32>(ECapePaint::Wood) + 1;
} // namespace

FLinearColor ACapeCampus::ColorOf(ECapePaint Paint) const
{
	switch (Paint)
	{
	case ECapePaint::Concrete: return FLinearColor(0.50f, 0.48f, 0.44f);
	case ECapePaint::White:    return FLinearColor(0.92f, 0.93f, 0.95f);
	case ECapePaint::NasaBlue: return FLinearColor(0.05f, 0.22f, 0.58f);
	case ECapePaint::Orange:   return FLinearColor(0.95f, 0.38f, 0.08f);
	case ECapePaint::Black:    return FLinearColor(0.04f, 0.04f, 0.05f);
	case ECapePaint::Beige:    return FLinearColor(0.74f, 0.66f, 0.52f);
	case ECapePaint::Steel:    return FLinearColor(0.76f, 0.79f, 0.84f);
	case ECapePaint::Red:      return FLinearColor(0.78f, 0.08f, 0.08f);
	case ECapePaint::Yellow:   return FLinearColor(0.95f, 0.80f, 0.10f);
	case ECapePaint::Glass:    return FLinearColor(0.12f, 0.28f, 0.42f);
	case ECapePaint::HeatTile: return FLinearColor(0.06f, 0.06f, 0.07f);
	case ECapePaint::DarkTrim: return FLinearColor(0.16f, 0.16f, 0.18f);
	case ECapePaint::Wood:     return FLinearColor(0.38f, 0.24f, 0.12f);
	default:                   return FLinearColor::White;
	}
}

float ACapeCampus::RoughnessOf(ECapePaint Paint) const
{
	// Colour alone does not tell you what a thing is made of. Under one sun,
	// thirteen surfaces at identical roughness read as thirteen shades of the
	// same plastic — which is most of what makes a blockout look like a
	// blockout, quite apart from the shapes.
	switch (Paint)
	{
	case ECapePaint::Concrete: return 0.94f;
	case ECapePaint::Beige:    return 0.88f;
	case ECapePaint::HeatTile: return 0.86f;
	case ECapePaint::Wood:     return 0.80f;
	case ECapePaint::White:    return 0.74f;
	case ECapePaint::Black:    return 0.68f;
	case ECapePaint::Orange:   return 0.60f;
	case ECapePaint::Red:      return 0.58f;
	case ECapePaint::NasaBlue: return 0.54f;
	case ECapePaint::Yellow:   return 0.52f;
	case ECapePaint::DarkTrim: return 0.42f;
	case ECapePaint::Steel:    return 0.30f;
	case ECapePaint::Glass:    return 0.06f;
	default:                   return 0.72f;
	}
}

float ACapeCampus::MetallicOf(ECapePaint Paint) const
{
	// Steel is the only true metal here. Painted steel is paint, and reads as
	// paint — making the hull metallic and the flaps metallic but the orange
	// stripe on them not is exactly the distinction that sells it.
	return Paint == ECapePaint::Steel ? 1.0f : 0.0f;
}

UInstancedStaticMeshComponent* ACapeCampus::GetPaint(ECapePaint Paint, ECapeBrush Brush)
{
	const int32 Key = static_cast<int32>(Paint) * BrushCount + static_cast<int32>(Brush);
	if (PaintBuckets.IsValidIndex(Key) && PaintBuckets[Key])
	{
		return PaintBuckets[Key];
	}

	if (PaintBuckets.Num() < PaintCount * BrushCount)
	{
		PaintBuckets.SetNum(PaintCount * BrushCount);
	}

	UStaticMesh* Mesh = FallbackCube;
	if (Brush == ECapeBrush::Tube) { Mesh = FallbackCylinder; }
	else if (Brush == ECapeBrush::Cone) { Mesh = FallbackCone; }

	UInstancedStaticMeshComponent* Comp = NewObject<UInstancedStaticMeshComponent>(this);
	if (!Comp)
	{
		return nullptr;
	}

	Comp->SetupAttachment(GetRootComponent());
	Comp->SetMobility(EComponentMobility::Movable);
	if (Mesh)
	{
		Comp->SetStaticMesh(Mesh);
	}

	const bool bCollides = (Paint != ECapePaint::Glass && Paint != ECapePaint::HeatTile);
	if (bCollides)
	{
		Comp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Comp->SetCollisionResponseToAllChannels(ECR_Block);
	}
	else
	{
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	Comp->SetCastShadow(bCollides);

	if (TintBaseMaterial)
	{
		if (UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(TintBaseMaterial, this))
		{
			Mid->SetVectorParameterValue(TEXT("Color"), ColorOf(Paint));
			Mid->SetScalarParameterValue(TEXT("Roughness"), RoughnessOf(Paint));
			Mid->SetScalarParameterValue(TEXT("Metallic"), MetallicOf(Paint));
			// Glass is the only paint that lights itself: an unlit window in a
			// lit facade reads as a hole, and these have no interior behind them.
			Mid->SetScalarParameterValue(TEXT("Emissive"),
				Paint == ECapePaint::Glass ? 0.55f : 0.0f);
			Comp->SetMaterial(0, Mid);
		}
	}

	Comp->RegisterComponent();
	AddInstanceComponent(Comp);
	Generated.Add(Comp);
	PaintBuckets[Key] = Comp;
	return Comp;
}

void ACapeCampus::PaintBox(ECapePaint Paint, const FVector& Center, const FVector& Size, float YawDegrees)
{
	AddBoxTo(GetPaint(Paint, ECapeBrush::Box), Center, Size, YawDegrees);
}

void ACapeCampus::PaintTube(ECapePaint Paint, const FVector& BaseCenter, float DiameterCm, float HeightCm)
{
	AddCylinderTo(GetPaint(Paint, ECapeBrush::Tube), BaseCenter, DiameterCm, HeightCm);
}

void ACapeCampus::PaintCone(ECapePaint Paint, const FVector& BaseCenter, float DiameterCm, float HeightCm)
{
	AddConeTo(GetPaint(Paint, ECapeBrush::Cone), BaseCenter, DiameterCm, HeightCm);
}

ECapePaint ACapeCampus::BodyPaintFor(const FCapeBuilding& Building) const
{
	if (Building.Id == TEXT("HQ")) { return ECapePaint::Beige; }
	if (Building.Id == TEXT("VAB")) { return ECapePaint::White; }
	return ECapePaint::White;
}

void ACapeCampus::AddFillLight(const FVector& Local, float Intensity, const FLinearColor& Color, float Radius)
{
	UPointLightComponent* Light = NewObject<UPointLightComponent>(this);
	if (!Light)
	{
		return;
	}

	Light->SetupAttachment(GetRootComponent());
	Light->RegisterComponent();
	AddInstanceComponent(Light);
	Light->SetMobility(EComponentMobility::Movable);
	Light->SetRelativeLocation(Local);
	Light->SetIntensity(Intensity);
	Light->SetLightColor(Color);
	Light->SetAttenuationRadius(Radius);
	Light->SetCastShadows(false);
	Generated.Add(Light);
}

void ACapeCampus::AddBelt(const FCapeBuilding& Building, float Z, float Height, ECapePaint Paint)
{
	const FVector& C = Building.Center;
	const FVector& S = Building.Size;
	const float T = Building.WallThickness + 16.0f;
	const float HalfX = S.X * 0.5f;
	const float HalfY = S.Y * 0.5f;

	// A belt is a solid bar standing ~46 cm proud of the wall, and it collides.
	// Run one across the door face at trim height and it is a barricade at shin
	// level across the only way in — which is what the low DarkTrim bands on
	// Mission Control, JPL and Admin were, and the VAB's lower black band too.
	// So on the door face the band is punched the same way the wall itself is.
	const float Head = Building.DoorHeight + 40.0f;
	const float Bottom = Z - Height * 0.5f;
	const float Top = Z + Height * 0.5f;

	auto Band = [&](ECapeDoorSide Side, const FVector& Center, const FVector& Size, bool bAlongY)
	{
		if (Side != Building.DoorSide || Bottom >= Head)
		{
			PaintBox(Paint, Center, Size);
			return;
		}

		const float SpanLength = bAlongY ? Size.Y : Size.X;
		const float Clear = FMath::Min(Building.DoorWidth + 80.0f, SpanLength - 100.0f);
		const float JambLength = FMath::Max((SpanLength - Clear) * 0.5f, 0.0f);

		if (JambLength > 0.0f)
		{
			const FVector Axis = bAlongY ? FVector(0, 1, 0) : FVector(1, 0, 0);
			const float Offset = (SpanLength - JambLength) * 0.5f;
			const FVector JambSize = bAlongY
				? FVector(Size.X, JambLength, Size.Z)
				: FVector(JambLength, Size.Y, Size.Z);

			PaintBox(Paint, Center + Axis * Offset, JambSize);
			PaintBox(Paint, Center - Axis * Offset, JambSize);
		}

		// Whatever part of the band clears the door head still spans the opening,
		// so a tall belt keeps its line unbroken across the facade.
		if (Top > Head)
		{
			const float LintelHeight = Top - Head;
			const FVector LintelSize = bAlongY
				? FVector(Size.X, Clear, LintelHeight)
				: FVector(Clear, Size.Y, LintelHeight);

			PaintBox(Paint, FVector(Center.X, Center.Y, Head + LintelHeight * 0.5f), LintelSize);
		}
	};

	Band(ECapeDoorSide::PlusX, C + FVector(HalfX + 8.0f, 0.0f, Z),
		FVector(T, S.Y + 20.0f, Height), /*bAlongY=*/true);
	Band(ECapeDoorSide::MinusX, C + FVector(-(HalfX + 8.0f), 0.0f, Z),
		FVector(T, S.Y + 20.0f, Height), /*bAlongY=*/true);
	Band(ECapeDoorSide::PlusY, C + FVector(0.0f, HalfY + 8.0f, Z),
		FVector(S.X + 20.0f, T, Height), /*bAlongY=*/false);
	Band(ECapeDoorSide::MinusY, C + FVector(0.0f, -(HalfY + 8.0f), Z),
		FVector(S.X + 20.0f, T, Height), /*bAlongY=*/false);
}

void ACapeCampus::AddWindowGrid(const FCapeBuilding& Building, ECapeDoorSide Face,
	int32 Cols, int32 Rows, float Z0, float Z1)
{
	if (Cols <= 0 || Rows <= 0 || Z1 <= Z0)
	{
		return;
	}

	const FVector& C = Building.Center;
	const FVector& S = Building.Size;
	const float Depth = 14.0f;
	// Clear of the 12 cm facade skin AddFacade lays on the wall. Sitting a pane
	// half inside that skin is two coplanar surfaces fighting over every pixel,
	// which flickers as the camera moves.
	const float Proud = 22.0f;
	const float Margin = 280.0f;
	const float ZSpan = Z1 - Z0;
	const float CellH = ZSpan / Rows;
	const float WinH = CellH * 0.62f;

	auto Place = [&](float FacePos, bool bAlongY, float Width)
	{
		const float Inner = Width - 2.0f * Margin;
		if (Inner <= 0.0f)
		{
			return;
		}
		const float CellW = Inner / Cols;
		const float WinW = CellW * 0.70f;

		for (int32 Col = 0; Col < Cols; ++Col)
		{
			const float Along = -Inner * 0.5f + CellW * (Col + 0.5f);
			for (int32 Row = 0; Row < Rows; ++Row)
			{
				const float Z = Z0 + CellH * (Row + 0.5f);

				// Leave the doorway clear so a window does not sit in the hole.
				if (Face == Building.DoorSide && Z < Building.DoorHeight + 80.0f
					&& FMath::Abs(Along) < Building.DoorWidth * 0.5f + 80.0f)
				{
					continue;
				}

				if (bAlongY)
				{
					PaintBox(ECapePaint::Glass,
						FVector(FacePos, C.Y + Along, Z),
						FVector(Depth, WinW, WinH));
				}
				else
				{
					PaintBox(ECapePaint::Glass,
						FVector(C.X + Along, FacePos, Z),
						FVector(WinW, Depth, WinH));
				}
			}
		}
	};

	switch (Face)
	{
	case ECapeDoorSide::PlusX:
		Place(C.X + S.X * 0.5f + Proud, true, S.Y);
		break;
	case ECapeDoorSide::MinusX:
		Place(C.X - S.X * 0.5f - Proud, true, S.Y);
		break;
	case ECapeDoorSide::PlusY:
		Place(C.Y + S.Y * 0.5f + Proud, false, S.X);
		break;
	case ECapeDoorSide::MinusY:
		Place(C.Y - S.Y * 0.5f - Proud, false, S.X);
		break;
	default:
		break;
	}
}

void ACapeCampus::AddFlagOnFace(const FCapeBuilding& Building, ECapeDoorSide Face,
	float Width, float Height, float MidZ)
{
	// Stylized Stars-and-Stripes. Thirteen fat stripes, a blue canton, a
	// handful of white "stars" as dots. Fortnite, not a sewing pattern.
	const FVector& C = Building.Center;
	const FVector& S = Building.Size;
	constexpr int32 Stripes = 13;
	const float StripeH = Height / Stripes;
	const float CantonW = Width * 0.40f;
	const float CantonH = StripeH * 7.0f;

	// Clear of the facade skin, same reason as the window grids.
	constexpr float Proud = 26.0f;

	FVector Origin;
	bool bAlongY = false;
	switch (Face)
	{
	case ECapeDoorSide::PlusX:
		Origin = FVector(C.X + S.X * 0.5f + Proud, C.Y, MidZ);
		bAlongY = true;
		break;
	case ECapeDoorSide::MinusX:
		Origin = FVector(C.X - S.X * 0.5f - Proud, C.Y, MidZ);
		bAlongY = true;
		break;
	case ECapeDoorSide::PlusY:
		Origin = FVector(C.X, C.Y + S.Y * 0.5f + Proud, MidZ);
		break;
	case ECapeDoorSide::MinusY:
		Origin = FVector(C.X, C.Y - S.Y * 0.5f - Proud, MidZ);
		break;
	default:
		return;
	}

	for (int32 I = 0; I < Stripes; ++I)
	{
		const float Z = MidZ - Height * 0.5f + StripeH * (I + 0.5f);
		const bool bRed = (I % 2) == 0;
		const bool bUnderCanton = (Z > MidZ + Height * 0.5f - CantonH);
		const float StripeW = bUnderCanton ? (Width - CantonW) : Width;
		const float Shift = bUnderCanton ? (CantonW * 0.5f) : 0.0f;

		if (bAlongY)
		{
			PaintBox(bRed ? ECapePaint::Red : ECapePaint::White,
				FVector(Origin.X, Origin.Y + Shift, Z),
				FVector(16.0f, StripeW, StripeH * 0.92f));
		}
		else
		{
			PaintBox(bRed ? ECapePaint::Red : ECapePaint::White,
				FVector(Origin.X + Shift, Origin.Y, Z),
				FVector(StripeW, 16.0f, StripeH * 0.92f));
		}
	}

	const float CantonZ = MidZ + Height * 0.5f - CantonH * 0.5f;
	const float CantonAlong = -Width * 0.5f + CantonW * 0.5f;
	if (bAlongY)
	{
		PaintBox(ECapePaint::NasaBlue,
			FVector(Origin.X, Origin.Y + CantonAlong, CantonZ),
			FVector(18.0f, CantonW, CantonH));
	}
	else
	{
		PaintBox(ECapePaint::NasaBlue,
			FVector(Origin.X + CantonAlong, Origin.Y, CantonZ),
			FVector(CantonW, 18.0f, CantonH));
	}

	// A 4x5 grid of white dots stands in for the stars.
	for (int32 Row = 0; Row < 5; ++Row)
	{
		for (int32 Col = 0; Col < 4; ++Col)
		{
			const float U = (Col + 0.5f) / 4.0f;
			const float V = (Row + 0.5f) / 5.0f;
			const float Along = CantonAlong - CantonW * 0.5f + U * CantonW;
			const float Z = CantonZ - CantonH * 0.5f + V * CantonH;
			// 14, not 8: the canton is 18 thick, so a 10-thick dot at 8 sits
			// half-buried in it and the stars flicker instead of reading.
			if (bAlongY)
			{
				PaintBox(ECapePaint::White,
					FVector(Origin.X + 14.0f, Origin.Y + Along, Z),
					FVector(10.0f, CantonW * 0.10f, CantonH * 0.08f));
			}
			else
			{
				PaintBox(ECapePaint::White,
					FVector(Origin.X + Along, Origin.Y + 14.0f, Z),
					FVector(CantonW * 0.10f, 10.0f, CantonH * 0.08f));
			}
		}
	}
}

void ACapeCampus::AddHeatTiles(const FVector& Base, float Diameter, float Z0, float Z1, float YawCenter)
{
	// Chunky black tiles on the windward side. Fortnite, so they are big
	// enough to read from the road — not a thousand real hexagonal tiles.
	const int32 Cols = 7;
	const int32 Rows = FMath::Max(6, FMath::RoundToInt((Z1 - Z0) / 180.0f));
	const float Radius = Diameter * 0.5f + 10.0f;
	const float TileH = (Z1 - Z0) / Rows;

	for (int32 Row = 0; Row < Rows; ++Row)
	{
		for (int32 Col = 0; Col < Cols; ++Col)
		{
			const float Angle = FMath::Lerp(-70.0f, 70.0f, Col / FMath::Max(1.0f, Cols - 1.0f));
			const float Yaw = YawCenter + Angle;
			const float Rad = FMath::DegreesToRadians(Yaw);
			const float Z = Z0 + TileH * (Row + 0.5f);
			PaintBox(ECapePaint::HeatTile,
				Base + FVector(FMath::Cos(Rad) * Radius, FMath::Sin(Rad) * Radius, Z),
				FVector(16.0f, 110.0f, TileH * 0.88f),
				Yaw);
		}
	}
}

void ACapeCampus::AddFacade(const FCapeBuilding& Building, ECapePaint Paint)
{
	// The shells in CapeCampus.cpp are all one concrete grey, which is the single
	// biggest reason the campus reads as a blockout. A 12 cm skin flush against
	// each wall costs seven instances and gives every building a body colour
	// without touching the layout.
	//
	// The door face goes through AddWallTo so the opening stays an opening.
	const FVector& C = Building.Center;
	const FVector& S = Building.Size;
	const float MidZ = S.Z * 0.5f;
	constexpr float Skin = 12.0f;

	auto Wall = [&](ECapeDoorSide Side, const FVector& Center, const FVector& Size, bool bAlongY)
	{
		AddWallTo(GetPaint(Paint), Center, Size, bAlongY,
			/*bWithDoor=*/Side == Building.DoorSide,
			Building.DoorWidth + 40.0f, Building.DoorHeight + 40.0f);
	};

	Wall(ECapeDoorSide::PlusX, C + FVector(S.X * 0.5f + Skin * 0.5f, 0.0f, MidZ),
		FVector(Skin, S.Y, S.Z), /*bAlongY=*/true);
	Wall(ECapeDoorSide::MinusX, C + FVector(-(S.X * 0.5f + Skin * 0.5f), 0.0f, MidZ),
		FVector(Skin, S.Y, S.Z), /*bAlongY=*/true);
	Wall(ECapeDoorSide::PlusY, C + FVector(0.0f, S.Y * 0.5f + Skin * 0.5f, MidZ),
		FVector(S.X, Skin, S.Z), /*bAlongY=*/false);
	Wall(ECapeDoorSide::MinusY, C + FVector(0.0f, -(S.Y * 0.5f + Skin * 0.5f), MidZ),
		FVector(S.X, Skin, S.Z), /*bAlongY=*/false);
}

void ACapeCampus::DressBuildingExterior(const FCapeBuilding& Building)
{
	AddFacade(Building, BodyPaintFor(Building));

	if (Building.Id == TEXT("VAB")) { DressVabExterior(Building); }
	else if (Building.Id == TEXT("MCC")) { DressMccExterior(Building); }
	else if (Building.Id == TEXT("RND")) { DressJplExterior(Building); }
	else if (Building.Id == TEXT("HQ")) { DressAdminExterior(Building); }
}

void ACapeCampus::DressVabExterior(const FCapeBuilding& Building)
{
	// The Kennedy VAB: white box, black bands, the flag on the plaza face,
	// a yellow high-bay door you could drive a booster through.
	AddBelt(Building, Building.Size.Z * 0.22f, 180.0f, ECapePaint::Black);
	AddBelt(Building, Building.Size.Z * 0.55f, 140.0f, ECapePaint::Black);
	AddBelt(Building, Building.Size.Z * 0.88f, 220.0f, ECapePaint::NasaBlue);

	// Plaza-facing flag (+Y). Door is on -Y, so this wall is the billboard.
	AddFlagOnFace(Building, ECapeDoorSide::PlusY, 3600.0f, 1900.0f, Building.Size.Z * 0.70f);

	// High-bay door: a giant yellow FRAME around the real opening — a solid
	// slab here would brick the doorway.
	{
		const FVector DoorOut = Building.Center + EntranceOffsetFor(Building);
		const FVector Inward = (Building.Center - DoorOut).GetSafeNormal();
		const float Depth = Building.Size.Y; // VAB door is on a Y face
		const FVector Face = Building.Center - Inward * (Depth * 0.5f + 20.0f);
		AddWallTo(GetPaint(ECapePaint::Yellow),
			Face + FVector(0.0f, 0.0f, Building.DoorHeight * 0.5f + 80.0f),
			FVector(Building.DoorWidth + 280.0f, 28.0f, Building.DoorHeight + 200.0f),
			/*bAlongY=*/false, /*bWithDoor=*/true,
			Building.DoorWidth, Building.DoorHeight);
	}

	// Side window slits — the VAB is mostly blank, a few punched openings.
	AddWindowGrid(Building, ECapeDoorSide::PlusX, 3, 6, 800.0f, Building.Size.Z - 600.0f);
	AddWindowGrid(Building, ECapeDoorSide::MinusX, 3, 6, 800.0f, Building.Size.Z - 600.0f);

	// Roof-edge "USA" colour blocks on the corners so the silhouette is not a
	// plain white slab from the pad.
	const float CornerZ = Building.Size.Z - 200.0f;
	for (int32 SX = -1; SX <= 1; SX += 2)
	{
		for (int32 SY = -1; SY <= 1; SY += 2)
		{
			PaintBox(ECapePaint::NasaBlue,
				Building.Center + FVector(SX * (Building.Size.X * 0.5f - 200.0f),
					SY * (Building.Size.Y * 0.5f - 200.0f), CornerZ),
				FVector(360.0f, 360.0f, 280.0f));
		}
	}
}

void ACapeCampus::DressMccExterior(const FCapeBuilding& Building)
{
	// Mission Control: white box, NASA-blue belt, ribbon windows, dishes on
	// the roof. The screen wall (-Y) stays mostly blank so the interior
	// projection wall is not a window.
	AddBelt(Building, Building.Size.Z * 0.78f, 260.0f, ECapePaint::NasaBlue);
	AddBelt(Building, 180.0f, 80.0f, ECapePaint::DarkTrim);

	AddWindowGrid(Building, ECapeDoorSide::PlusX, 6, 2, 280.0f, Building.Size.Z - 400.0f);
	AddWindowGrid(Building, ECapeDoorSide::MinusX, 6, 2, 280.0f, Building.Size.Z - 400.0f);
	AddWindowGrid(Building, ECapeDoorSide::PlusY, 8, 2, 280.0f, Building.Size.Z - 400.0f);

	// Entrance canopy.
	{
		const FVector Out = Building.Center + EntranceOffsetFor(Building);
		PaintBox(ECapePaint::NasaBlue, Out + FVector(0.0f, 0.0f, Building.DoorHeight + 40.0f),
			FVector(900.0f, 700.0f, 50.0f));
		PaintBox(ECapePaint::DarkTrim, Out + FVector(-350.0f, 0.0f, Building.DoorHeight * 0.5f),
			FVector(40.0f, 40.0f, Building.DoorHeight));
		PaintBox(ECapePaint::DarkTrim, Out + FVector(350.0f, 0.0f, Building.DoorHeight * 0.5f),
			FVector(40.0f, 40.0f, Building.DoorHeight));
	}

	// Roof antenna farm — the thing that makes it read as a control centre
	// from across the plaza.
	const FVector Roof = Building.Center + FVector(0.0f, 0.0f, Building.Size.Z);
	PaintTube(ECapePaint::White, Roof + FVector(-800.0f, -400.0f, 0.0f), 80.0f, 900.0f);
	PaintCone(ECapePaint::Steel, Roof + FVector(-800.0f, -400.0f, 900.0f), 420.0f, 180.0f);
	PaintTube(ECapePaint::White, Roof + FVector(600.0f, 500.0f, 0.0f), 60.0f, 700.0f);
	PaintCone(ECapePaint::Steel, Roof + FVector(600.0f, 500.0f, 700.0f), 320.0f, 140.0f);
	PaintBox(ECapePaint::DarkTrim, Roof + FVector(200.0f, -600.0f, 80.0f), FVector(400.0f, 280.0f, 160.0f));
}

void ACapeCampus::DressJplExterior(const FCapeBuilding& Building)
{
	// JPL: mid-century white lab, a fat orange identity belt, lots of glass,
	// rooftop HVAC and a dish garden. This is the "we build spacecraft here"
	// building, not another grey office.
	AddBelt(Building, Building.Size.Z * 0.62f, 220.0f, ECapePaint::Orange);
	AddBelt(Building, 140.0f, 60.0f, ECapePaint::DarkTrim);

	AddWindowGrid(Building, ECapeDoorSide::PlusX, 7, 3, 220.0f, Building.Size.Z - 280.0f);
	AddWindowGrid(Building, ECapeDoorSide::MinusX, 7, 3, 220.0f, Building.Size.Z - 280.0f);
	AddWindowGrid(Building, ECapeDoorSide::PlusY, 8, 3, 220.0f, Building.Size.Z - 280.0f);
	AddWindowGrid(Building, ECapeDoorSide::MinusY, 8, 3, 220.0f, Building.Size.Z - 280.0f);

	// Entrance canopy in orange.
	{
		const FVector Out = Building.Center + EntranceOffsetFor(Building);
		PaintBox(ECapePaint::Orange, Out + FVector(0.0f, 0.0f, Building.DoorHeight + 30.0f),
			FVector(1000.0f, 800.0f, 40.0f));
		PaintTube(ECapePaint::White, Out + FVector(-380.0f, 0.0f, 0.0f), 50.0f, Building.DoorHeight);
		PaintTube(ECapePaint::White, Out + FVector(380.0f, 0.0f, 0.0f), 50.0f, Building.DoorHeight);
	}

	// Rooftop: HVAC boxes and a couple of tracking dishes.
	const FVector Roof = Building.Center + FVector(0.0f, 0.0f, Building.Size.Z);
	PaintBox(ECapePaint::DarkTrim, Roof + FVector(-900.0f, -500.0f, 90.0f), FVector(500.0f, 360.0f, 180.0f));
	PaintBox(ECapePaint::DarkTrim, Roof + FVector(-900.0f, 400.0f, 70.0f), FVector(400.0f, 300.0f, 140.0f));
	PaintTube(ECapePaint::Steel, Roof + FVector(700.0f, -200.0f, 0.0f), 70.0f, 500.0f);
	PaintCone(ECapePaint::White, Roof + FVector(700.0f, -200.0f, 500.0f), 380.0f, 160.0f);
	PaintTube(ECapePaint::Steel, Roof + FVector(1100.0f, 600.0f, 0.0f), 50.0f, 360.0f);
	PaintCone(ECapePaint::White, Roof + FVector(1100.0f, 600.0f, 360.0f), 240.0f, 110.0f);
}

void ACapeCampus::DressAdminExterior(const FCapeBuilding& Building)
{
	// Regular NASA office: beige body, punched window grid, a colonnade at
	// the door, a flagpole. No sci-fi, no high-bay — this is where the
	// budget lives.
	AddBelt(Building, Building.Size.Z * 0.92f, 120.0f, ECapePaint::NasaBlue);
	AddBelt(Building, 160.0f, 50.0f, ECapePaint::DarkTrim);

	AddWindowGrid(Building, ECapeDoorSide::PlusX, 5, 3, 260.0f, Building.Size.Z - 240.0f);
	AddWindowGrid(Building, ECapeDoorSide::MinusX, 5, 3, 260.0f, Building.Size.Z - 240.0f);
	AddWindowGrid(Building, ECapeDoorSide::PlusY, 6, 3, 260.0f, Building.Size.Z - 240.0f);
	AddWindowGrid(Building, ECapeDoorSide::MinusY, 6, 3, 260.0f, Building.Size.Z - 240.0f);

	// Colonnade. Admin's door is on -X, so the facade runs along Y.
	{
		const FVector Out = Building.Center + EntranceOffsetFor(Building);
		PaintBox(ECapePaint::Beige, Out + FVector(-200.0f, 0.0f, Building.DoorHeight + 80.0f),
			FVector(900.0f, 1600.0f, 70.0f));
		for (int32 I = -2; I <= 2; ++I)
		{
			// No pillar on the centre line — it would stand in the doorway.
			if (I == 0)
			{
				continue;
			}
			PaintTube(ECapePaint::White, Out + FVector(-80.0f, I * 280.0f, 0.0f),
				70.0f, Building.DoorHeight + 40.0f);
		}
	}

	// Flagpole beside the entrance, along the facade, not inside the building.
	{
		const FVector Pole = Building.Center + EntranceOffsetFor(Building) + FVector(-80.0f, 900.0f, 0.0f);
		PaintTube(ECapePaint::Steel, Pole, 18.0f, 1400.0f);
		PaintBox(ECapePaint::NasaBlue, Pole + FVector(-120.0f, 0.0f, 1280.0f), FVector(220.0f, 12.0f, 140.0f));
		PaintBox(ECapePaint::Red, Pole + FVector(-120.0f, 0.0f, 1160.0f), FVector(220.0f, 12.0f, 80.0f));
		PaintBox(ECapePaint::White, Pole + FVector(-120.0f, 0.0f, 1100.0f), FVector(220.0f, 12.0f, 40.0f));
	}
}

void ACapeCampus::DressPlaza()
{
	// A colour-blocked NASA meatball in the plaza: blue disc, red chevron,
	// white ring. Close enough to read as the logo from a Fortnite camera.
	PaintTube(ECapePaint::NasaBlue, FVector(0.0f, 0.0f, 8.0f), 1600.0f, 18.0f);
	PaintTube(ECapePaint::White, FVector(0.0f, 0.0f, 20.0f), 1180.0f, 16.0f);
	PaintTube(ECapePaint::NasaBlue, FVector(0.0f, 0.0f, 28.0f), 980.0f, 16.0f);
	PaintBox(ECapePaint::Red, FVector(80.0f, 0.0f, 50.0f), FVector(900.0f, 180.0f, 24.0f), 18.0f);
	PaintBox(ECapePaint::Red, FVector(200.0f, -80.0f, 50.0f), FVector(520.0f, 140.0f, 24.0f), -22.0f);
	PaintBox(ECapePaint::White, FVector(-180.0f, 220.0f, 50.0f), FVector(220.0f, 80.0f, 20.0f));

	// Planters at the four corners of the plaza apron.
	const float PlantR = 2400.0f;
	for (int32 I = 0; I < 4; ++I)
	{
		const float Yaw = 45.0f + I * 90.0f;
		const float Rad = FMath::DegreesToRadians(Yaw);
		const FVector P(FMath::Cos(Rad) * PlantR, FMath::Sin(Rad) * PlantR, 0.0f);
		PaintBox(ECapePaint::DarkTrim, P + FVector(0.0f, 0.0f, 40.0f), FVector(280.0f, 280.0f, 80.0f), Yaw);
		PaintBox(ECapePaint::White, P + FVector(0.0f, 0.0f, 90.0f), FVector(220.0f, 220.0f, 20.0f), Yaw);
		if (Foliage)
		{
			AddConeTo(Foliage, P + FVector(0.0f, 0.0f, 90.0f), 260.0f, 220.0f);
		}
	}

	// Benches facing the logo.
	for (int32 I = -1; I <= 1; I += 2)
	{
		PaintBox(ECapePaint::Wood, FVector(I * 1400.0f, 900.0f, 30.0f), FVector(360.0f, 70.0f, 60.0f));
		PaintBox(ECapePaint::Wood, FVector(I * 1400.0f, 930.0f, 70.0f), FVector(360.0f, 16.0f, 50.0f));
	}

	// Road centre line out toward the pad, so the 650 m walk has a graphic.
	const float RoadStartX = 3200.0f;
	const float RoadEndX = PadCenter.X - 6000.0f;
	for (float X = RoadStartX; X < RoadEndX; X += 800.0f)
	{
		PaintBox(ECapePaint::Yellow, FVector(X, 0.0f, 12.0f), FVector(280.0f, 30.0f, 6.0f));
	}
}
