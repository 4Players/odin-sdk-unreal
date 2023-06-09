/* Copyright (c) 2022-2023 4Players GmbH. All rights reserved. */

using System.IO;
using UnrealBuildTool;
using System.Diagnostics;

public class OdinLibrary : ModuleRules
{
  public OdinLibrary(ReadOnlyTargetRules Target) : base(Target)
  {
    Type = ModuleType.External;

    if (Target.Platform == UnrealTargetPlatform.Win64)
    {
      string modulePath = Path.Combine(ModuleDirectory, "x64", "Win");
      string libPath = Path.Combine(modulePath, "odin.lib");
      string dllPath = Path.Combine(modulePath, "odin.dll");

      PublicAdditionalLibraries.Add(libPath);
      PublicDelayLoadDLLs.Add("odin.dll");
      RuntimeDependencies.Add(dllPath);
    }
    else if (Target.Platform == UnrealTargetPlatform.Mac)
    {
      PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "universal", "macOS", "libodin_static.a"));
    }
    else if (Target.Platform == UnrealTargetPlatform.IOS)
    {
      PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "universal", "iOS", "libodin_static.a"));
    }
    else if (Target.Platform == UnrealTargetPlatform.Linux)
    {
      string odinSoPath = Path.Combine(ModuleDirectory, "x64", "Linux", "libodin.so");

      PublicAdditionalLibraries.Add(odinSoPath);
      PublicDelayLoadDLLs.Add(odinSoPath);
      RuntimeDependencies.Add(odinSoPath);
    }
#if UE_5_0_OR_LATER
    else if (Target.Platform == UnrealTargetPlatform.LinuxArm64)
    {
      string odinSoPath = Path.Combine(ModuleDirectory, "arm64", "Linux", "libodin.so");

      PublicAdditionalLibraries.Add(odinSoPath);
      PublicDelayLoadDLLs.Add(odinSoPath);
      RuntimeDependencies.Add(odinSoPath);
    }
#endif
    else if (Target.Platform == UnrealTargetPlatform.Android)
    {
      string odinSoPathX86 = Path.Combine(ModuleDirectory, "x64", "Android", "libodin.so");
      string odinSoPathArm = Path.Combine(ModuleDirectory, "arm64", "Android", "libodin.so");

      PublicAdditionalLibraries.Add(odinSoPathX86);
      PublicAdditionalLibraries.Add(odinSoPathArm);

      string PluginPath = Utils.MakePathRelativeTo(ModuleDirectory, Target.RelativeEnginePath);

      AdditionalPropertiesForReceipt.Add("AndroidPlugin", Path.Combine(PluginPath, "OdinLibrarySDK_UPL.xml"));
    }
  }
}
