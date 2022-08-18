/* Copyright (c) 2022 4Players GmbH. All rights reserved. */

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
            PublicAdditionalLibraries.Add(Path.Combine(modulePath, "odin.lib")); ;
            PublicDelayLoadDLLs.Add("odin.dll");
            RuntimeDependencies.Add("$(TargetOutputDir)/odin.dll", "$(PluginDir)/Source/OdinCore/x64/Win/odin.dll");
        }
        else if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            string odinDylibPath = Path.Combine(ModuleDirectory, "x64", "Mac", "libodin.dylib");
            PublicAdditionalLibraries.Add(odinDylibPath);
            PublicDelayLoadDLLs.Add(odinDylibPath);
            RuntimeDependencies.Add("$(TargetOutputDir)/libodin.dylib", odinDylibPath);
            RuntimeDependencies.Add(odinDylibPath);
        }
        //else if (Target.Platform == UnrealTargetPlatform.LinuxArm64)
        //{
        //    string odinSoPath = Path.Combine(ModuleDirectory, "arm64", "Linux", "libodin.so");
        //    PublicAdditionalLibraries.Add(odinSoPath);
        //    PublicDelayLoadDLLs.Add(odinSoPath);
        //    RuntimeDependencies.Add("$(TargetOutputDir)/libodin.so", odinSoPath);
        //}
        else if (Target.Platform == UnrealTargetPlatform.Linux)
        {
            string odinSoPath = Path.Combine(ModuleDirectory, "x64", "Linux", "libodin.so");
            PublicAdditionalLibraries.Add(odinSoPath);
            PublicDelayLoadDLLs.Add(odinSoPath);
            RuntimeDependencies.Add("$(TargetOutputDir)/libodin.so", odinSoPath);
        }
        else if (Target.Platform == UnrealTargetPlatform.Android)
        {
            PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "arm64", "Android", "libodin.so"));
            PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "x64", "Android", "libodin.so"));
            string PluginPath = Utils.MakePathRelativeTo(ModuleDirectory, Target.RelativeEnginePath);
            AdditionalPropertiesForReceipt.Add("AndroidPlugin", Path.Combine(PluginPath, "OdinLibrarySDK_UPL.xml"));
        }
    }
}
