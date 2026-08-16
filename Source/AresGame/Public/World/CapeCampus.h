// The space center, generated.
//
// Brief §4.2 asks for a small, dense, hand-authored campus: four buildings
// around a plaza, a road to a pad ~600 m out, and walking end to end taking
// about 90 seconds. §2 also says blockout geometry is something we generate,
// and §9 says no art passes on unproven mechanics.
//
// So the campus is built from code rather than placed by hand in a .umap. That
// buys three things:
//   - the layout is a diff, not a binary asset
//   - distances and proportions are numbers you can retune in the details panel
//   - it rebuilds in-editor on every change, no play-in-editor round trip
//
// It is deliberately NOT procedural in the roguelike sense: the layout is
// fixed and authored, it just happens to be authored in C++. Replacing these
// boxes with real meshes later does not change the layout data.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Terminals/AresTerminalWidget.h"

#include "CapeCampus.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPointLightComponent;
class UStaticMesh;
class UTextRenderComponent;

/** How an assigned mesh is fitted to the size the layout asks for. */
UENUM()
enum class ECapeFit : uint8
{
	/** Scale each axis independently. Right for a box-like mesh used as a wall. */
	Stretch,
	/** Uniform scale so the mesh's tallest axis matches. Right for props and trees. */
	Uniform,
	/** Place at authored size. Right for a mesh that is already the real thing. */
	Native,
};

/**
 * One assignable appearance.
 *
 * Everything the campus draws goes through one of these. Leave Mesh empty and
 * you get the engine primitive and a flat tint — the programmer-art fallback.
 * Assign a mesh and a material from Fab and the same layout code draws the same
 * campus with real assets, because the layout asks for "a wall 400 x 60 x 300
 * cm" and this works out the scale from the mesh's own bounds.
 *
 * That indirection is the whole point: art can be replaced without touching a
 * line of layout.
 */
USTRUCT(BlueprintType)
struct FCapeMeshSlot
{
	GENERATED_BODY()

	/** Drop a Fab / Quixel mesh here. Empty falls back to an engine primitive. */
	UPROPERTY(EditAnywhere, Category = "Ares|Cape|Art")
	TObjectPtr<UStaticMesh> Mesh;

	/** Overrides the mesh's own material. Empty keeps whatever the mesh ships with. */
	UPROPERTY(EditAnywhere, Category = "Ares|Cape|Art")
	TObjectPtr<UMaterialInterface> Material;

	/** Used only when no Mesh and no Material are assigned. */
	UPROPERTY(EditAnywhere, Category = "Ares|Cape|Art")
	FLinearColor FallbackTint = FLinearColor(0.42f, 0.42f, 0.44f);

	UPROPERTY(EditAnywhere, Category = "Ares|Cape|Art")
	ECapeFit Fit = ECapeFit::Stretch;

	/** Extra scale after fitting. Handy for dialling a Fab asset in by eye. */
	UPROPERTY(EditAnywhere, Category = "Ares|Cape|Art")
	FVector ExtraScale = FVector::OneVector;

	/** Random yaw per instance. Kills the repetition on scattered props. */
	UPROPERTY(EditAnywhere, Category = "Ares|Cape|Art")
	bool bRandomYaw = false;
};

/**
 * Paint colours for the exterior dressing pass.
 *
 * Order is load-bearing: CapeCampusDressing.cpp derives its bucket count from
 * the LAST entry, so Wood must stay last and nothing may be inserted without
 * checking that file.
 */
UENUM()
enum class ECapePaint : uint8
{
	Concrete,
	White,
	NasaBlue,
	Orange,
	Black,
	Beige,
	Steel,
	Red,
	Yellow,
	Glass,
	HeatTile,
	DarkTrim,
	Wood,
};

/** Primitive a painted instance is drawn with. Cone must stay last (see above). */
UENUM()
enum class ECapeBrush : uint8
{
	Box,
	Tube,
	Cone,
};

/** Which wall a building's doorway is cut into. */
UENUM()
enum class ECapeDoorSide : uint8
{
	PlusX,
	MinusX,
	PlusY,
	MinusY,
};

/** One walkable shell: floor, four walls with a doorway, and a roof. */
USTRUCT(BlueprintType)
struct FCapeBuilding
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Ares|Cape")
	FName Id;

	UPROPERTY(EditAnywhere, Category = "Ares|Cape")
	FText DisplayName;

	/** Centre of the footprint, cm, relative to the campus actor. */
	UPROPERTY(EditAnywhere, Category = "Ares|Cape")
	FVector Center = FVector::ZeroVector;

	/** Exterior extents, cm. Z is the interior clear height. */
	UPROPERTY(EditAnywhere, Category = "Ares|Cape")
	FVector Size = FVector(4000.0f, 3000.0f, 1200.0f);

	UPROPERTY(EditAnywhere, Category = "Ares|Cape")
	float WallThickness = 60.0f;

	UPROPERTY(EditAnywhere, Category = "Ares|Cape")
	float DoorWidth = 400.0f;

	UPROPERTY(EditAnywhere, Category = "Ares|Cape")
	float DoorHeight = 320.0f;

	UPROPERTY(EditAnywhere, Category = "Ares|Cape")
	ECapeDoorSide DoorSide = ECapeDoorSide::MinusX;

	/** The VAB is left open-topped so its volume reads from inside. */
	UPROPERTY(EditAnywhere, Category = "Ares|Cape")
	bool bRoof = true;

	/** Brief §4.2 gives every building exactly one console. */
	UPROPERTY(EditAnywhere, Category = "Ares|Cape")
	bool bHasTerminal = true;

	UPROPERTY(EditAnywhere, Category = "Ares|Cape")
	ETerminalKind TerminalKind = ETerminalKind::MissionControl;
};

UCLASS()
class ARESGAME_API ACapeCampus : public AActor
{
	GENERATED_BODY()

public:
	ACapeCampus();

	virtual void BeginPlay() override;

	/** Rebuilds the whole campus. Runs in-editor on every property change. */
	virtual void OnConstruction(const FTransform& Transform) override;

	/** World-space point just outside a building's doorway, for spawn/nav use. */
	UFUNCTION(BlueprintPure, Category = "Ares|Cape")
	FVector GetBuildingEntrance(FName BuildingId) const;

protected:
	/*
	 * One instanced component per material, because an ISM draws with a single
	 * material. Splitting by colour rather than by object type is what lets the
	 * whole campus stay a handful of draw calls while still reading as a place
	 * rather than a grey box.
	 */

	/** Structure: walls, floors, slabs. Concrete grey. */
	UPROPERTY(VisibleAnywhere, Category = "Ares|Cape")
	TObjectPtr<UInstancedStaticMeshComponent> Boxes;

	/** Tanks, rocket bodies, tree trunks. */
	UPROPERTY(VisibleAnywhere, Category = "Ares|Cape")
	TObjectPtr<UInstancedStaticMeshComponent> Cylinders;

	/** Console furniture, desks, equipment racks. Near-black. */
	UPROPERTY(VisibleAnywhere, Category = "Ares|Cape")
	TObjectPtr<UInstancedStaticMeshComponent> Furniture;

	/** Screens and the front projection wall. Cool blue, and it glows. */
	UPROPERTY(VisibleAnywhere, Category = "Ares|Cape")
	TObjectPtr<UInstancedStaticMeshComponent> Screens;

	/*
	 * Steel needs three components, not one: an instanced mesh is a single mesh
	 * AND a single material, so a steel barrel, a steel flap and a steel
	 * nosecone are three different draws no matter how they are tinted.
	 */

	/** Barrels: Starship, Super Heavy, spacecraft buses. */
	UPROPERTY(VisibleAnywhere, Category = "Ares|Cape")
	TObjectPtr<UInstancedStaticMeshComponent> Steel;

	/** Boxy steel: flaps, grid fins, landing legs, the crane hook. */
	UPROPERTY(VisibleAnywhere, Category = "Ares|Cape")
	TObjectPtr<UInstancedStaticMeshComponent> SteelBox;

	/** Nosecones and dishes. */
	UPROPERTY(VisibleAnywhere, Category = "Ares|Cape")
	TObjectPtr<UInstancedStaticMeshComponent> SteelCone;

	/** Ground cover. */
	UPROPERTY(VisibleAnywhere, Category = "Ares|Cape")
	TObjectPtr<UInstancedStaticMeshComponent> Grass;

	/** Tree canopies. */
	UPROPERTY(VisibleAnywhere, Category = "Ares|Cape")
	TObjectPtr<UInstancedStaticMeshComponent> Foliage;

	/**
	 * Ground cover — actual grass clumps, not a grass texture.
	 *
	 * Hierarchical, because this carries tens of thousands of instances and
	 * needs the per-cluster culling a plain ISM does not do.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Ares|Cape")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> GroundCover;

	UPROPERTY(EditAnywhere, Category = "Ares|Cape")
	TArray<FCapeBuilding> Buildings;

	/** Ground slab extents, cm. Generous enough to cover campus and pad. */
	UPROPERTY(EditAnywhere, Category = "Ares|Cape")
	FVector GroundSize = FVector(200000.0f, 120000.0f, 100.0f);

	/** Pad centre, cm. Brief §4.2 puts it ~600 m from the plaza. */
	UPROPERTY(EditAnywhere, Category = "Ares|Cape")
	FVector PadCenter = FVector(65000.0f, 0.0f, 0.0f);

	UPROPERTY(EditAnywhere, Category = "Ares|Cape")
	float RoadWidth = 1800.0f;

	/** Draws floating building names. Invaluable in an all-grey blockout. */
	UPROPERTY(EditAnywhere, Category = "Ares|Cape")
	bool bShowSigns = true;

	/**
	 * Interior lighting. A roofed blockout box is pitch black inside — the sun
	 * cannot reach it and there is nothing else emitting. Without these you walk
	 * into Mission Control and see nothing at all.
	 */
	UPROPERTY(EditAnywhere, Category = "Ares|Cape")
	bool bInteriorLights = true;

	/** Lumens per interior light, scaled up for larger rooms. */
	UPROPERTY(EditAnywhere, Category = "Ares|Cape")
	float InteriorLightIntensity = 60000.0f;

	/** Fit out the buildings. Off gives you the M1 empty shells. */
	UPROPERTY(EditAnywhere, Category = "Ares|Cape")
	bool bBuildInteriors = true;

	/** Colour-blocked exterior facades, flags, window grids and heat tiles. */
	UPROPERTY(EditAnywhere, Category = "Ares|Cape")
	bool bDressExteriors = true;

	/** Stack Super Heavy and Starship on the pad. */
	UPROPERTY(EditAnywhere, Category = "Ares|Cape")
	bool bBuildLaunchVehicle = true;

	/** Grass and trees around the campus. */
	UPROPERTY(EditAnywhere, Category = "Ares|Cape")
	bool bBuildLandscape = true;

	UPROPERTY(EditAnywhere, Category = "Ares|Cape")
	int32 TreeCount = 260;

	/**
	 * Edge length of one ground tile, metres.
	 *
	 * Ground is drawn as a GRID of quads, not one big one, because a material
	 * tiles by UV: a single quad stretched across the whole map stretches its
	 * UVs with it, so any texture becomes one smeared texel per hundred metres.
	 * Smaller tiles repeat the texture more often and cost more instances.
	 * 25 m is a reasonable balance; drop to 10 for close-up detail.
	 */
	UPROPERTY(EditAnywhere, Category = "Ares|Cape", meta = (ClampMin = "4"))
	float GroundTileSizeM = 25.0f;

	/**
	 * How many ground-cover clumps to scatter. Only used once GroundCoverSlot
	 * has a mesh. 30k covers the walkable campus convincingly; it is instanced,
	 * so the cost is mostly in the cull pass rather than the draw.
	 */
	UPROPERTY(EditAnywhere, Category = "Ares|Cape", meta = (ClampMin = "0"))
	int32 GroundCoverCount = 30000;

	/** Radius around the campus centre to scatter within, metres. */
	UPROPERTY(EditAnywhere, Category = "Ares|Cape")
	float GroundCoverRadiusM = 700.0f;

	/** Beyond this, clumps stop drawing. Grass is not worth a distant draw. */
	UPROPERTY(EditAnywhere, Category = "Ares|Cape")
	float GroundCoverCullDistanceM = 120.0f;

	/** Size of one clump, cm. Scaled per-instance by +/- 35% for variety. */
	UPROPERTY(EditAnywhere, Category = "Ares|Cape")
	float GroundCoverSizeCm = 60.0f;

	/** Bikes at the plaza rack, for the ride out to the pad. */
	UPROPERTY(EditAnywhere, Category = "Ares|Cape")
	int32 BikeCount = 4;

	/**
	 * Spawns an ACapeSky at play time if the level has none.
	 *
	 * A level without a sky is unlit and unusable, and losing the placed actor
	 * to an unsaved crash should not cost a session. Placing one by hand still
	 * wins — this only fills a gap.
	 */
	UPROPERTY(EditAnywhere, Category = "Ares|Cape")
	bool bEnsureSky = true;

	/* ----------------------------------------------------------------------
	 * Art slots.
	 *
	 * Assign Fab / Quixel meshes and materials here and the campus redraws
	 * itself with them. Nothing below changes any layout — the geometry code
	 * asks for sizes in centimetres and each slot works out its own scale.
	 * -------------------------------------------------------------------- */

	/** Walls, floors, slabs, platforms, the pad. */
	UPROPERTY(EditAnywhere, Category = "Ares|Cape|Art")
	FCapeMeshSlot StructureSlot;

	/** Desks, consoles, racks, chairs, benches. */
	UPROPERTY(EditAnywhere, Category = "Ares|Cape|Art")
	FCapeMeshSlot FurnitureSlot;

	/** Monitors and the projection wall. Give this an emissive material. */
	UPROPERTY(EditAnywhere, Category = "Ares|Cape|Art")
	FCapeMeshSlot ScreenSlot;

	/** Rocket barrels and spacecraft buses. Cylindrical. */
	UPROPERTY(EditAnywhere, Category = "Ares|Cape|Art")
	FCapeMeshSlot SteelBarrelSlot;

	/** Flaps, grid fins, legs, the crane hook. Boxy. */
	UPROPERTY(EditAnywhere, Category = "Ares|Cape|Art")
	FCapeMeshSlot SteelDetailSlot;

	/** Nosecones and dishes. Conical. */
	UPROPERTY(EditAnywhere, Category = "Ares|Cape|Art")
	FCapeMeshSlot NoseconeSlot;

	/** Ground cover. A landscape material belongs here. */
	UPROPERTY(EditAnywhere, Category = "Ares|Cape|Art")
	FCapeMeshSlot GroundSlot;

	/** Tree trunks — or the whole tree, if the Fab asset includes foliage. */
	UPROPERTY(EditAnywhere, Category = "Ares|Cape|Art")
	FCapeMeshSlot TrunkSlot;

	/** Canopies. Leave the mesh empty if TrunkSlot is a complete tree. */
	UPROPERTY(EditAnywhere, Category = "Ares|Cape|Art")
	FCapeMeshSlot CanopySlot;

	/**
	 * Grass clumps, flowers, small rocks — whatever should sit ON the ground
	 * rather than be painted on it.
	 *
	 * A material makes the ground LOOK like grass; only geometry makes it grass.
	 * Assign a Fab grass-clump mesh here. Nothing is scattered while this is
	 * empty, because tens of thousands of grey cubes would be worse than bare
	 * ground.
	 */
	UPROPERTY(EditAnywhere, Category = "Ares|Cape|Art")
	FCapeMeshSlot GroundCoverSlot;

	/** Skips canopy instances, for when TrunkSlot is a complete tree asset. */
	UPROPERTY(EditAnywhere, Category = "Ares|Cape|Art")
	bool bTrunkSlotIsWholeTree = false;

private:
	void BuildGroundAndRoad();
	void BuildBuilding(const FCapeBuilding& Building);
	void BuildPad();

	/* --- interiors. One per building, keyed off FCapeBuilding::Id. --- */
	void BuildInteriors();
	void BuildMissionControlInterior(const FCapeBuilding& B);
	void BuildResearchInterior(const FCapeBuilding& B);
	void BuildAdministrationInterior(const FCapeBuilding& B);
	void BuildVabInterior(const FCapeBuilding& B);

	/** Super Heavy plus Starship, stacked on the mount. */
	void BuildLaunchVehicle();

	/** Grass apron and scattered trees, kept clear of roads and structures. */
	void BuildLandscape();

	/**
	 * Fills an axis-aligned area with tiles of roughly TileSize, so an assigned
	 * material repeats at a usable density instead of being stretched over the
	 * whole surface.
	 */
	void AddTiledSurface(UInstancedStaticMeshComponent* Component, const FVector& Center,
		const FVector2D& AreaSize, float Thickness, float TileSizeCm);

	/** Scatters ground cover. No-op until GroundCoverSlot has a mesh. */
	void BuildGroundCover();

	/** True when a point is clear of roads, plaza, pad and buildings. */
	bool IsOpenGround(const FVector& LocalPoint) const;

	/** Bikes at the plaza. Spawned at BeginPlay alongside the terminals. */
	void SpawnBikes();

	/** Spawns a sky if the level has none. No-op when one already exists. */
	void EnsureSkyExists();

	/* ----------------------------------------------------------------------
	 * Exterior dressing — implemented in CapeCampusDressing.cpp.
	 *
	 * Colour-blocked facades so each building reads as itself from the plaza
	 * without an authored mesh. A separate component per paint-and-brush
	 * combination, because an instanced component carries exactly one material
	 * and one mesh.
	 * -------------------------------------------------------------------- */

	FLinearColor ColorOf(ECapePaint Paint) const;

	/** Component for a paint/brush pair, created on first use. */
	UInstancedStaticMeshComponent* GetPaint(ECapePaint Paint, ECapeBrush Brush = ECapeBrush::Box);

	void PaintBox(ECapePaint Paint, const FVector& Center, const FVector& Size,
		float YawDegrees = 0.0f);
	void PaintTube(ECapePaint Paint, const FVector& BaseCenter, float DiameterCm, float HeightCm);
	void PaintCone(ECapePaint Paint, const FVector& BaseCenter, float DiameterCm, float HeightCm);

	ECapePaint BodyPaintFor(const FCapeBuilding& Building) const;

	void AddFillLight(const FVector& Local, float Intensity, const FLinearColor& Color, float Radius);
	void AddBelt(const FCapeBuilding& Building, float Z, float Height, ECapePaint Paint);
	void AddWindowGrid(const FCapeBuilding& Building, ECapeDoorSide Face,
		int32 Cols, int32 Rows, float Z0, float Z1);
	void AddFlagOnFace(const FCapeBuilding& Building, ECapeDoorSide Face,
		float Width, float Height, float MidZ);
	void AddHeatTiles(const FVector& Base, float Diameter, float Z0, float Z1, float YawCenter);

	/** Thin colour skin over a building's four walls, doorway left open. */
	void AddFacade(const FCapeBuilding& Building, ECapePaint Paint);

	void DressBuildingExterior(const FCapeBuilding& Building);
	void DressVabExterior(const FCapeBuilding& Building);
	void DressMccExterior(const FCapeBuilding& Building);
	void DressJplExterior(const FCapeBuilding& Building);
	void DressAdminExterior(const FCapeBuilding& Building);
	void DressPlaza();

	/**
	 * One component per paint x brush, indexed Paint * BrushCount + Brush.
	 * Entries are also tracked in Generated, so ClearGenerated destroys them —
	 * which means this array MUST be reset at the same time or it is left full
	 * of dangling pointers that GetPaint will happily return.
	 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> PaintBuckets;

	/**
	 * Fit rules for the paint buckets. Fixed, and deliberately not editable.
	 *
	 * Every other component maps to an art slot the player can drop a Fab asset
	 * into. The paint buckets must not: they are always the engine primitive at
	 * an exact requested size, so a stripe is the width it says it is. Without
	 * this they fall through to StructureSlot, and the moment that slot gets a
	 * uniform fit or a random yaw the flags and window grids deform with it.
	 */
	UPROPERTY(Transient)
	FCapeMeshSlot PaintSlot;

	/**
	 * Points an instanced component at a slot's mesh and material, falling back
	 * to the given engine primitive and a flat tint when nothing is assigned.
	 */
	void ApplySlot(UInstancedStaticMeshComponent* Component, const FCapeMeshSlot& Slot,
		UStaticMesh* FallbackMesh);

	/** Which art slot drives a given instanced component. */
	const FCapeMeshSlot& SlotForComponent(const UInstancedStaticMeshComponent* Component) const;

	/** Native bounds of whatever mesh a component currently holds, in cm. */
	FVector NativeSizeOf(const UInstancedStaticMeshComponent* Component) const;

	/** Bounds-centre offset, so an off-pivot Fab mesh still lands where asked. */
	FVector PivotOffsetOf(const UInstancedStaticMeshComponent* Component) const;

	/** Adds one instance sized to Size, honouring the slot's fit mode. */
	void AddFitted(UInstancedStaticMeshComponent* Component, const FCapeMeshSlot& Slot,
		const FVector& Center, const FVector& Size, float YawDegrees, int32 InstanceSeed);

	/** Destroys everything OnConstruction generated, before regenerating. */
	void ClearGenerated();

	void AddInteriorLight(const FCapeBuilding& Building);

	/** Spawned at BeginPlay rather than in OnConstruction: spawning actors
	 *  from a construction script is a well-known source of editor duplicates. */
	void SpawnTerminals();

	/** Adds a cube instance covering an axis-aligned box. */
	void AddBox(const FVector& Center, const FVector& Size);

	/** As AddBox, into a chosen component and with a yaw. */
	void AddBoxTo(UInstancedStaticMeshComponent* Component, const FVector& Center,
		const FVector& Size, float YawDegrees = 0.0f);

	/** Adds a cylinder instance of the given diameter and height. */
	void AddCylinder(const FVector& BaseCenter, float DiameterCm, float HeightCm);

	void AddCylinderTo(UInstancedStaticMeshComponent* Component, const FVector& BaseCenter,
		float DiameterCm, float HeightCm);

	/** Cone standing on its base. Nosecones and tree canopies. */
	void AddConeTo(UInstancedStaticMeshComponent* Component, const FVector& BaseCenter,
		float DiameterCm, float HeightCm);

	/** Base material for the flat-tint fallback. Engine content. */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> TintBaseMaterial;

	/** Engine primitives, used whenever a slot has no mesh assigned. */
	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> FallbackCube;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> FallbackCylinder;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> FallbackCone;

	/**
	 * One wall, with a doorway punched through it if bWithDoor. The gap is
	 * built as two jambs plus a lintel rather than by boolean subtraction.
	 */
	void AddWall(const FVector& Center, const FVector& Size, bool bAlongY,
		bool bWithDoor, float DoorWidth, float DoorHeight);

	/** As AddWall, into a chosen component. The doorway is measured from the
	 *  BOTTOM of Size.Z, so the box passed in has to start at floor level. */
	void AddWallTo(UInstancedStaticMeshComponent* Component, const FVector& Center,
		const FVector& Size, bool bAlongY, bool bWithDoor, float DoorWidth, float DoorHeight);

	FVector EntranceOffsetFor(const FCapeBuilding& Building) const;

	/**
	 * Signs and lights created by OnConstruction. Tracked together so a rebuild
	 * can tear down exactly what it made and nothing else.
	 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<USceneComponent>> Generated;
};
