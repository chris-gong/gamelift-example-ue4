// Created by YetiTech Studios.

using UnrealBuildTool;
using System.IO;

public class GameLiftClientSDK : ModuleRules
{
	public GameLiftClientSDK(ReadOnlyTargetRules Target) : base(Target)
	{
        bEnableExceptions = true;

        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Engine", "Core", "CoreUObject", "InputCore", "Projects", "AWSCore" });
		
		PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Private"));
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public"));

        PublicDefinitions.Add("USE_IMPORT_EXPORT");

        string BaseDirectory = System.IO.Path.GetFullPath(System.IO.Path.Combine(ModuleDirectory, "..", ".."));
        string ThirdPartyPath = System.IO.Path.Combine(BaseDirectory, "ThirdParty", "GameLiftClientSDK", Target.Platform.ToString());
        string BinariesDirectory = System.IO.Path.Combine(BaseDirectory, "Binaries", Target.Platform.ToString());
        bool bIsThirdPartyPathValid = System.IO.Directory.Exists(ThirdPartyPath);

        if (bIsThirdPartyPathValid)
        {
            if (Target.Type == TargetRules.TargetType.Client)
            {
                if (!Directory.Exists(BinariesDirectory))
                {
                    Directory.CreateDirectory(BinariesDirectory);
                }

                PublicDefinitions.Add("WITH_GAMELIFTCLIENTSDK=1");
                PublicLibraryPaths.Add(ThirdPartyPath);

                if (Target.Platform == UnrealTargetPlatform.Linux)
                {
                    // gamelift (aws cpp sdk)
                    string GameLiftSoFile = System.IO.Path.Combine(ThirdPartyPath, "libaws-cpp-sdk-gamelift.so");
                    if (File.Exists(GameLiftSoFile))
                    {
                        PublicAdditionalLibraries.Add(GameLiftSoFile);
                        RuntimeDependencies.Add(GameLiftSoFile);
                    }
                    else
                    {
                        throw new BuildException("libaws-cpp-sdk-gamelift.so not found. Expected in this location: " + GameLiftSoFile);
                    }

                    // binaries
                    if (File.Exists(System.IO.Path.Combine(BinariesDirectory, "libaws-cpp-sdk-gamelift.so")) == false)
                    {
                        File.Copy(System.IO.Path.Combine(ThirdPartyPath, "libaws-cpp-sdk-gamelift.so"), System.IO.Path.Combine(BinariesDirectory, "libaws-cpp-sdk-gamelift.so"));
                    }
                }
                else if (Target.Platform == UnrealTargetPlatform.Win64)
                {
                    // gamelift (aws cpp sdk)
                    string GameLiftLibFile = System.IO.Path.Combine(ThirdPartyPath, "aws-cpp-sdk-gamelift.lib");
                    if (File.Exists(GameLiftLibFile))
                    {
                        PublicAdditionalLibraries.Add(GameLiftLibFile);
                    }
                    else
                    {
                        throw new BuildException("aws-cpp-sdk-gamelift.lib not found. Expected in this location: " + GameLiftLibFile);
                    }

                    string GameLiftDLLFile = System.IO.Path.Combine(ThirdPartyPath, "aws-cpp-sdk-gamelift.dll");
                    if (File.Exists(GameLiftDLLFile))
                    {
                        PublicDelayLoadDLLs.Add("aws-cpp-sdk-gamelift.dll");
                        RuntimeDependencies.Add(GameLiftDLLFile);
                    }
                    else
                    {
                        throw new BuildException("aws-cpp-sdk-gamelift.dll not found. Expected in this location: " + GameLiftDLLFile);
                    }

                    // binaries
                    if (File.Exists(System.IO.Path.Combine(BinariesDirectory, "aws-cpp-sdk-gamelift.dll")) == false)
                    {
                        File.Copy(System.IO.Path.Combine(ThirdPartyPath, "aws-cpp-sdk-gamelift.dll"), System.IO.Path.Combine(BinariesDirectory, "aws-cpp-sdk-gamelift.dll"));
                    }
                }
            }
            else
            {
                PublicDefinitions.Add("WITH_GAMELIFTCLIENTSDK=0");
            }
        }
        else
        {
            PublicDefinitions.Add("WITH_GAMELIFTCLIENTSDK=0");
        }
	}
}
