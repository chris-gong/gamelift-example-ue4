// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Runtime/Online/HTTP/Public/Http.h"
#include "GameLiftServerSDK.h"
#include "GameLiftTutorialGameMode.generated.h"

class AGameLiftTutorialGameState;

UENUM()
enum class EUpdateReason : uint8
{
	BACKFILL_INITIATED, // custom one to tell whether or not a backfill request has been updated yet
	MATCHMAKING_DATA_UPDATED,
	BACKFILL_FAILED,
	BACKFILL_TIMED_OUT,
	BACKFILL_CANCELLED,
	BACKFILL_COMPLETED, // custom one to tell whether we have acknowledged that the backfill request is done
	NO_UPDATE // custom one to say that updategamesession lambda handler hasn't been called yet
};

USTRUCT()
struct FStartGameSessionState
{
	GENERATED_BODY();

	UPROPERTY()
	bool Status;

	UPROPERTY()
	FString MatchmakingConfigurationArn;

	TMap<FString, FPlayer> PlayerIdToPlayer;

	FStartGameSessionState() {
		Status = false;
	}
};

USTRUCT()
struct FUpdateGameSessionState
{
	GENERATED_BODY();

	UPROPERTY()
	EUpdateReason Reason;
	UPROPERTY()
	FString MatchmakingConfigurationArn;
	UPROPERTY()
	FString LatestBackfillTicketId;
	TMap<FString, FPlayer> PlayerIdToPlayer;

	FUpdateGameSessionState() {
		Reason = EUpdateReason::NO_UPDATE;
	}
};

USTRUCT()
struct FProcessTerminateState
{
	GENERATED_BODY();

	UPROPERTY()
	bool Status;

	// made this a long variable since the GetTerminationTime method returns an outcome with a long and did not feel like converting
	// long variables can not be upropertiees
	long TerminationTime; 

	FProcessTerminateState() {
		Status = false;
	}
};

USTRUCT()
struct FHealthCheckState
{
	GENERATED_BODY();

	UPROPERTY()
	bool Status;

	FHealthCheckState() {
		Status = false;
	}
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

	FString ApiUrl;
	FString AssignMatchResultsUrl;

	bool GameSessionActivated;
	bool WaitingForPlayersToJoin;
	int WaitingForPlayersToJoinTime;
	int TimeUntilGameOver;
	FString LatestBackfillTicketId;
	TMap<FString, FPlayer> ConnectedPlayers; // kind of misleading, because it's supposed to represent the players who should be connected

	UPROPERTY()
	FStartGameSessionState StartGameSessionState;
	UPROPERTY()
	FUpdateGameSessionState UpdateGameSessionState;
	UPROPERTY()
	FProcessTerminateState ProcessTerminateState;
	UPROPERTY()
	FHealthCheckState HealthCheckState;
	
	FTimerHandle CountDownUntilGameOverHandle;
	FTimerHandle EndGameHandle;
	FTimerHandle PickAWinningTeamHandle;
	FTimerHandle HandleBackfillUpdatesHandle;

	void CountDownUntilGameOver();
	void EndGame();
	void PickAWinningTeam();
	void HandleBackfillUpdates();

	void CreateBackfillRequest(FString GameSessionArn, FString MatchmakingConfigurationArn, TMap<FString, FPlayer> Players);

	void OnAssignMatchResultsResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

	FString ServerPassword;

	AGameLiftTutorialGameState* GameLiftTutorialGameState;
};

