// Copyright (c) MARS project.

using UnrealBuildTool;

/// <summary>
/// AresCore — pure simulation.
///
/// HARD RULE (brief §3.1): no dependency on Engine. Only Core, CoreUObject and
/// Json are permitted. This module must compile and unit-test headless.
///
/// In practice the sources go further than the rule requires: they include no
/// Unreal header at all, only standard C++20. That is what lets CMakeLists.txt
/// build and test this exact same source tree on a machine with no engine
/// installed. Do not add an Engine dependency here to "make something easier" —
/// if you need Engine, the code belongs in AresGame.
///
/// The reference implementation violates the equivalent rule: src/sim/research.js,
/// src/sim/planning.js and src/sim/fleet.js each import MISSION_TYPES from
/// src/render/phases.js. Mission types are simulation vocabulary and live in
/// this module instead.
/// </summary>
public class AresCore : ModuleRules
{
	public AresCore(ReadOnlyTargetRules Target) : base(Target)
	{
		// The sources are plain C++ with no UE headers, so a shared PCH would
		// only slow this down.
		PCHUsage = PCHUsageMode.NoPCHs;

		// Deliberately NOT disabling unity builds here. The obvious candidates
		// are version-unstable: bUseUnityBuild lives on TargetRules, not
		// ModuleRules, and ModuleRules.bUseUnity has moved across 5.x releases.
		// Unity builds can mask a missing #include, but these sources already
		// compile non-unity under the headless CMake build (see CMakeLists.txt),
		// which covers that risk without betting on a UBT property name.

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Json",
		});

		// Deliberately empty. Anything that needs Engine belongs in AresGame.
		PrivateDependencyModuleNames.AddRange(new string[] { });

		// Determinism: the simulation must produce identical results from
		// identical seeds, so the compiler must not reassociate floating-point
		// work in the clock or the Kepler solver. MSVC's /fp:precise and
		// Clang's default -ffp-contract=on-per-statement are both acceptable;
		// what matters is that nothing enables fast-math for this module.
		//
		// Deliberately NOT setting bUseAVX here: that property was deprecated
		// in favour of MinCpuArchX64 and its availability varies by engine
		// version. If you later need to pin the ISA for cross-machine
		// reproducibility, use MinCpuArchX64 rather than reviving bUseAVX.
		PrivateDefinitions.Add("ARES_STRICT_FP=1");
	}
}
