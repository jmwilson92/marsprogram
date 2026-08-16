// Copyright (c) MARS project.

using UnrealBuildTool;
using System.Collections.Generic;

public class AresTarget : TargetRules
{
	public AresTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

		ExtraModuleNames.AddRange(new string[]
		{
			"AresCore",
			"AresGame",
			"AresUI",
		});
	}
}
