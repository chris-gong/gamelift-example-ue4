// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "GameLiftTutorialGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class GAMELIFTTUTORIAL_API UGameLiftTutorialGameInstance : public UGameInstance
{
	GENERATED_BODY()
	
public:
	UGameLiftTutorialGameInstance(const FObjectInitializer& ObjectInitializer);

	// AWS Stuff
	FString IdToken;
	FString AccessToken;
	FString RefreshToken;

	// GameLift Stuff
	FString MatchmakingTicketId;
};
