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
      // Add the import library
      PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "x64", "Win", "odin.lib"));
      PublicDelayLoadDLLs.Add("odin.dll");
      RuntimeDependencies.Add("$(TargetOutputDir)/odin.dll", "$(PluginDir)/Source/ThirdParty/OdinLibrary/x64/Win/odin.dll");
    }
    else if (Target.Platform == UnrealTargetPlatform.Mac)
    {
      string odinDylibPath = Path.Combine("$(PluginDir)", "Source", "ThirdParty", "OdinLibrary", "x64", "Mac", "libodin.dylib");
      PublicAdditionalLibraries.Add(odinDylibPath);
      RuntimeDependencies.Add("$(TargetOutputDir)/libodin.dylib", odinDylibPath);
    }
    else if (Target.Platform == UnrealTargetPlatform.Linux)
    {
      string odinSoPath = Path.Combine("$(PluginDir)", "Source", "ThirdParty", "OdinLibrary", "x64", "Linux", "libodin.so");
      PublicAdditionalLibraries.Add(odinSoPath);
      RuntimeDependencies.Add("$(TargetOutputDir)/libodin.so", odinSoPath);
    }
  }
}
