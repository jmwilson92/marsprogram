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
class UPointLightComponent;
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
	UPROPERTY(VisibleAnywhere, Category = "Ares|Cape")
	TObjectPtr<UInstancedStaticMeshComponent> Boxes;

	UPROPERTY(VisibleAnywhere, Category = "Ares|Cape")
	TObjectPtr<UInstancedStaticMeshComponent> Cylinders;

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

private:
	void BuildGroundAndRoad();
	void BuildBuilding(const FCapeBuilding& Building);
	void BuildPad();

	/** Destroys everything OnConstruction generated, before regenerating. */
	void ClearGenerated();

	void AddInteriorLight(const FCapeBuilding& Building);

	/** Spawned at BeginPlay rather than in OnConstruction: spawning actors
	 *  from a construction script is a well-known source of editor duplicates. */
	void SpawnTerminals();

	/** Adds a cube instance covering an axis-aligned box. */
	void AddBox(const FVector& Center, const FVector& Size);

	/** Adds a cylinder instance of the given diameter and height. */
	void AddCylinder(const FVector& BaseCenter, float DiameterCm, float HeightCm);

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
