// Fill out your copyright notice in the Description page of Project Settings.


#include "MenuGameMode.h"
#include "MenuHUD.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFilemanager.h"

AMenuGameMode::AMenuGameMode()
{
	// clear the webcache folder in the saved folder
	FString SavedDirPath = FPaths::ProjectSavedDir();
	FString WebCacheDirPath = SavedDirPath + "webcache/";
	UE_LOG(LogTemp, Warning, TEXT("saved directory path: %s"), *(SavedDirPath));
	UE_LOG(LogTemp, Warning, TEXT("Webcache directory path: %s"), *(WebCacheDirPath));

	if (FPlatformFileManager::Get().GetPlatformFile().DeleteDirectoryRecursively(*WebCacheDirPath)) {
		UE_LOG(LogTemp, Warning, TEXT("Webcache directory was deleted"));
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("Webcache directory was not deleted"));

	}
	UE_LOG(LogTemp, Warning, TEXT("After Webcache directory was deleted or not"));

	HUDClass = AMenuHUD::StaticClass();
}