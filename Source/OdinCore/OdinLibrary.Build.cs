/* Copyright (c) 2022-2023 4Players GmbH. All rights reserved. */

using System.IO;
using UnrealBuildTool;
using System;
using System.Linq;

public class OdinLibrary : ModuleRules
{
    public OdinLibrary(ReadOnlyTargetRules Target) : base(Target)
    {
        Type = ModuleType.External;

        string extensionFlag = "ODIN_USE_EXTENSIONS=true";
        string[] winExtensionFiles = new string[] { "odin_crypto.dll", "odin_crypto.lib" };
        string[] soExtensionFiles = new string[] { "libodin_crypto.so" };

        PublicDefinitions.Add(extensionFlag);
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "include"));

        var addExtern = (string[] tree, string[] files, Action<string, string> action) =>
        {
            string fullpath = Path.Combine(tree);
            foreach (string file in files)
                action(Path.Combine(fullpath, file), file);
        };

        string[] winFiles = new string[] { "odin.dll", "odin.lib"}.Union(winExtensionFiles).ToArray();
        string[] soFiles = new string[] { "libodin.so" }.Union(soExtensionFiles).ToArray();

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            addExtern(new string[] { ModuleDirectory, "x64", "Win" }, winFiles, (fullpath, file) =>  {
                if(Path.GetExtension(fullpath) == ".lib")
                    PublicAdditionalLibraries.Add(fullpath);

                if(Path.GetExtension(fullpath) == ".dll")
                {
                    PublicDelayLoadDLLs.Add(fullpath);
                    PublicDelayLoadDLLs.Add(file); // delay import short name
                    RuntimeDependencies.Add(fullpath);
                }
            });
        }
        else if (Target.Platform == UnrealTargetPlatform.Mac)
        {

            PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "universal", "macOS", "libodin_static.a"));
            string crypto = Path.Combine(ModuleDirectory, "universal", "macOS", "libodin_crypto_static.a");
            if (File.Exists(crypto))
                PublicAdditionalLibraries.Add(crypto);
            else
                PublicDefinitions.Remove(extensionFlag);

            PublicFrameworks.Add("SystemConfiguration");
        }
        else if (Target.Platform == UnrealTargetPlatform.IOS)
        {
            PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "universal", "iOS", "libodin_static.a"));
            string crypto = Path.Combine(ModuleDirectory, "universal", "iOS", "libodin_crypto_static.a");
            if (File.Exists(crypto))
                PublicAdditionalLibraries.Add(crypto);
            else
                PublicDefinitions.Remove(extensionFlag);

            PublicFrameworks.Add("SystemConfiguration");
        }
        else if (Target.Platform == UnrealTargetPlatform.Linux)
        {
            addExtern(new string[] { ModuleDirectory, "x64", "Linux" }, soFiles, (fullpath, _) => {
                PublicAdditionalLibraries.Add(fullpath);
                PublicDelayLoadDLLs.Add(fullpath);
                RuntimeDependencies.Add(fullpath);
            });
        }
#if UE_5_0_OR_LATER
        else if (Target.Platform == UnrealTargetPlatform.LinuxArm64)
        {
            addExtern(new string[] { ModuleDirectory, "arm64", "Linux" }, soFiles, (fullpath, _) => {
                PublicAdditionalLibraries.Add(fullpath);
                PublicDelayLoadDLLs.Add(fullpath);
                RuntimeDependencies.Add(fullpath);
            });
        }
#endif
        else if (Target.Platform == UnrealTargetPlatform.Android)
        {
            addExtern(new string[] { ModuleDirectory, "x64", "Android" }, soFiles, (fullpath, _) =>  PublicAdditionalLibraries.Add(fullpath));
            addExtern(new string[] { ModuleDirectory, "arm64", "Android" }, soFiles, (fullpath, _) => PublicAdditionalLibraries.Add(fullpath));

            string PluginPath = Utils.MakePathRelativeTo(ModuleDirectory, Target.RelativeEnginePath);

            AdditionalPropertiesForReceipt.Add("AndroidPlugin", Path.Combine(PluginPath, "OdinLibrarySDK_UPL.xml"));
        }

        else if (UnrealTargetPlatform.IsValidName("PS5") && UnrealTargetPlatform.TryParse("PS5", out UnrealTargetPlatform support) && Target.Platform == support)
        {
            PublicDefinitions.Remove(extensionFlag);

            string[] ps5Files = new string[] { "libodin.prx", "libodin_stub_weak.a" /*, "libodin_crypto.prx", "libodin_crypto_stub_weak.a" */ };

            addExtern(new string[] { ModuleDirectory, "x64", "PS5" }, ps5Files, (fullpath, file) => {
                if (File.Exists(fullpath) == false) return;

                if (Path.GetExtension(fullpath) == ".a")
                    PublicAdditionalLibraries.Add(fullpath);

                if (Path.GetExtension(fullpath) == ".prx")
                {
                    PublicDelayLoadDLLs.Add(fullpath);
                    RuntimeDependencies.Add(fullpath);
                }
            });
        }
    }
}
