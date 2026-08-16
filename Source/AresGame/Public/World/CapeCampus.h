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

class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPointLightComponent;
class UStaticMesh;
class UTextRenderComponent;

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

	/** Stack Super Heavy and Starship on the pad. */
	UPROPERTY(EditAnywhere, Category = "Ares|Cape")
	bool bBuildLaunchVehicle = true;

	/** Grass and trees around the campus. */
	UPROPERTY(EditAnywhere, Category = "Ares|Cape")
	bool bBuildLandscape = true;

	UPROPERTY(EditAnywhere, Category = "Ares|Cape")
	int32 TreeCount = 260;

	/** Bikes at the plaza rack, for the ride out to the pad. */
	UPROPERTY(EditAnywhere, Category = "Ares|Cape")
	int32 BikeCount = 4;

	/* --- palette. Engine-default materials tinted, not authored art. --- */

	UPROPERTY(EditAnywhere, Category = "Ares|Cape|Palette")
	FLinearColor ConcreteColor = FLinearColor(0.42f, 0.42f, 0.44f);

	UPROPERTY(EditAnywhere, Category = "Ares|Cape|Palette")
	FLinearColor FurnitureColor = FLinearColor(0.06f, 0.07f, 0.09f);

	UPROPERTY(EditAnywhere, Category = "Ares|Cape|Palette")
	FLinearColor ScreenColor = FLinearColor(0.10f, 0.42f, 0.85f);

	UPROPERTY(EditAnywhere, Category = "Ares|Cape|Palette")
	FLinearColor SteelColor = FLinearColor(0.62f, 0.65f, 0.70f);

	UPROPERTY(EditAnywhere, Category = "Ares|Cape|Palette")
	FLinearColor GrassColor = FLinearColor(0.16f, 0.30f, 0.11f);

	UPROPERTY(EditAnywhere, Category = "Ares|Cape|Palette")
	FLinearColor FoliageColor = FLinearColor(0.12f, 0.26f, 0.10f);

	UPROPERTY(EditAnywhere, Category = "Ares|Cape|Palette")
	FLinearColor TrunkColor = FLinearColor(0.20f, 0.14f, 0.09f);

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

	/** Bikes at the plaza. Spawned at BeginPlay alongside the terminals. */
	void SpawnBikes();

	/** Tints an instanced component by making a dynamic instance of the base material. */
	void ApplyTint(UInstancedStaticMeshComponent* Component, const FLinearColor& Color);

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

	/** Base material for tinting. Engine content, loaded once in the constructor. */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> TintBaseMaterial;

	/**
	 * One wall, with a doorway punched through it if bWithDoor. The gap is
	 * built as two jambs plus a lintel rather than by boolean subtraction.
	 */
	void AddWall(const FVector& Center, const FVector& Size, bool bAlongY,
		bool bWithDoor, float DoorWidth, float DoorHeight);

	FVector EntranceOffsetFor(const FCapeBuilding& Building) const;

	/**
	 * Signs and lights created by OnConstruction. Tracked together so a rebuild
	 * can tear down exactly what it made and nothing else.
	 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<USceneComponent>> Generated;
};
