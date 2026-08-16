// Copyright (c) MARS project.

using UnrealBuildTool;

/// <summary>
/// AresEditor — data validation commandlets and JSON to DataTable importers.
///
/// Editor-only. Turns Data/*.json into DataTables for cooked builds and runs
/// the content validation that keeps balance data honest (brief §2/§9).
/// </summary>
public class AresEditor : ModuleRules
{
	public AresEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"UnrealEd",
			"AresCore",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Json",
			"JsonUtilities",
			"AssetTools",
			"DataValidation",
			"Slate",
			"SlateCore",
			"AresGame",
		});
	}
}
