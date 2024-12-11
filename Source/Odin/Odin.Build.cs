/* Copyright (c) 2022-2024 4Players GmbH. All rights reserved. */

using System.IO;
using UnrealBuildTool;

public class Odin : ModuleRules
{
	public Odin(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.CPlusPlus;
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		PrecompileForTargets = PrecompileTargetsType.Any;
		DefaultBuildSettings = BuildSettingsVersion.Latest;

		// List of all paths to include files that are exposed to other modules
		PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public"));

		// List of all paths to this module's internal include files, not exposed to other modules
		PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Private"));

#if UE_5_5_OR_LATER
		UndefinedIdentifierWarningLevel = WarningLevel.Warning;
#else
    // Enable warnings for using undefined identifiers in #if expressions
    bEnableUndefinedIdentifierWarnings = false;
#endif
		// List of public dependency module names
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"AudioMixer",
				"SignalProcessing",
				"AudioCapture",
				"Json",
				"JsonUtilities",
				"OdinLibrary",
				"Projects", "AudioCaptureCore",
			}
		);
	}
}