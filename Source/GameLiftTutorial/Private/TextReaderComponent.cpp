// Fill out your copyright notice in the Description page of Project Settings.


#include "TextReaderComponent.h"
#include "Runtime/Core/Public/Misc/Paths.h"
#include "Runtime/Core/Public/HAL/PlatformFilemanager.h"
#include "Runtime/Core/Public/Misc/FileHelper.h"

UTextReaderComponent::UTextReaderComponent()
{

}

FString UTextReaderComponent::ReadFile(FString filePath)
{
	//FPaths::ProjectDir()
	//Read file from [project]/Content/
	FString directory = FPaths::ProjectContentDir();
	FString result;
	IPlatformFile& file = FPlatformFileManager::Get().GetPlatformFile();
	if (file.DirectoryExists(*directory)) {
		FString myFile = directory + "/" + filePath;
		FFileHelper::LoadFileToString(result, *myFile);
	}
	return result;
}