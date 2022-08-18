/* Copyright (c) 2022 4Players GmbH. All rights reserved. */

using System.IO;

using UnrealBuildTool;

public class Odin : ModuleRules
{
  public Odin(ReadOnlyTargetRules Target) : base(Target)
  {
    PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

    PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public"));

    PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Private"));

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
        "Projects",
      }
      );
  }
}
