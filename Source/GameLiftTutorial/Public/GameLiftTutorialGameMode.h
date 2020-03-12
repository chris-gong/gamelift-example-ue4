// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Runtime/Online/HTTP/Public/Http.h"
#include "GameLiftTutorialGameMode.generated.h"

class FGameLiftServerSDKModule;

struct FStartGameSessionState
{
	bool Status;
};

struct FUpdateGameSessionState
{
	FString LatestBackfillTicketId;
};

struct FProcessTerminateState
{
	bool Status;
};

struct FHealthCheckState
{
	bool Status;
};

UCLASS(minimalapi)
class AGameLiftTutorialGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AGameLiftTutorialGameMode();

	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;

	virtual void Logout(AController* Exiting) override;

protected:
	virtual void BeginPlay() override;

	virtual FString InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal) override;

private:
	FHttpModule* HttpModule;

	FString AssignMatchResultsUrl;

	FStartGameSessionState* StartGameSessionState;
	FUpdateGameSessionState* UpdateGameSessionState;
	FProcessTerminateState* ProcessTerminateState;
	FHealthCheckState* HealthCheckState;
	
	FTimerHandle CheckPlayerCountHandle;
	FTimerHandle StopBackfillHandle;
	FTimerHandle EndGameHandle;

	void CheckPlayerCount();
	void StopBackfill();
	void EndGame();

	void OnAssignMatchResultsResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
};

