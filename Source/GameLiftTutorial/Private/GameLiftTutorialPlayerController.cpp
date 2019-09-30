// Fill out your copyright notice in the Description page of Project Settings.


#include "GameLiftTutorialPlayerController.h"

AGameLiftTutorialPlayerController::AGameLiftTutorialPlayerController() {
	PlayerSessionId = "";
}

void AGameLiftTutorialPlayerController::BeginPlay() {
	bShowMouseCursor = false;
}