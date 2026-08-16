// The bridge between the simulation and the game (brief §3.2).
//
// Owns the one FProgramState. Lives on the GameInstance, so the program
// survives level travel — that is what lets you walk out of Mission Control,
// board Starship and fly to Mars across three maps without the simulation
// resetting.
//
// Nothing outside this subsystem mutates FProgramState. UI and actors read
// const snapshots and raise commands; the subsystem applies them. That rule is
// what keeps a determinism guarantee meaningful: if any widget could write to
// the state, "same seed, same run" would be a lie.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"

#include "AresCore/AresConstants.h"
#include "AresCore/AresData.h"
#include "AresCore/ProgramState.h"

#include "ProgramSubsystem.generated.h"

/** Raised after any tick or command that changed the program. */
DECLARE_MULTICAST_DELEGATE(FOnProgramChanged);

/** Timewarp rates offered in Mission Control. Ports the reference's FLIGHT_RATES. */
UENUM(BlueprintType)
enum class EProgramSpeed : uint8
{
	Paused  UMETA(DisplayName = "Paused"),
	X1      UMETA(DisplayName = "1x"),
	X3      UMETA(DisplayName = "3x"),
	X10     UMETA(DisplayName = "10x"),
	X30     UMETA(DisplayName = "30x"),
};

UCLASS()
class ARESGAME_API UProgramSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	// --- UGameInstanceSubsystem ---
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// --- FTickableGameObject ---
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override { return bDataLoaded; }
	/** Ticks in the editor too, so a terminal previews live numbers. */
	virtual bool IsTickableInEditor() const override { return false; }

	/* --- reading ------------------------------------------------------- */

	/** The live program. Const on purpose — see the class comment. */
	const Ares::FProgramState& GetState() const { return State; }

	/** Loaded content tables. */
	const Ares::FAresData& GetData() const { return Data; }

	bool IsReady() const { return bDataLoaded; }

	/** Fires whenever the program changed. Widgets bind to this rather than polling. */
	FOnProgramChanged OnProgramChanged;

	/* --- time ---------------------------------------------------------- */

	UFUNCTION(BlueprintCallable, Category = "Ares|Program")
	void SetSpeed(EProgramSpeed InSpeed);

	UFUNCTION(BlueprintPure, Category = "Ares|Program")
	EProgramSpeed GetSpeed() const { return Speed; }

	/** Days per real second at the current rate. */
	UFUNCTION(BlueprintPure, Category = "Ares|Program")
	float GetDaysPerSecond() const;

	/**
	 * Runs days immediately, ignoring the rate. Used by "skip to next event".
	 * Returns how many actually ran — a cancelled program stops early.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ares|Program")
	int32 SkipDays(int32 Days);

	/** Advances to the next transfer window, or the cap, whichever comes first. */
	UFUNCTION(BlueprintCallable, Category = "Ares|Program")
	int32 SkipToNextWindow(int32 MaxDays = 900);

	/* --- program ------------------------------------------------------- */

	/** Discards the current program and starts a new one. */
	UFUNCTION(BlueprintCallable, Category = "Ares|Program")
	void StartNewProgram(int32 Seed, const FString& Difficulty);

	/* --- read-only accessors for UI ------------------------------------ */
	// Blueprint and UMG cannot see the AresCore types, so the handful of values
	// a screen actually shows are surfaced individually rather than by exposing
	// the state struct itself. That also keeps the mutation rule enforceable.

	UFUNCTION(BlueprintPure, Category = "Ares|Program") float GetEarthDay() const;
	UFUNCTION(BlueprintPure, Category = "Ares|Program") FString GetEarthDateIso() const;
	UFUNCTION(BlueprintPure, Category = "Ares|Program") float GetProgramYear() const;
	UFUNCTION(BlueprintPure, Category = "Ares|Program") float GetBudgetRemainingUsd() const;
	UFUNCTION(BlueprintPure, Category = "Ares|Program") float GetBudgetAnnualUsd() const;
	UFUNCTION(BlueprintPure, Category = "Ares|Program") bool IsHoarding() const;
	UFUNCTION(BlueprintPure, Category = "Ares|Program") float GetSupport() const;
	UFUNCTION(BlueprintPure, Category = "Ares|Program") int32 GetQuartersUnderThreshold() const;
	UFUNCTION(BlueprintPure, Category = "Ares|Program") bool IsCancelled() const;
	UFUNCTION(BlueprintPure, Category = "Ares|Program") FString GetCancellationReason() const;
	UFUNCTION(BlueprintPure, Category = "Ares|Program") float GetResearchPoints() const;
	UFUNCTION(BlueprintPure, Category = "Ares|Program") float GetDaysToNextWindow() const;
	UFUNCTION(BlueprintPure, Category = "Ares|Program") int32 GetCrewCount() const;
	UFUNCTION(BlueprintPure, Category = "Ares|Program") int32 GetShipCount() const;

protected:
	/** Loads Data/*.json. Returns false and logs on any malformed table. */
	bool LoadContent();

	Ares::FProgramState State;
	Ares::FAresData Data;

	EProgramSpeed Speed = EProgramSpeed::Paused;

	/** Fractional day carried between frames so slow rates still advance. */
	double DayAccumulator = 0.0;

	bool bDataLoaded = false;

	/**
	 * Days ticked in one frame is capped. A long timewarp after a stall would
	 * otherwise try to run thousands of days in a single frame and hitch;
	 * brief §3.2 wants that work on a background task, which is the next step.
	 */
	static constexpr int32 MaxDaysPerFrame = 16;
};
