using UnrealBuildTool;
using System.IO;
using System.Diagnostics;

public class AWSCore : ModuleRules
{
	public AWSCore(ReadOnlyTargetRules Target ): base(Target)
	{
		PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Private"));
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public"));

		PublicDependencyModuleNames.AddRange(new string[] { "Engine", "Core", "CoreUObject", "InputCore", "Projects"});
		PrivateDependencyModuleNames.AddRange(new string[] { });

        // This is required to fix a warning for Unreal Engine 4.21 and later
        PrivatePCHHeaderFile = "Private/AWSCorePrivatePCH.h";
		
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

                PublicDefinitions.Add("WITH_AWSCORE=1");
                PublicLibraryPaths.Add(ThirdPartyPath);
                if (Target.Platform == UnrealTargetPlatform.Linux)
                {
                    // dependencies
                    string CommonSoFile = System.IO.Path.Combine(ThirdPartyPath, "libaws-c-common.so");
                    if (File.Exists(CommonSoFile))
                    {
                        PublicAdditionalLibraries.Add(CommonSoFile);
                        RuntimeDependencies.Add(CommonSoFile);
                    }
                    else
                    {
                        throw new BuildException("libaws-c-common.so not found. Expected in this location: " + CommonSoFile);
                    }

                    string EventStreamSoFile = System.IO.Path.Combine(ThirdPartyPath, "libaws-c-event-stream.so");
                    if (File.Exists(EventStreamSoFile))
                    {
                        PublicAdditionalLibraries.Add(EventStreamSoFile);
                        RuntimeDependencies.Add(EventStreamSoFile);
                    }
                    else
                    {
                        throw new BuildException("libaws-c-event-stream.so not found. Expected in this location: " + EventStreamSoFile);
                    }

                    string ChecksumsSoFile = System.IO.Path.Combine(ThirdPartyPath, "libaws-checksums.so");
                    if (File.Exists(ChecksumsSoFile))
                    {
                        PublicAdditionalLibraries.Add(ChecksumsSoFile);
                        RuntimeDependencies.Add(ChecksumsSoFile);
                    }
                    else
                    {
                        throw new BuildException("libaws-checksums.so not found. Expected in this location: " + ChecksumsSoFile);
                    }

                    // aws core
                    string AWSCoreSoFile = System.IO.Path.Combine(ThirdPartyPath, "libaws-cpp-sdk-core.so");
                    if (File.Exists(AWSCoreSoFile))
                    {
                        PublicAdditionalLibraries.Add(AWSCoreSoFile);
                        RuntimeDependencies.Add(AWSCoreSoFile);
                    }
                    else
                    {
                        throw new BuildException("libaws-cpp-sdk-core.so not found. Expected in this location: " + AWSCoreSoFile);
                    }

                    // binaries
                    if (File.Exists(System.IO.Path.Combine(BinariesDirectory, "libaws-c-common.so")) == false)
                    {
                        File.Copy(System.IO.Path.Combine(ThirdPartyPath, "libaws-c-common.so"), System.IO.Path.Combine(BinariesDirectory, "libaws-c-common.so"));
                    }
                    if (File.Exists(System.IO.Path.Combine(BinariesDirectory, "libaws-c-event-stream.so")) == false)
                    {
                        File.Copy(System.IO.Path.Combine(ThirdPartyPath, "libaws-c-event-stream.so"), System.IO.Path.Combine(BinariesDirectory, "libaws-c-event-stream.so"));
                    }
                    if (File.Exists(System.IO.Path.Combine(BinariesDirectory, "libaws-checksums.so")) == false)
                    {
                        File.Copy(System.IO.Path.Combine(ThirdPartyPath, "libaws-checksums.so"), System.IO.Path.Combine(BinariesDirectory, "libaws-checksums.so"));
                    }
                    if (File.Exists(System.IO.Path.Combine(BinariesDirectory, "libaws-cpp-sdk-core.so")) == false)
                    {
                        File.Copy(System.IO.Path.Combine(ThirdPartyPath, "libaws-cpp-sdk-core.so"), System.IO.Path.Combine(BinariesDirectory, "libaws-cpp-sdk-core.so"));
                    }
                }
                else if (Target.Platform == UnrealTargetPlatform.Win64)
                {
                    // dependencies
                    string CommonLibFile = System.IO.Path.Combine(ThirdPartyPath, "aws-c-common.lib");
                    if (File.Exists(CommonLibFile))
                    {
                        PublicAdditionalLibraries.Add(CommonLibFile);
                    }
                    else
                    {
                        throw new BuildException("aws-c-common.lib not found. Expected in this location: " + CommonLibFile);
                    }

                    string CommonDLLFile = System.IO.Path.Combine(ThirdPartyPath, "aws-c-common.dll");
                    if (File.Exists(CommonDLLFile))
                    {
                        PublicDelayLoadDLLs.Add("aws-c-common.dll");
                        RuntimeDependencies.Add(CommonDLLFile);
                    }
                    else
                    {
                        throw new BuildException("aws-c-common.dll not found. Expected in this location: " + CommonDLLFile);
                    }

                    string EventStreamLibFile = System.IO.Path.Combine(ThirdPartyPath, "aws-c-event-stream.lib");
                    if (File.Exists(EventStreamLibFile))
                    {
                        PublicAdditionalLibraries.Add(EventStreamLibFile);
                    }
                    else
                    {
                        throw new BuildException("aws-c-event-stream.lib not found. Expected in this location: " + EventStreamLibFile);
                    }

                    string EventStreamDLLFile = System.IO.Path.Combine(ThirdPartyPath, "aws-c-event-stream.dll");
                    if (File.Exists(EventStreamDLLFile))
                    {
                        PublicDelayLoadDLLs.Add("aws-c-event-stream.dll");
                        RuntimeDependencies.Add(EventStreamDLLFile);
                    }
                    else
                    {
                        throw new BuildException("aws-c-event-stream.dll not found. Expected in this location: " + EventStreamDLLFile);
                    }

                    string ChecksumsLibFile = System.IO.Path.Combine(ThirdPartyPath, "aws-checksums.lib");
                    if (File.Exists(ChecksumsLibFile))
                    {
                        PublicAdditionalLibraries.Add(ChecksumsLibFile);
                    }
                    else
                    {
                        throw new BuildException("aws-checksums.lib not found. Expected in this location: " + ChecksumsLibFile);
                    }

                    string ChecksumsDLLFile = System.IO.Path.Combine(ThirdPartyPath, "aws-checksums.dll");
                    if (File.Exists(ChecksumsDLLFile))
                    {
                        PublicDelayLoadDLLs.Add("aws-checksums.dll");
                        RuntimeDependencies.Add(ChecksumsDLLFile);
                    }
                    else
                    {
                        throw new BuildException("aws-checksums.dll not found. Expected in this location: " + ChecksumsDLLFile);
                    }

                    // aws core
                    string AWSCoreLibFile = System.IO.Path.Combine(ThirdPartyPath, "aws-cpp-sdk-core.lib");
                    if (File.Exists(AWSCoreLibFile))
                    {
                        PublicAdditionalLibraries.Add(AWSCoreLibFile);
                    }
                    else
                    {
                        throw new BuildException("aws-cpp-sdk-core.lib not found. Expected in this location: " + AWSCoreLibFile);
                    }

                    string AWSCoreDLLFile = System.IO.Path.Combine(ThirdPartyPath, "aws-cpp-sdk-core.dll");
                    if (File.Exists(AWSCoreDLLFile))
                    {
                        PublicDelayLoadDLLs.Add("aws-cpp-sdk-core.dll");
                        RuntimeDependencies.Add(AWSCoreDLLFile);
                    }
                    else
                    {
                        throw new BuildException("aws-cpp-sdk-core.dll not found. Expected in this location: " + AWSCoreDLLFile);
                    }

                    // binaries     
                    if (File.Exists(System.IO.Path.Combine(BinariesDirectory, "aws-c-common.dll")) == false)
                    {
                        File.Copy(System.IO.Path.Combine(ThirdPartyPath, "aws-c-common.dll"), System.IO.Path.Combine(BinariesDirectory, "aws-c-common.dll"));
                    }
                    if (File.Exists(System.IO.Path.Combine(BinariesDirectory, "aws-c-event-stream.dll")) == false)
                    {
                        File.Copy(System.IO.Path.Combine(ThirdPartyPath, "aws-c-event-stream.dll"), System.IO.Path.Combine(BinariesDirectory, "aws-c-event-stream.dll"));
                    }
                    if (File.Exists(System.IO.Path.Combine(BinariesDirectory, "aws-checksums.dll")) == false)
                    {
                        File.Copy(System.IO.Path.Combine(ThirdPartyPath, "aws-checksums.dll"), System.IO.Path.Combine(BinariesDirectory, "aws-checksums.dll"));
                    }
                    if (File.Exists(System.IO.Path.Combine(BinariesDirectory, "aws-cpp-sdk-core.dll")) == false)
                    {
                        File.Copy(System.IO.Path.Combine(ThirdPartyPath, "aws-cpp-sdk-core.dll"), System.IO.Path.Combine(BinariesDirectory, "aws-cpp-sdk-core.dll"));
                    }
                }
            }
            else
            {
                PublicDefinitions.Add("WITH_AWSCORE=0");
            }
        }
        else
        {
            PublicDefinitions.Add("WITH_AWSCORE=0");
        }
    }
}
