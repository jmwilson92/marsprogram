// Copyright (c) MARS project.

using UnrealBuildTool;

/// <summary>
/// AresUI — UMG widgets, terminal screens, HUD.
///
/// Depends on AresCore for READ-ONLY snapshots. Widgets never mutate program
/// state; they raise commands that UProgramSubsystem applies. Note the absence
/// of a dependency on AresGame: the UI reads the simulation, and the game
/// module wires widgets to terminals, not the other way round.
/// </summary>
public class AresUI : ModuleRules
{
	public AresUI(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"UMG",
			"AresCore",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate",
			"SlateCore",
			"InputCore",
		});
	}
}
