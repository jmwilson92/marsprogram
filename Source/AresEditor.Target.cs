// Copyright (c) MARS project.

using UnrealBuildTool;
using System.Collections.Generic;

public class AresEditorTarget : TargetRules
{
	public AresEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

		ExtraModuleNames.AddRange(new string[]
		{
			"AresCore",
			"AresGame",
			"AresUI",
			"AresEditor",
		});
	}
}
