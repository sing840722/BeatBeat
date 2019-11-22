// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.IO;


public class BeatBeat : ModuleRules
{
    public BeatBeat(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "AudioCapture" });

        PrivateDependencyModuleNames.AddRange(new string[] { });

        //Include ThirdParty Plugins
        //**FMOD**//
        var basePath = Path.GetDirectoryName(RulesCompiler.GetFileNameFromType(GetType()));
        string thirdPartyPath = Path.Combine(basePath, "..", "..", "ThirdParty");

        PublicIncludePaths.Add(Path.Combine(thirdPartyPath, "FMOD", "inc"));

        switch (Target.Platform)
        {
            case UnrealTargetPlatform.Win64:
                PublicLibraryPaths.Add(Path.Combine(thirdPartyPath, "FMOD", "lib", "Win64"));
                PublicAdditionalLibraries.Add("fmod64_vc.lib");
                string fmodDllPath = Path.Combine(thirdPartyPath, "FMOD", "lib", "Win64", "fmod64.dll");
                RuntimeDependencies.Add(new RuntimeDependency(fmodDllPath));

                string binariesDir = Path.Combine(basePath, "..", "..", "Binaries", "Win64");
                if (!Directory.Exists(binariesDir))
                    System.IO.Directory.CreateDirectory(binariesDir);

                string fmodDllDest = System.IO.Path.Combine(binariesDir, "fmod64.dll");
                CopyFile(fmodDllPath, fmodDllDest);
                PublicDelayLoadDLLs.AddRange(new string[] { "fmod64.dll" });

                break;
            case UnrealTargetPlatform.Android:

                break;
            default:
                throw new System.Exception(System.String.Format("Unsupported platform {0}", Target.Platform.ToString()));
        }

        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true

    }

    private void CopyFile(string source, string dest)
    {
        System.Console.WriteLine("Copying {0} to {1}", source, dest);
        if (System.IO.File.Exists(dest))
        {
            System.IO.File.SetAttributes(dest, System.IO.File.GetAttributes(dest) & ~System.IO.FileAttributes.ReadOnly);
        }
        try
        {
            System.IO.File.Copy(source, dest, true);
        }
        catch (System.Exception ex)
        {
            System.Console.WriteLine("Failed to copy file: {0}", ex.Message);
        }
    }
}
