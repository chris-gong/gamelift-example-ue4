// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameLiftTutorialGameMode.generated.h"

class FGameLiftServerSDKModule;

UCLASS(minimalapi)
class AGameLiftTutorialGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AGameLiftTutorialGameMode();

	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;

	virtual void Logout(AController* Exiting);

protected:
	virtual void BeginPlay() override;

	virtual FString InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal) override;

private:
	FGameLiftServerSDKModule* gameLiftSdkModule;

	UFUNCTION()
	void CheckPlayerReadyCount();

	UFUNCTION()
	void CheckPlayersLeft();

	UFUNCTION()
	void StartGame();

	UFUNCTION()
	void EndGame();

	UPROPERTY()
	int ReadyTimeCount;
	
	UPROPERTY()
	bool GameStarted;

	UPROPERTY()
	FTimerHandle ReadyCheckTimerHandle;

	UPROPERTY()
	FTimerHandle PlayersLeftTimerHandle;
};



