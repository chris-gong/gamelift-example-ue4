#include "AWSCoreModulePrivatePCH.h"
#include "AWSCoreModule.h"
#include "GameLiftClientSDK/Public/GameLiftClientGlobals.h"
#include "IPluginManager.h"

#define LOCTEXT_NAMESPACE "FAWSCoreModule"

void* FAWSCoreModule::AWSCoreLibraryHandle = nullptr;
void* FAWSCoreModule::AWSCommonLibraryHandle = nullptr;
void* FAWSCoreModule::AWSEventStreamLibraryHandle = nullptr;
void* FAWSCoreModule::AWSChecksumsLibraryHandle = nullptr;

void FAWSCoreModule::StartupModule()
{
#if PLATFORM_WINDOWS && PLATFORM_64BITS && WITH_AWSCORE
	LOG_NORMAL("Starting AWSCore Module...");

	const FString BaseDir = IPluginManager::Get().FindPlugin("GameLiftClientSDK")->GetBaseDir();
	LOG_NORMAL(FString::Printf(TEXT("Base directory is %s"), *BaseDir));

	const FString ThirdPartyDir = FPaths::Combine(*BaseDir, TEXT("ThirdParty"), TEXT("GameLiftClientSDK"), TEXT("Win64"));
	LOG_NORMAL(FString::Printf(TEXT("ThirdParty directory is %s"), *ThirdPartyDir));

	// dependencies
	static const FString CommonDLLName = "aws-c-common";
	const bool bCommonDependencyLoaded = LoadDependency(ThirdPartyDir, CommonDLLName, AWSCommonLibraryHandle);

	if (bCommonDependencyLoaded == false)
	{
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("Name"), FText::FromString(CommonDLLName));
		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(LOCTEXT("LoadDependencyError", "Failed to load {Name}. Plugin will not be functional"), Arguments));
		FreeDependency(AWSCommonLibraryHandle);
		return;
	}

	static const FString EventStreamDLLName = "aws-c-event-stream";
	const bool bEventStreamDependencyLoaded = LoadDependency(ThirdPartyDir, EventStreamDLLName, AWSEventStreamLibraryHandle);

	if (bEventStreamDependencyLoaded == false)
	{
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("Name"), FText::FromString(EventStreamDLLName));
		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(LOCTEXT("LoadDependencyError", "Failed to load {Name}. Plugin will not be functional"), Arguments));
		FreeDependency(AWSEventStreamLibraryHandle);
		return;
	}

	static const FString ChecksumsDLLName = "aws-checksums";
	const bool bChecksumsDependencyLoaded = LoadDependency(ThirdPartyDir, ChecksumsDLLName, AWSChecksumsLibraryHandle);

	if (bChecksumsDependencyLoaded == false)
	{
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("Name"), FText::FromString(ChecksumsDLLName));
		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(LOCTEXT("LoadDependencyError", "Failed to load {Name}. Plugin will not be functional"), Arguments));
		FreeDependency(AWSChecksumsLibraryHandle);
		return;
	}

	// aws core
	static const FString CoreDLLName = "aws-cpp-sdk-core";
	const bool bCoreDependencyLoaded = LoadDependency(ThirdPartyDir, CoreDLLName, AWSCoreLibraryHandle);
	
	if (bCoreDependencyLoaded == false)
	{
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("Name"), FText::FromString(CoreDLLName));
		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(LOCTEXT("LoadDependencyError", "Failed to load {Name}. Plugin will not be functional"), Arguments));
		FreeDependency(AWSCoreLibraryHandle);
		return;
	}
	
	Aws::InitAPI(options);
	LOG_NORMAL("Aws::InitAPI called.");
#endif
}

void FAWSCoreModule::ShutdownModule()
{
#if WITH_AWSCORE
	Aws::ShutdownAPI(options);
	LOG_NORMAL("Aws::ShutdownAPI called.");
	FreeDependency(AWSCoreLibraryHandle);
	FreeDependency(AWSCommonLibraryHandle);
	FreeDependency(AWSEventStreamLibraryHandle);
	FreeDependency(AWSChecksumsLibraryHandle);
	LOG_NORMAL("Shutting down AWSCore Module...");
#endif
}

bool FAWSCoreModule::LoadDependency(const FString& Dir, const FString& Name, void*& Handle)
{
	FString Lib = Name + TEXT(".") + FPlatformProcess::GetModuleExtension();
	FString Path = Dir.IsEmpty() ? *Lib : FPaths::Combine(*Dir, *Lib);

	Handle = FPlatformProcess::GetDllHandle(*Path);

	if (Handle == nullptr)
	{
		LOG_ERROR(FString::Printf(TEXT("Dependency %s failed to load in directory %s"), *Lib, *Dir));
		return false;
	}

	LOG_NORMAL(FString::Printf(TEXT("Dependency %s successfully loaded from directory %s"), *Lib, *Dir));
	return true;
}

void FAWSCoreModule::FreeDependency(void*& Handle)
{
#if !PLATFORM_LINUX
	if (Handle != nullptr)
	{
		FPlatformProcess::FreeDllHandle(Handle);
		Handle = nullptr;
	}
#endif
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAWSCoreModule, AWSCore);
