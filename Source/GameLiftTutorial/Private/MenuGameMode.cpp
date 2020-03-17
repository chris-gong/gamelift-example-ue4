// Fill out your copyright notice in the Description page of Project Settings.


#include "MenuGameMode.h"
#include "MenuHUD.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFilemanager.h"

AMenuGameMode::AMenuGameMode()
{
	HUDClass = AMenuHUD::StaticClass();
}