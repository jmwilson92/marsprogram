// Copyright (c) MARS project.

using UnrealBuildTool;

/// <summary>
/// AresGame — actors, subsystems, the player character, level scripting.
///
/// Owns UProgramSubsystem (a UGameInstanceSubsystem, so the program survives
/// level travel), UFlightDirectorSubsystem, UAstroBridge and UAresMovementComponent.
/// This is the only place allowed to mutate FProgramState, and it does so
/// through command structs rather than direct writes.
/// </summary>
public class AresGame : ModuleRules
{
	public AresGame(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AresCore",
			"AresUI",
			// AresGame calls UUserWidget directly (CreateWidget, AddToViewport),
			// and AAresPlayerController exposes a TSubclassOf<UAresTerminalWidget>
			// in its header. A module that uses an API declares it rather than
			// relying on picking it up through AresUI.
			"UMG",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Json",
			"JsonUtilities",
			"Slate",
			"SlateCore",
		});
	}
}
