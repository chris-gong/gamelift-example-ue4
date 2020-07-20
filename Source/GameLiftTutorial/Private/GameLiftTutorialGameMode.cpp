// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "GameLiftTutorialGameMode.h"
#include "GameLiftTutorialCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "GameLiftTutorialHUD.h"
#include "GameLiftTutorialPlayerState.h"
#include "GameLiftTutorialGameState.h"
#include "TextReaderComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Json.h"
#include "JsonUtilities.h"

AGameLiftTutorialGameMode::AGameLiftTutorialGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPersonCPP/Blueprints/ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
		HUDClass = AGameLiftTutorialHUD::StaticClass();
		PlayerStateClass = AGameLiftTutorialPlayerState::StaticClass();
		GameStateClass = AGameLiftTutorialGameState::StaticClass();
	}

	UTextReaderComponent* TextReader = CreateDefaultSubobject<UTextReaderComponent>(TEXT("TextReaderComp"));
	ApiUrl = TextReader->ReadFile("Urls/ApiUrl.txt");

	HttpModule = &FHttpModule::Get();

	RemainingGameTime = 240;
	GameSessionActivated = false;

	WaitingForPlayersToJoin = false;
	TimeSpentWaitingForPlayersToJoin = 0;
}

void AGameLiftTutorialGameMode::BeginPlay() {
	Super::BeginPlay();

#if WITH_GAMELIFT
	auto InitSDKOutcome = Aws::GameLift::Server::InitSDK();

	if (InitSDKOutcome.IsSuccess()) {
		auto OnStartGameSession = [](Aws::GameLift::Server::Model::GameSession GameSessionObj, void* Params)
		{
			FStartGameSessionState* State = (FStartGameSessionState*)Params;

			State->Status = Aws::GameLift::Server::ActivateGameSession().IsSuccess();

			FString MatchmakerData = GameSessionObj.GetMatchmakerData();

			TSharedPtr<FJsonObject> JsonObject;
			TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(MatchmakerData);

			if (FJsonSerializer::Deserialize(Reader, JsonObject)) {
				State->MatchmakingConfigurationArn = JsonObject->GetStringField("matchmakingConfigurationArn");

				TArray<TSharedPtr<FJsonValue>> Teams = JsonObject->GetArrayField("teams");
				for (TSharedPtr<FJsonValue> Team : Teams) {
					TSharedPtr<FJsonObject> TeamObj = Team->AsObject();
					FString TeamName = TeamObj->GetStringField("name");

					TArray<TSharedPtr<FJsonValue>> Players = TeamObj->GetArrayField("players");

					for (TSharedPtr<FJsonValue> Player : Players) {
						TSharedPtr<FJsonObject> PlayerObj = Player->AsObject();
						FString PlayerId = PlayerObj->GetStringField("playerId");

						TSharedPtr<FJsonObject> Attributes = PlayerObj->GetObjectField("attributes");
						TSharedPtr<FJsonObject> Skill = Attributes->GetObjectField("skill");
						FString SkillValue = Skill->GetStringField("valueAttribute");
						auto SkillAttributeValue = new Aws::GameLift::Server::Model::AttributeValue(FCString::Atod(*SkillValue));

						Aws::GameLift::Server::Model::Player AwsPlayerObj;

						AwsPlayerObj.SetPlayerId(TCHAR_TO_ANSI(*PlayerId));
						AwsPlayerObj.SetTeam(TCHAR_TO_ANSI(*TeamName));
						AwsPlayerObj.AddPlayerAttribute("skill", *SkillAttributeValue);

						State->PlayerIdToPlayer.Add(PlayerId, AwsPlayerObj);
					}
				}
			}
		};

		auto OnUpdateGameSession = [](Aws::GameLift::Server::Model::UpdateGameSession UpdateGameSessionObj, void* Params)
		{
			FUpdateGameSessionState* State = (FUpdateGameSessionState*)Params;

			auto Reason = UpdateGameSessionObj.GetUpdateReason();

			if (Reason == Aws::GameLift::Server::Model::UpdateReason::MATCHMAKING_DATA_UPDATED) {
				State->Reason = EUpdateReason::MATCHMAKING_DATA_UPDATED;

				auto GameSessionObj = UpdateGameSessionObj.GetGameSession();
				FString MatchmakerData = GameSessionObj.GetMatchmakerData();

				TSharedPtr<FJsonObject> JsonObject;
				TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(MatchmakerData);

				if (FJsonSerializer::Deserialize(Reader, JsonObject)) {
					TArray<TSharedPtr<FJsonValue>> Teams = JsonObject->GetArrayField("teams");
					for (TSharedPtr<FJsonValue> Team : Teams) {
						TSharedPtr<FJsonObject> TeamObj = Team->AsObject();
						FString TeamName = TeamObj->GetStringField("name");

						TArray<TSharedPtr<FJsonValue>> Players = TeamObj->GetArrayField("players");

						for (TSharedPtr<FJsonValue> Player : Players) {
							TSharedPtr<FJsonObject> PlayerObj = Player->AsObject();
							FString PlayerId = PlayerObj->GetStringField("playerId");

							TSharedPtr<FJsonObject> Attributes = PlayerObj->GetObjectField("attributes");
							TSharedPtr<FJsonObject> Skill = Attributes->GetObjectField("skill");
							FString SkillValue = Skill->GetStringField("valueAttribute");
							auto SkillAttributeValue = new Aws::GameLift::Server::Model::AttributeValue(FCString::Atod(*SkillValue));

							Aws::GameLift::Server::Model::Player AwsPlayerObj;

							AwsPlayerObj.SetPlayerId(TCHAR_TO_ANSI(*PlayerId));
							AwsPlayerObj.SetTeam(TCHAR_TO_ANSI(*TeamName));
							AwsPlayerObj.AddPlayerAttribute("skill", *SkillAttributeValue);

							State->PlayerIdToPlayer.Add(PlayerId, AwsPlayerObj);
						}
					}
				}
			}
			else if (Reason == Aws::GameLift::Server::Model::UpdateReason::BACKFILL_CANCELLED) {
				State->Reason = EUpdateReason::BACKFILL_CANCELLED;
			}
			else if (Reason == Aws::GameLift::Server::Model::UpdateReason::BACKFILL_FAILED) {
				State->Reason = EUpdateReason::BACKFILL_FAILED;
			}
			else if (Reason == Aws::GameLift::Server::Model::UpdateReason::BACKFILL_TIMED_OUT) {
				State->Reason = EUpdateReason::BACKFILL_TIMED_OUT;
			}
		};

		auto OnProcessTerminate = [](void* Params)
		{
			FProcessTerminateState* State = (FProcessTerminateState*)Params;

			auto GetTerminationTimeOutcome = Aws::GameLift::Server::GetTerminationTime();
			if (GetTerminationTimeOutcome.IsSuccess()) {
				State->TerminationTime = GetTerminationTimeOutcome.GetResult();
			}

			State->Status = true;
		};

		auto OnHealthCheck = [](void* Params)
		{
			FHealthCheckState* State = (FHealthCheckState*)Params;
			State->Status = true;

			return State->Status;
		};

		TArray<FString> CommandLineTokens;
		TArray<FString> CommandLineSwitches;
		int Port = FURL::UrlConfig.DefaultPort;

		// GameLiftTutorialServer.exe token -port=7777
		FCommandLine::Parse(FCommandLine::Get(), CommandLineTokens, CommandLineSwitches);

		for (FString Str : CommandLineSwitches) {
			FString Key;
			FString Value;

			if (Str.Split("=", &Key, &Value)) {
				if (Key.Equals("port")) {
					Port = FCString::Atoi(*Value);
				}
				else if (Key.Equals("password")) {
					ServerPassword = Value;
				}
			}
		}

		const char* LogFile = "aLogFile.txt";
		const char** LogFiles = &LogFile;
		auto LogParams = new Aws::GameLift::Server::LogParameters(LogFiles, 1);

		auto Params = new Aws::GameLift::Server::ProcessParameters(
			OnStartGameSession,
			&StartGameSessionState,
			OnUpdateGameSession,
			&UpdateGameSessionState,
			OnProcessTerminate,
			&ProcessTerminateState,
			OnHealthCheck,
			&HealthCheckState,
			Port,
			*LogParams
		);

		auto ProcessReadyOutcome = Aws::GameLift::Server::ProcessReady(*Params);
	}
#endif
	GetWorldTimerManager().SetTimer(HandleGameSessionUpdateHandle, this, &AGameLiftTutorialGameMode::HandleGameSessionUpdate, 1.0f, true, 5.0f);
	GetWorldTimerManager().SetTimer(HandleProcessTerminationHandle, this, &AGameLiftTutorialGameMode::HandleProcessTermination, 1.0f, true, 5.0f);
	/*if (GameState != nullptr) {
		AGameLiftTutorialGameState* GameLiftTutorialGameState = Cast<AGameLiftTutorialGameState>(GameState);
		if (GameLiftTutorialGameState != nullptr) {
			GameLiftTutorialGameState->LatestEvent = "GameEnded";
			GameLiftTutorialGameState->WinningTeam = "cowboys";
		}
	}*/
}

void AGameLiftTutorialGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) {
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
#if WITH_GAMELIFT
	if (Options.Len() > 0) {
		const FString& PlayerSessionId = UGameplayStatics::ParseOption(Options, "PlayerSessionId");
		const FString& PlayerId = UGameplayStatics::ParseOption(Options, "PlayerId");

		if (PlayerSessionId.Len() > 0 && PlayerId.Len() > 0) {
			Aws::GameLift::Server::Model::DescribePlayerSessionsRequest DescribePlayerSessionsRequest;
			DescribePlayerSessionsRequest.SetPlayerSessionId(TCHAR_TO_ANSI(*PlayerSessionId));

			auto DescribePlayerSessionsOutcome = Aws::GameLift::Server::DescribePlayerSessions(DescribePlayerSessionsRequest);
			if (DescribePlayerSessionsOutcome.IsSuccess()) {
				auto DescribePlayerSessionsResult = DescribePlayerSessionsOutcome.GetResult();
				int Count = 1;
				auto PlayerSessions = DescribePlayerSessionsResult.GetPlayerSessions(Count);
				if (PlayerSessions != nullptr) {
					auto PlayerSession = PlayerSessions[0];
					FString ExpectedPlayerId = PlayerSession.GetPlayerId();
					auto PlayerStatus = PlayerSession.GetStatus();

					if (ExpectedPlayerId.Equals(PlayerId) && PlayerStatus == Aws::GameLift::Server::Model::PlayerSessionStatus::RESERVED) {
						auto AcceptPlayerSessionOutcome = Aws::GameLift::Server::AcceptPlayerSession(TCHAR_TO_ANSI(*PlayerSessionId));

						if (!AcceptPlayerSessionOutcome.IsSuccess()) {
							ErrorMessage = "Unauthorized";
						}
					}
					else {
						ErrorMessage = "Unauthorized";
					}
				}
				else {
					ErrorMessage = "Unauthorized";
				}
			}
			else {
				ErrorMessage = "Unauthorized";
			}
		}
		else {
			ErrorMessage = "Unauthorized";
		}
	}
	else {
		ErrorMessage = "Unauthorized";
	}
#endif
}

void AGameLiftTutorialGameMode::Logout(AController* Exiting) {
#if WITH_GAMELIFT
	if (LatestBackfillTicketId.Len() > 0) {
		auto GameSessionIdOutcome = Aws::GameLift::Server::GetGameSessionId();
		if (GameSessionIdOutcome.IsSuccess()) {
			FString GameSessionId = GameSessionIdOutcome.GetResult();
			FString MatchmakingConfigurationArn = StartGameSessionState.MatchmakingConfigurationArn;
			StopBackfillRequest(GameSessionId, MatchmakingConfigurationArn, LatestBackfillTicketId);
		}
	}
	if (Exiting != nullptr) {
		APlayerState* PlayerState = Exiting->PlayerState;
		if (PlayerState != nullptr) {
			AGameLiftTutorialPlayerState* GameLiftTutorialPlayerState = Cast<AGameLiftTutorialPlayerState>(PlayerState);
			const FString& PlayerSessionId = GameLiftTutorialPlayerState->PlayerSessionId;
			if (PlayerSessionId.Len() > 0) {
				Aws::GameLift::Server::RemovePlayerSession(TCHAR_TO_ANSI(*PlayerSessionId));
			}
		}
	}
#endif
	Super::Logout(Exiting);
}

FString AGameLiftTutorialGameMode::InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal) {
	FString InitializedString = Super::InitNewPlayer(NewPlayerController, UniqueId, Options, Portal);

	/*if (NewPlayerController != nullptr) {
		APlayerState* PlayerState = NewPlayerController->PlayerState;
		if (PlayerState != nullptr) {
			AGameLiftTutorialPlayerState* GameLiftTutorialPlayerState = Cast<AGameLiftTutorialPlayerState>(PlayerState);
			if (GameLiftTutorialPlayerState != nullptr) {
				if (FMath::RandRange(0, 1) == 0) {
					GameLiftTutorialPlayerState->Team = "cowboys";
				}
				else {
					GameLiftTutorialPlayerState->Team = "aliens";
				}
			}
		}
	}*/
#if WITH_GAMELIFT
	const FString& PlayerSessionId = UGameplayStatics::ParseOption(Options, "PlayerSessionId");
	const FString& PlayerId = UGameplayStatics::ParseOption(Options, "PlayerId");

	if (NewPlayerController != nullptr) {
		APlayerState* PlayerState = NewPlayerController->PlayerState;
		if (PlayerState != nullptr) {
			AGameLiftTutorialPlayerState* GameLiftTutorialPlayerState = Cast<AGameLiftTutorialPlayerState>(PlayerState);
			if (GameLiftTutorialPlayerState != nullptr) {
				GameLiftTutorialPlayerState->PlayerSessionId = *PlayerSessionId;
				GameLiftTutorialPlayerState->MatchmakingPlayerId = *PlayerId;

				if (UpdateGameSessionState.PlayerIdToPlayer.Num() > 0) {
					if (UpdateGameSessionState.PlayerIdToPlayer.Contains(PlayerId)) {
						auto PlayerObj = UpdateGameSessionState.PlayerIdToPlayer.Find(PlayerId);
						FString Team = PlayerObj->GetTeam();
						GameLiftTutorialPlayerState->Team = *Team;
					}
				}
				else if (StartGameSessionState.PlayerIdToPlayer.Num() > 0) {
					if (StartGameSessionState.PlayerIdToPlayer.Contains(PlayerId)) {
						auto PlayerObj = StartGameSessionState.PlayerIdToPlayer.Find(PlayerId);
						FString Team = PlayerObj->GetTeam();
						GameLiftTutorialPlayerState->Team = *Team;
					}
				}
			}
		}
	}
#endif
	return InitializedString;
}

void AGameLiftTutorialGameMode::CountDownUntilGameOver() {
	if (GameState != nullptr) {
		AGameLiftTutorialGameState* GameLiftTutorialGameState = Cast<AGameLiftTutorialGameState>(GameState);
		if (GameLiftTutorialGameState != nullptr) {
			GameLiftTutorialGameState->LatestEvent = FString::FromInt(RemainingGameTime) + " seconds until the game is over";
		}
	}

	if (RemainingGameTime > 0) {
		RemainingGameTime--;
	}
	else {
		GetWorldTimerManager().ClearTimer(CountDownUntilGameOverHandle);
	}
}

void AGameLiftTutorialGameMode::EndGame() {
	GetWorldTimerManager().ClearTimer(CountDownUntilGameOverHandle);
	GetWorldTimerManager().ClearTimer(EndGameHandle);
	GetWorldTimerManager().ClearTimer(PickAWinningTeamHandle);
	GetWorldTimerManager().ClearTimer(HandleProcessTerminationHandle);
	GetWorldTimerManager().ClearTimer(HandleGameSessionUpdateHandle);
	GetWorldTimerManager().ClearTimer(SuspendBackfillHandle);

#if WITH_GAMELIFT
	Aws::GameLift::Server::TerminateGameSession();
	Aws::GameLift::Server::ProcessEnding();
	FGenericPlatformMisc::RequestExit(false);
#endif
}

void AGameLiftTutorialGameMode::PickAWinningTeam() {
	GetWorldTimerManager().ClearTimer(CountDownUntilGameOverHandle);

#if WITH_GAMELIFT
	if (GameState != nullptr) {
		AGameLiftTutorialGameState* GameLiftTutorialGameState = Cast<AGameLiftTutorialGameState>(GameState);
		if (GameLiftTutorialGameState != nullptr) {
			GameLiftTutorialGameState->LatestEvent = "GameEnded";

			if (FMath::RandRange(0, 1) == 0) {
				GameLiftTutorialGameState->WinningTeam = "cowboys";
			}
			else {
				GameLiftTutorialGameState->WinningTeam = "aliens";
			}

			TSharedPtr<FJsonObject> RequestObj = MakeShareable(new FJsonObject);
			RequestObj->SetStringField("winningTeam", GameLiftTutorialGameState->WinningTeam);

			auto GetGameSessionIdOutcome = Aws::GameLift::Server::GetGameSessionId();
			if (GetGameSessionIdOutcome.IsSuccess()) {
				RequestObj->SetStringField("gameSessionId", GetGameSessionIdOutcome.GetResult());

				FString RequestBody;
				TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
				if (FJsonSerializer::Serialize(RequestObj.ToSharedRef(), Writer)) {
					TSharedRef<IHttpRequest> RecordMatchResultRequest = HttpModule->CreateRequest();
					RecordMatchResultRequest->OnProcessRequestComplete().BindUObject(this, &AGameLiftTutorialGameMode::OnRecordMatchResultResponseReceived);
					RecordMatchResultRequest->SetURL(ApiUrl + "/recordmatchresult");
					RecordMatchResultRequest->SetVerb("POST");
					RecordMatchResultRequest->SetHeader("Authorization", ServerPassword);
					RecordMatchResultRequest->SetHeader("Content-Type", "application/json");
					RecordMatchResultRequest->SetContentAsString(RequestBody);
					RecordMatchResultRequest->ProcessRequest();
				}
				else {
					GetWorldTimerManager().SetTimer(EndGameHandle, this, &AGameLiftTutorialGameMode::EndGame, 1.0f, false, 5.0f);
				}
			}
			else {
				GetWorldTimerManager().SetTimer(EndGameHandle, this, &AGameLiftTutorialGameMode::EndGame, 1.0f, false, 5.0f);
			}
		}
		else {
			GetWorldTimerManager().SetTimer(EndGameHandle, this, &AGameLiftTutorialGameMode::EndGame, 1.0f, false, 5.0f);
		}
	}
	else {
		GetWorldTimerManager().SetTimer(EndGameHandle, this, &AGameLiftTutorialGameMode::EndGame, 1.0f, false, 5.0f);
	}
#endif
}

void AGameLiftTutorialGameMode::HandleProcessTermination() {
	if (ProcessTerminateState.Status) {
		GetWorldTimerManager().ClearTimer(CountDownUntilGameOverHandle);
		GetWorldTimerManager().ClearTimer(HandleProcessTerminationHandle);
		GetWorldTimerManager().ClearTimer(HandleGameSessionUpdateHandle);
		GetWorldTimerManager().ClearTimer(SuspendBackfillHandle);

#if WITH_GAMELIFT
		if (LatestBackfillTicketId.Len() > 0) {
			auto GameSessionIdOutcome = Aws::GameLift::Server::GetGameSessionId();
			if (GameSessionIdOutcome.IsSuccess()) {
				FString GameSessionArn = FString(GameSessionIdOutcome.GetResult());
				FString MatchmakingConfigurationArn = StartGameSessionState.MatchmakingConfigurationArn;
				StopBackfillRequest(GameSessionArn, MatchmakingConfigurationArn, LatestBackfillTicketId);
			}
		}
#endif

		FString ProcessInterruptionMessage;

		if (ProcessTerminateState.TerminationTime <= 0L) {
			ProcessInterruptionMessage = "Server process could shut down at any time";
		}
		else {
			long TimeLeft = (long)(ProcessTerminateState.TerminationTime - FDateTime::Now().ToUnixTimestamp());
			ProcessInterruptionMessage = FString::Printf(TEXT("Server process scheduled to terminate in %ld seconds"), TimeLeft);
		}

		if (GameState != nullptr) {
			AGameLiftTutorialGameState* GameLiftTutorialGameState = Cast<AGameLiftTutorialGameState>(GameState);
			if (GameLiftTutorialGameState != nullptr) {
				GameLiftTutorialGameState->LatestEvent = ProcessInterruptionMessage;
			}
		}

		GetWorldTimerManager().SetTimer(EndGameHandle, this, &AGameLiftTutorialGameMode::EndGame, 1.0f, false, 10.0f);
	}
}

void AGameLiftTutorialGameMode::HandleGameSessionUpdate() {
#if WITH_GAMELIFT
	if (!GameSessionActivated) {
		if (StartGameSessionState.Status) {
			GameSessionActivated = true;

			ExpectedPlayers = StartGameSessionState.PlayerIdToPlayer;

			WaitingForPlayersToJoin = true;

			GetWorldTimerManager().SetTimer(PickAWinningTeamHandle, this, &AGameLiftTutorialGameMode::PickAWinningTeam, 1.0f, false, (float)RemainingGameTime);
			GetWorldTimerManager().SetTimer(SuspendBackfillHandle, this, &AGameLiftTutorialGameMode::SuspendBackfill, 1.0f, false, (float)(RemainingGameTime - 60));
			GetWorldTimerManager().SetTimer(CountDownUntilGameOverHandle, this, &AGameLiftTutorialGameMode::CountDownUntilGameOver, 1.0f, true, 0.0f);
		}
	}
	else if (WaitingForPlayersToJoin) {
		if (TimeSpentWaitingForPlayersToJoin < 60) {
			auto GameSessionIdOutcome = Aws::GameLift::Server::GetGameSessionId();
			if (GameSessionIdOutcome.IsSuccess()) {
				FString GameSessionId = FString(GameSessionIdOutcome.GetResult());

				Aws::GameLift::Server::Model::DescribePlayerSessionsRequest DescribePlayerSessionsRequest;
				DescribePlayerSessionsRequest.SetGameSessionId(TCHAR_TO_ANSI(*GameSessionId));
				DescribePlayerSessionsRequest.SetPlayerSessionStatusFilter("RESERVED");

				auto DescribePlayerSessionsOutcome = Aws::GameLift::Server::DescribePlayerSessions(DescribePlayerSessionsRequest);
				if (DescribePlayerSessionsOutcome.IsSuccess()) {
					auto DescribePlayerSessionsResult = DescribePlayerSessionsOutcome.GetResult();
					int Count = DescribePlayerSessionsResult.GetPlayerSessionsCount();
					if (Count == 0) {
						UpdateGameSessionState.Reason = EUpdateReason::BACKFILL_COMPLETED;

						WaitingForPlayersToJoin = false;
						TimeSpentWaitingForPlayersToJoin = 0;
					}
					else {
						TimeSpentWaitingForPlayersToJoin++;
					}
				}
				else {
					TimeSpentWaitingForPlayersToJoin++;
				}
			}
			else {
				TimeSpentWaitingForPlayersToJoin++;
			}
		}
		else {
			UpdateGameSessionState.Reason = EUpdateReason::BACKFILL_COMPLETED;

			WaitingForPlayersToJoin = false;
			TimeSpentWaitingForPlayersToJoin = 0;
		}
	}
	else if (UpdateGameSessionState.Reason == EUpdateReason::MATCHMAKING_DATA_UPDATED) {
		LatestBackfillTicketId = "";
		ExpectedPlayers = UpdateGameSessionState.PlayerIdToPlayer;

		WaitingForPlayersToJoin = true;
	}
	else if (UpdateGameSessionState.Reason == EUpdateReason::BACKFILL_CANCELLED || UpdateGameSessionState.Reason == EUpdateReason::BACKFILL_COMPLETED
		|| UpdateGameSessionState.Reason == EUpdateReason::BACKFILL_FAILED || UpdateGameSessionState.Reason == EUpdateReason::BACKFILL_TIMED_OUT) {
		LatestBackfillTicketId = "";

		TArray<APlayerState*> PlayerStates = GetWorld()->GetGameState()->PlayerArray;

		TMap<FString, Aws::GameLift::Server::Model::Player> ConnectedPlayers;
		for (APlayerState* PlayerState : PlayerStates) {
			if (PlayerState != nullptr) {
				AGameLiftTutorialPlayerState* GameLiftTutorialPlayerState = Cast<AGameLiftTutorialPlayerState>(PlayerState);
				if (GameLiftTutorialPlayerState != nullptr) {
					auto PlayerObj = ExpectedPlayers.Find(GameLiftTutorialPlayerState->MatchmakingPlayerId);
					if (PlayerObj != nullptr) {
						ConnectedPlayers.Add(GameLiftTutorialPlayerState->MatchmakingPlayerId, *PlayerObj);
					}
				}
			}
		}

		if (ConnectedPlayers.Num() == 0) {
			EndGame();
		}
		else if (ConnectedPlayers.Num() < 4) {
			auto GameSessionIdOutcome = Aws::GameLift::Server::GetGameSessionId();
			if (GameSessionIdOutcome.IsSuccess()) {
				FString GameSessionId = FString(GameSessionIdOutcome.GetResult());
				FString MatchmakingConfigurationArn = StartGameSessionState.MatchmakingConfigurationArn;
				LatestBackfillTicketId = CreateBackfillRequest(GameSessionId, MatchmakingConfigurationArn, ConnectedPlayers);
				if (LatestBackfillTicketId.Len() > 0) {
					UpdateGameSessionState.Reason = EUpdateReason::BACKFILL_INITIATED;
				}
			}
		}
	}
#endif
}

void AGameLiftTutorialGameMode::OnRecordMatchResultResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful) {
	GetWorldTimerManager().SetTimer(EndGameHandle, this, &AGameLiftTutorialGameMode::EndGame, 1.0f, false, 5.0f);
}

void AGameLiftTutorialGameMode::SuspendBackfill() {
	GetWorldTimerManager().ClearTimer(HandleGameSessionUpdateHandle);
#if WITH_GAMELIFT
	if (LatestBackfillTicketId.Len() > 0) {
		auto GameSessionIdOutcome = Aws::GameLift::Server::GetGameSessionId();
		if (GameSessionIdOutcome.IsSuccess()) {
			FString GameSessionId = GameSessionIdOutcome.GetResult();
			FString MatchmakingConfigurationArn = StartGameSessionState.MatchmakingConfigurationArn;
			if (!StopBackfillRequest(GameSessionId, MatchmakingConfigurationArn, LatestBackfillTicketId)) {
				GetWorldTimerManager().SetTimer(SuspendBackfillHandle, this, &AGameLiftTutorialGameMode::SuspendBackfill, 1.0f, false, 1.0f);
			}
		}
		else {
			GetWorldTimerManager().SetTimer(SuspendBackfillHandle, this, &AGameLiftTutorialGameMode::SuspendBackfill, 1.0f, false, 1.0f);
		}
	}
#endif
}

FString AGameLiftTutorialGameMode::CreateBackfillRequest(FString GameSessionArn, FString MatchmakingConfigurationArn, TMap<FString, Aws::GameLift::Server::Model::Player> Players) {
#if WITH_GAMELIFT
	Aws::GameLift::Server::Model::StartMatchBackfillRequest StartMatchBackfillRequest;
	StartMatchBackfillRequest.SetGameSessionArn(TCHAR_TO_ANSI(*GameSessionArn));
	StartMatchBackfillRequest.SetMatchmakingConfigurationArn(TCHAR_TO_ANSI(*MatchmakingConfigurationArn));

	for (auto& Elem : Players) {
		auto PlayerObj = Elem.Value;
		StartMatchBackfillRequest.AddPlayer(PlayerObj);
	}

	auto StartMatchBackfillOutcome = Aws::GameLift::Server::StartMatchBackfill(StartMatchBackfillRequest);
	if (StartMatchBackfillOutcome.IsSuccess()) {
		return StartMatchBackfillOutcome.GetResult().GetTicketId();
	}
	else {
		return "";
	}
#endif
	return "";
}

bool AGameLiftTutorialGameMode::StopBackfillRequest(FString GameSessionArn, FString MatchmakingConfigurationArn, FString TicketId) {
#if WITH_GAMELIFT
	Aws::GameLift::Server::Model::StopMatchBackfillRequest StopMatchBackfillRequest;
	StopMatchBackfillRequest.SetGameSessionArn(TCHAR_TO_ANSI(*GameSessionArn));
	StopMatchBackfillRequest.SetMatchmakingConfigurationArn(TCHAR_TO_ANSI(*MatchmakingConfigurationArn));
	StopMatchBackfillRequest.SetTicketId(TCHAR_TO_ANSI(*TicketId));

	auto StopMatchBackfillOutcome = Aws::GameLift::Server::StopMatchBackfill(StopMatchBackfillRequest);

	return StopMatchBackfillOutcome.IsSuccess();
#endif
	return false;
}