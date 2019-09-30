// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameLiftTutorialPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class GAMELIFTTUTORIAL_API AGameLiftTutorialPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	AGameLiftTutorialPlayerController();

	UPROPERTY()
	FString	PlayerSessionId;
};
