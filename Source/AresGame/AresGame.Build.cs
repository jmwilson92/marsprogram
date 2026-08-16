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
