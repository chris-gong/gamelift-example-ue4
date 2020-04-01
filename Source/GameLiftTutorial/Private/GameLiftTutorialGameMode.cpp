// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "GameLiftTutorialGameMode.h"
#include "GameLiftTutorial.h"
#include "GameLiftTutorialGameState.h"
#include "TextReaderComponent.h"
#include "Engine/Engine.h"
#include "GameLiftTutorialCharacter.h"
#include "GameLiftTutorialPlayerState.h"
#include "GameLiftTutorialHUD.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Json.h"
#include "JsonUtilities.h"
#include "GameFramework/GameSession.h"

AGameLiftTutorialGameMode::AGameLiftTutorialGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPersonCPP/Blueprints/ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
		PlayerStateClass = AGameLiftTutorialPlayerState::StaticClass();
		HUDClass = AGameLiftTutorialHUD::StaticClass();
		GameStateClass = AGameLiftTutorialGameState::StaticClass();
	}

	StartGameSessionState = new FStartGameSessionState();
	UpdateGameSessionState = new FUpdateGameSessionState();
	ProcessTerminateState = new FProcessTerminateState();
	HealthCheckState = new FHealthCheckState();

	UTextReaderComponent* TextReader = CreateDefaultSubobject<UTextReaderComponent>(TEXT("TextReaderComp"));
	ApiUrl = TextReader->ReadFile("SecretUrls/ApiUrl.txt");
	AssignMatchResultsUrl = ApiUrl + "/assignmatchresults";

	HttpModule = &FHttpModule::Get();
	
	ServerPassword = "";

	GameSessionActivated = false;
	WaitingForPlayersToJoin = false;
	WaitingForPlayersToJoinTime = 0;
	TimeUntilGameOver = 0;
}

void AGameLiftTutorialGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) {
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
	// kick the player out id the player did not pass a valid player session id
#if WITH_GAMELIFT
	if (*Options && Options.Len() > 0) {
		const FString& PlayerSessionId = UGameplayStatics::ParseOption(Options, "PlayerSessionId");
		if (PlayerSessionId.Len() > 0) {
			auto AcceptPlayerSessionOutcome = Aws::GameLift::Server::AcceptPlayerSession(TCHAR_TO_ANSI(*PlayerSessionId));
			if (!AcceptPlayerSessionOutcome.IsSuccess()) {
				ErrorMessage = FString("Unauthorized");
			}
		}
		else {
			ErrorMessage = FString("Unauthorized");
		}
	}
	else {
		ErrorMessage = FString("Unauthorized");
	}
#endif
}

void AGameLiftTutorialGameMode::Logout(AController* Exiting) {
	Super::Logout(Exiting);
	APlayerState* State = Exiting->PlayerState;
	if (State != nullptr) {
		AGameLiftTutorialPlayerState* PlayerState = Cast<AGameLiftTutorialPlayerState>(State);
		const FString& PlayerSessionId = PlayerState->PlayerSessionId;
		if (PlayerSessionId.Len() > 0) {
#if WITH_GAMELIFT
			Aws::GameLift::Server::RemovePlayerSession(TCHAR_TO_ANSI(*PlayerSessionId));
#endif
		}
	}
}

void AGameLiftTutorialGameMode::BeginPlay() {
	Super::BeginPlay();
	AGameStateBase* FGameState = GameState;
	if (FGameState != nullptr) {
		GameLiftTutorialGameState = Cast<AGameLiftTutorialGameState>(FGameState);
	}
	//Let's run this code only if GAMELIFT is enabled. Only with Server targets!
#if WITH_GAMELIFT
	UE_LOG(LogTemp, Warning, TEXT("khai loves pizza"));
	auto InitSDKOutcome = Aws::GameLift::Server::InitSDK();

	if (InitSDKOutcome.IsSuccess()) {
		Aws::GameLift::Server::StartGameSessionFn OnStartGameSession = [](Aws::GameLift::Server::Model::GameSession GameSessionObj, void* Params)
		{
			FStartGameSessionState* State = (FStartGameSessionState*)Params;

			State->Status = Aws::GameLift::Server::ActivateGameSession().IsSuccess();

			//extract matchmaker data 
			FString MatchmakerData = GameSessionObj.GetMatchmakerData();
			UE_LOG(LogTemp, Warning, TEXT("matchmaker data in onstartgamesession: %s"), *(MatchmakerData));

			// Create a pointer to hold the json serialized data
			TSharedPtr<FJsonObject> JsonObject;

			//Create a reader pointer to read the json data
			TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(MatchmakerData);

			//Deserialize the json data given Reader and the actual object to deserialize
			if (FJsonSerializer::Deserialize(Reader, JsonObject))
			{
				UE_LOG(LogTemp, Warning, TEXT("deserialized json response from get matchmaker data"));
				FString MatchmakingConfigurationArn = JsonObject->GetStringField("matchmakingConfigurationArn"); // need to check if this is correct vs getting the backfill ticket from the game session object
				if (MatchmakingConfigurationArn.Len() > 0) {
					UE_LOG(LogTemp, Warning, TEXT("MatchmakingConfigurationArn: %s"), *(MatchmakingConfigurationArn));
				}
				State->MatchmakingConfigurationArn = MatchmakingConfigurationArn;

				TArray<TSharedPtr<FJsonValue>> Teams = JsonObject->GetArrayField("teams");
				for (TSharedPtr<FJsonValue> Team : Teams) {
					TSharedPtr<FJsonObject> TeamObj = Team->AsObject();
					FString TeamName = TeamObj->GetStringField("name");
					if (TeamName.Len() > 0) {
						UE_LOG(LogTemp, Warning, TEXT("team: %s"), *(TeamName));
					}
					TArray<TSharedPtr<FJsonValue>> Players = TeamObj->GetArrayField("players");

					for (TSharedPtr<FJsonValue> Player : Players) {
						TSharedPtr<FJsonObject> PlayerObj = Player->AsObject();
						FString PlayerId = PlayerObj->GetStringField("playerId");
						TSharedPtr<FJsonObject> Attributes = PlayerObj->GetObjectField("attributes");
						TSharedPtr<FJsonObject> Skill = Attributes->GetObjectField("skill");
						FString SkillValue = Skill->GetStringField("valueAttribute");
						FAttributeValue SkillAttributeValue;
						SkillAttributeValue.m_N = FCString::Atof(*SkillValue);

						FPlayer FPlayerObj;

						if (PlayerId.Len() > 0) {
							UE_LOG(LogTemp, Warning, TEXT("player id: %s"), *(PlayerId));
						}
						FPlayerObj.m_playerId = PlayerId;
						FPlayerObj.m_team = TeamName;
						FPlayerObj.m_playerAttributes.Add("skill", SkillAttributeValue);
						//TODO: FIGURE OUT HOW TO GET THE PING INFORMATION FOR BELOW
						//FPlayerObj.m_latencyInMs.Add(? , ? );
						State->PlayerIdToPlayer.Add(PlayerId, FPlayerObj);
					}
				}
				UE_LOG(LogTemp, Warning, TEXT("processed matchmaking data for onstartgamesession"));
			}
		};

		Aws::GameLift::Server::UpdateGameSessionFn OnUpdateGameSession = [](Aws::GameLift::Server::Model::UpdateGameSession UpdateGameSessionObj, void* Params)
		{
			FUpdateGameSessionState* State = (FUpdateGameSessionState*)Params;

			Aws::GameLift::Server::Model::UpdateReason Reason = UpdateGameSessionObj.GetUpdateReason();

			if (Reason == Aws::GameLift::Server::Model::UpdateReason::MATCHMAKING_DATA_UPDATED) {
				State->Reason = EUpdateReason::MATCHMAKING_DATA_UPDATED;
				// extract matchmaker data
				Aws::GameLift::Server::Model::GameSession GameSessionObj = UpdateGameSessionObj.GetGameSession();
				FString MatchmakerData = GameSessionObj.GetMatchmakerData();
				UE_LOG(LogTemp, Warning, TEXT("matchmaker data in onupdategamesession: %s"), *(MatchmakerData));

				// Create a pointer to hold the json serialized data
				TSharedPtr<FJsonObject> JsonObject;

				//Create a reader pointer to read the json data
				TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(MatchmakerData);

				//Deserialize the json data given Reader and the actual object to deserialize
				if (FJsonSerializer::Deserialize(Reader, JsonObject))
				{
					UE_LOG(LogTemp, Warning, TEXT("deserialized json response from get matchmaker data"));
					FString LatestBackfillTicketId = JsonObject->GetStringField("autoBackfillTicketId"); // need to check if this is correct vs getting the backfill ticket from the game session object
					if (LatestBackfillTicketId.Len() > 0) {
						UE_LOG(LogTemp, Warning, TEXT("backfill ticket id: %s"), *(LatestBackfillTicketId));
					}
					State->LatestBackfillTicketId = LatestBackfillTicketId;

					FString MatchmakingConfigurationArn = JsonObject->GetStringField("matchmakingConfigurationArn"); // need to check if this is correct vs getting the backfill ticket from the game session object
					if (MatchmakingConfigurationArn.Len() > 0) {
						UE_LOG(LogTemp, Warning, TEXT("MatchmakingConfigurationArn: %s"), *(MatchmakingConfigurationArn));
					}
					State->MatchmakingConfigurationArn = MatchmakingConfigurationArn;

					TArray<TSharedPtr<FJsonValue>> Teams = JsonObject->GetArrayField("teams");
					for (TSharedPtr<FJsonValue> Team : Teams) {
						TSharedPtr<FJsonObject> TeamObj = Team->AsObject();
						FString TeamName = TeamObj->GetStringField("name");
						if (TeamName.Len() > 0) {
							UE_LOG(LogTemp, Warning, TEXT("team: %s"), *(TeamName));
						}
						TArray<TSharedPtr<FJsonValue>> Players = TeamObj->GetArrayField("players");

						for (TSharedPtr<FJsonValue> Player : Players) {
							TSharedPtr<FJsonObject> PlayerObj = Player->AsObject();
							FString PlayerId = PlayerObj->GetStringField("playerId");
							TSharedPtr<FJsonObject> Attributes = PlayerObj->GetObjectField("attributes");
							TSharedPtr<FJsonObject> Skill = Attributes->GetObjectField("skill");
							FString SkillValue = Skill->GetStringField("valueAttribute");
							FAttributeValue SkillAttributeValue;
							SkillAttributeValue.m_N = FCString::Atof(*SkillValue);

							FPlayer FPlayerObj;

							if (PlayerId.Len() > 0) {
								UE_LOG(LogTemp, Warning, TEXT("player id: %s"), *(PlayerId));
							}
							FPlayerObj.m_playerId = PlayerId;
							FPlayerObj.m_team = TeamName;
							FPlayerObj.m_playerAttributes.Add("skill", SkillAttributeValue);
							//TODO: FIGURE OUT HOW TO GET THE PING INFORMATION FOR BELOW
							//FPlayerObj.m_latencyInMs.Add(? , ? );
							State->PlayerIdToPlayer.Add(PlayerId, FPlayerObj);
						}
					}
				}
			}
			else if (Reason == Aws::GameLift::Server::Model::UpdateReason::BACKFILL_FAILED) {
				State->Reason = EUpdateReason::BACKFILL_FAILED;
			}
			else if (Reason == Aws::GameLift::Server::Model::UpdateReason::BACKFILL_TIMED_OUT) {
				State->Reason = EUpdateReason::BACKFILL_TIMED_OUT;
			}
			else if (Reason == Aws::GameLift::Server::Model::UpdateReason::BACKFILL_CANCELLED) {
				State->Reason = EUpdateReason::BACKFILL_CANCELLED;
			}
		};

		Aws::GameLift::Server::ProcessTerminateFn OnProcessTerminate = [](void* Params)
		{
			FProcessTerminateState* State = (FProcessTerminateState*)Params;

			State->Status = true;
		};

		Aws::GameLift::Server::HealthCheckFn OnHealthCheck = [](void* Params)
		{
			FHealthCheckState* State = (FHealthCheckState*)Params;
			State->Status = true;

			return State->Status;
		};

		TArray<FString> CommandLineTokens;
		TArray<FString> CommandLineSwitches;
		//UE_LOG(LogTemp, Warning, TEXT("Command line arguments when starting the game: %s"), *(FString(FCommandLine::Get())));
		int Port = FURL::UrlConfig.DefaultPort;

		FCommandLine::Parse(FCommandLine::Get(), CommandLineTokens, CommandLineSwitches);
		
		for (FString Str : CommandLineSwitches)
		{
			//UE_LOG(LogTemp, Warning, TEXT("Attempt to extract port from command line arguments: %s"), *(Str));
			FString Name;
			FString Value;

			if (Str.Split("=", &Name, &Value)) {
				if (Name.Equals("port")) {
					Port = FCString::Atoi(*Value);
				}
				else if (Name.Equals("password")) {
					ServerPassword = Value;
				}
			}
		}	

		const char* LogFile = "aLogFile.txt";
		const char** LogFiles = &LogFile;
		const Aws::GameLift::Server::LogParameters* LogParams = new Aws::GameLift::Server::LogParameters(LogFiles, 1);

		const Aws::GameLift::Server::ProcessParameters* Params =
			new Aws::GameLift::Server::ProcessParameters(
				OnStartGameSession,
				StartGameSessionState,
				OnUpdateGameSession,
				UpdateGameSessionState,
				OnProcessTerminate,
				ProcessTerminateState,
				OnHealthCheck,
				HealthCheckState,
				Port,
				*LogParams
			);

		auto ProcessReadyOutcome = Aws::GameLift::Server::ProcessReady(*Params);

		if (ProcessReadyOutcome.IsSuccess()) {

		}
		else {

		}
	}
	else {

	}
#endif
	GetWorldTimerManager().SetTimer(HandleBackfillUpdatesHandle, this, &AGameLiftTutorialGameMode::HandleBackfillUpdates, 1.f, true, 5.0f);
}

FString AGameLiftTutorialGameMode::InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal) {
	FString InitializedString = Super::InitNewPlayer(NewPlayerController, UniqueId, Options, Portal);
	// UNCOMMENT BELOW FOR TESTING LOCALLY
	/*APlayerState* State = NewPlayerController->PlayerState;
	if (State != nullptr) {
		AGameLiftTutorialPlayerState* PlayerState = Cast<AGameLiftTutorialPlayerState>(State);
		PlayerState->Team = "cowboys";
	}*/
	UE_LOG(LogTemp, Warning, TEXT("inside init new player"));
	
	if (*Options && Options.Len() > 0) {
		const FString& PlayerSessionId = UGameplayStatics::ParseOption(Options, "PlayerSessionId");
		const FString& PlayerId = UGameplayStatics::ParseOption(Options, "PlayerId");
		UE_LOG(LogTemp, Warning, TEXT("Player session id in init new player: %s"), *(PlayerSessionId));
		
		APlayerState* State = NewPlayerController->PlayerState;
		if (State != nullptr) {
			AGameLiftTutorialPlayerState* PlayerState = Cast<AGameLiftTutorialPlayerState>(State);
			if (PlayerState != nullptr) {
				PlayerState->PlayerSessionId = *PlayerSessionId;
				PlayerState->PlayerId = *PlayerId;
				UE_LOG(LogTemp, Warning, TEXT("state is not null in init new player"));

				// assign player's mesh color based on the player's team
				if (UpdateGameSessionState != nullptr && UpdateGameSessionState->PlayerIdToPlayer.Num() > 0) {
					UE_LOG(LogTemp, Warning, TEXT("Updategamesessionstate is not null and PlayerIdToPlayer is populated"));
					if (UpdateGameSessionState->PlayerIdToPlayer.Contains(PlayerId)) {
						FPlayer* PlayerObj = UpdateGameSessionState->PlayerIdToPlayer.Find(PlayerId);
						FString Team = PlayerObj->m_team;
						PlayerState->Team = *Team;
					}
				}
				else if (StartGameSessionState != nullptr && StartGameSessionState->PlayerIdToPlayer.Num() > 0) {
					UE_LOG(LogTemp, Warning, TEXT("StartGameSessionState is not null and PlayerIdToPlayer is populated"));

					if (StartGameSessionState->PlayerIdToPlayer.Contains(PlayerId)) {
						FPlayer* PlayerObj = StartGameSessionState->PlayerIdToPlayer.Find(PlayerId);
						FString Team = PlayerObj->m_team;
						PlayerState->Team = *Team;
					}
				}
			}
		}
	}
	return InitializedString;
}

void AGameLiftTutorialGameMode::CountDownUntilGameOver() {
	// update time left in the game
	if (GameLiftTutorialGameState != nullptr) {
		GameLiftTutorialGameState->LatestEvent = FString::FromInt(TimeUntilGameOver) + FString(" seconds until the game is over");
	}
	if (TimeUntilGameOver > 0) {
		TimeUntilGameOver--;
	}
	else {
		GetWorldTimerManager().ClearTimer(CountDownUntilGameOverHandle);
	}
}

void AGameLiftTutorialGameMode::EndGame() {
	GetWorldTimerManager().ClearTimer(EndGameHandle);
#if WITH_GAMELIFT
	auto TerminateGameSessionOutcome = Aws::GameLift::Server::TerminateGameSession();
	if (TerminateGameSessionOutcome.IsSuccess()) {

		auto ProcessEndingOutcome = Aws::GameLift::Server::ProcessEnding();
		if (ProcessEndingOutcome.IsSuccess())
		{
			FGenericPlatformMisc::RequestExit(false);
		}
	}
#endif
}

void AGameLiftTutorialGameMode::PickAWinningTeam() {
	GetWorldTimerManager().ClearTimer(CountDownUntilGameOverHandle);
	GetWorldTimerManager().ClearTimer(HandleBackfillUpdatesHandle);
	GetWorldTimerManager().ClearTimer(PickAWinningTeamHandle);
	
	int Num = FMath::RandRange(0, 1);
	FString WinningTeam;

	if (Num == 0) {
		// cowboys win
		WinningTeam = "cowboys";
	}
	else {
		// aliens win
		WinningTeam = "aliens";
	}
	if (GameLiftTutorialGameState != nullptr) {
		GameLiftTutorialGameState->LatestEvent = "GameEnded";
		GameLiftTutorialGameState->WinningTeam = WinningTeam;
	}
#if WITH_GAMELIFT
	TSharedPtr<FJsonObject> RequestObj = MakeShareable(new FJsonObject);
	RequestObj->SetStringField("winningTeam", WinningTeam);
	auto GameSessionIdOutcome = Aws::GameLift::Server::GetGameSessionId();
	if (GameSessionIdOutcome.IsSuccess()) {
		RequestObj->SetStringField("gameSessionId", GameSessionIdOutcome.GetResult());
		UE_LOG(LogTemp, Warning, TEXT("gamesessionid: %s"), *(FString(GameSessionIdOutcome.GetResult())));
	}
	else {
		GetWorldTimerManager().SetTimer(EndGameHandle, this, &AGameLiftTutorialGameMode::EndGame, 1.0f, false, 5.0f);
	}

	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);

	if (FJsonSerializer::Serialize(RequestObj.ToSharedRef(), Writer)) {
		UE_LOG(LogTemp, Warning, TEXT("About to call assign match results api at this url: %s"), *AssignMatchResultsUrl);

		// send a get request to google discovery document to retrieve endpoints
		TSharedRef<IHttpRequest> AssignMatchResultsRequest = HttpModule->CreateRequest();
		AssignMatchResultsRequest->OnProcessRequestComplete().BindUObject(this, &AGameLiftTutorialGameMode::OnAssignMatchResultsResponseReceived);
		AssignMatchResultsRequest->SetURL(AssignMatchResultsUrl);
		AssignMatchResultsRequest->SetVerb("POST");
		AssignMatchResultsRequest->SetHeader("Authorization", ServerPassword);
		AssignMatchResultsRequest->SetHeader("Content-Type", "application/json");
		AssignMatchResultsRequest->SetContentAsString(RequestBody);
		AssignMatchResultsRequest->ProcessRequest();
	}
	else {
		GetWorldTimerManager().SetTimer(EndGameHandle, this, &AGameLiftTutorialGameMode::EndGame, 1.0f, false, 5.0f);
	}
#endif
	
}

void AGameLiftTutorialGameMode::HandleBackfillUpdates() {
#if WITH_GAMELIFT
	if (!GameSessionActivated) {
		if (StartGameSessionState != nullptr && StartGameSessionState->Status) {
			GameSessionActivated = true;
			int NumRequestedPlayers = StartGameSessionState->PlayerIdToPlayer.Num();

			// create the game session's first backfill request using those players if there are less than four players
			if (NumRequestedPlayers < 4) {
				auto GameSessionIdOutcome = Aws::GameLift::Server::GetGameSessionId();
				if (GameSessionIdOutcome.IsSuccess()) {
					FString GameSessionArn = FString(GameSessionIdOutcome.GetResult());
					FString MatchmakingConfigurationArn = StartGameSessionState->MatchmakingConfigurationArn;
					UE_LOG(LogTemp, Warning, TEXT("in handle backfill updates gamesessionid: %s"), *(GameSessionArn));
					CreateBackfillRequest(GameSessionArn, MatchmakingConfigurationArn, StartGameSessionState->PlayerIdToPlayer);
				}
			}
			else {
				UpdateGameSessionState->Reason = EUpdateReason::BACKFILL_COMPLETED;
			}

			ConnectedPlayers = StartGameSessionState->PlayerIdToPlayer;
			WaitingForPlayersToJoin = true;
			WaitingForPlayersToJoinTime = 0;

			// start the clock for ending the game session after four minutes
			TimeUntilGameOver = 240;
			GetWorldTimerManager().SetTimer(CountDownUntilGameOverHandle, this, &AGameLiftTutorialGameMode::CountDownUntilGameOver, 1.0f, true, 0.0f);
			GetWorldTimerManager().SetTimer(PickAWinningTeamHandle, this, &AGameLiftTutorialGameMode::PickAWinningTeam, 1.0f, false, 240.0f);
		}
	}
	else {
		if (UpdateGameSessionState != nullptr && UpdateGameSessionState->Reason != EUpdateReason::BACKFILL_INITIATED && UpdateGameSessionState->Reason != EUpdateReason::BACKFILL_COMPLETED) {
			// something happened to the current backfill request 
			if (UpdateGameSessionState->Reason == EUpdateReason::MATCHMAKING_DATA_UPDATED) {
				int NumRequestedPlayers = UpdateGameSessionState->PlayerIdToPlayer.Num();

				// create the game session's first backfill request using those players if there are less than four players
				if (NumRequestedPlayers < 4) {
					auto GameSessionIdOutcome = Aws::GameLift::Server::GetGameSessionId();
					if (GameSessionIdOutcome.IsSuccess()) {
						FString GameSessionArn = FString(GameSessionIdOutcome.GetResult());
						FString MatchmakingConfigurationArn = UpdateGameSessionState->MatchmakingConfigurationArn;
						CreateBackfillRequest(GameSessionArn, MatchmakingConfigurationArn, UpdateGameSessionState->PlayerIdToPlayer);
					}
				}
				else {
					UpdateGameSessionState->Reason = EUpdateReason::BACKFILL_COMPLETED;
				}

				ConnectedPlayers = UpdateGameSessionState->PlayerIdToPlayer;
				WaitingForPlayersToJoin = true;
				WaitingForPlayersToJoinTime = 0;
			}
			else if (UpdateGameSessionState->Reason == EUpdateReason::BACKFILL_FAILED || UpdateGameSessionState->Reason == EUpdateReason::BACKFILL_TIMED_OUT
				|| UpdateGameSessionState->Reason == EUpdateReason::BACKFILL_CANCELLED) {
				WaitingForPlayersToJoin = false;
				WaitingForPlayersToJoinTime = 0;
				// make a new backfill request with the players currently in the game, because in this situation, we shouldn't be waiting on players to connect anymore
				TArray<APlayerState*> PlayerStates = GetWorld()->GetGameState()->PlayerArray;

				TMap<FString, FPlayer> ConnectedPlayersUpdated;
				for (APlayerState* PlayerState : PlayerStates) {
					if (PlayerState != nullptr) {
						AGameLiftTutorialPlayerState* GameLiftTutorialPlayerState = Cast<AGameLiftTutorialPlayerState>(PlayerState);
						if (GameLiftTutorialPlayerState != nullptr) {
							FPlayer* PlayerObj = ConnectedPlayers.Find(GameLiftTutorialPlayerState->PlayerId);
							if (PlayerObj != nullptr) {
								ConnectedPlayersUpdated.Add(GameLiftTutorialPlayerState->PlayerId, *PlayerObj);
							}
						}
					}
				}

				int NumPlayersJoined = ConnectedPlayersUpdated.Num();
				if (NumPlayersJoined > 0 && NumPlayersJoined < 4) {
					ConnectedPlayers = ConnectedPlayersUpdated;
					// start a new backfill request
					auto GameSessionIdOutcome = Aws::GameLift::Server::GetGameSessionId();
					if (GameSessionIdOutcome.IsSuccess()) {
						FString GameSessionArn = FString(GameSessionIdOutcome.GetResult());
						CreateBackfillRequest(GameSessionArn, UpdateGameSessionState->MatchmakingConfigurationArn, ConnectedPlayers);
					}
				}
				else if (NumPlayersJoined == 0) {
					// terminate the game session
					GetWorldTimerManager().ClearTimer(CountDownUntilGameOverHandle);
					GetWorldTimerManager().ClearTimer(HandleBackfillUpdatesHandle);
					GetWorldTimerManager().ClearTimer(PickAWinningTeamHandle);
					GetWorldTimerManager().ClearTimer(EndGameHandle);
					auto TerminateGameSessionOutcome = Aws::GameLift::Server::TerminateGameSession();
					if (TerminateGameSessionOutcome.IsSuccess()) {
						auto ProcessEndingOutcome = Aws::GameLift::Server::ProcessEnding();
						if (ProcessEndingOutcome.IsSuccess())
						{
							FGenericPlatformMisc::RequestExit(false);
						}
					}
				}
			}
		}
		else if (WaitingForPlayersToJoin) {
			if (WaitingForPlayersToJoinTime == 60) {
				// gave players 60 seconds to join the game since that is how much time they have to accept his/her player session
				WaitingForPlayersToJoin = false;
				WaitingForPlayersToJoinTime = 0;
				// check if the players currently in the game match the players who are supposed to be in the game
				TArray<APlayerState*> PlayerStates = GetWorld()->GetGameState()->PlayerArray;

				TMap<FString, FPlayer> ConnectedPlayersUpdated;
				for (APlayerState* PlayerState : PlayerStates) {
					if (PlayerState != nullptr) {
						AGameLiftTutorialPlayerState* GameLiftTutorialPlayerState = Cast<AGameLiftTutorialPlayerState>(PlayerState);
						if (GameLiftTutorialPlayerState != nullptr) {
							FPlayer* PlayerObj = ConnectedPlayers.Find(GameLiftTutorialPlayerState->PlayerId);
							if (PlayerObj != nullptr) {
								ConnectedPlayersUpdated.Add(GameLiftTutorialPlayerState->PlayerId, *PlayerObj);
							}
						}
					}
				}

				int NumPlayersJoined = ConnectedPlayersUpdated.Num();
				// if all the players in the game are the ones who are supposed to be in the game,
				// then do not do anything because there is currently a backfill request involving those players
				if (NumPlayersJoined > 0 && NumPlayersJoined < 4 && ConnectedPlayers.Num() != NumPlayersJoined) {
					ConnectedPlayers = ConnectedPlayersUpdated;
					// start a new backfill request
					auto GameSessionIdOutcome = Aws::GameLift::Server::GetGameSessionId();
					if (GameSessionIdOutcome.IsSuccess()) {
						FString GameSessionArn = FString(GameSessionIdOutcome.GetResult());
						// the only benefit of this if check is in the case where the matchmaking configuration arn ever changes, in our tutorial that won't be the case
						if (UpdateGameSessionState != nullptr && UpdateGameSessionState->MatchmakingConfigurationArn.Len() > 0) {
							CreateBackfillRequest(GameSessionArn, UpdateGameSessionState->MatchmakingConfigurationArn, ConnectedPlayers);
						}
						else if (StartGameSessionState != nullptr && StartGameSessionState->MatchmakingConfigurationArn.Len() > 0) {
							CreateBackfillRequest(GameSessionArn, StartGameSessionState->MatchmakingConfigurationArn, ConnectedPlayers);
						}
					}
				}
				else if (NumPlayersJoined == 0) {
					// terminate the game session
					GetWorldTimerManager().ClearTimer(CountDownUntilGameOverHandle);
					GetWorldTimerManager().ClearTimer(HandleBackfillUpdatesHandle);
					GetWorldTimerManager().ClearTimer(PickAWinningTeamHandle);
					GetWorldTimerManager().ClearTimer(EndGameHandle);
					auto TerminateGameSessionOutcome = Aws::GameLift::Server::TerminateGameSession();
					if (TerminateGameSessionOutcome.IsSuccess()) {
						auto ProcessEndingOutcome = Aws::GameLift::Server::ProcessEnding();
						if (ProcessEndingOutcome.IsSuccess())
						{
							FGenericPlatformMisc::RequestExit(false);
						}
					}
				}
			}
			else {
				WaitingForPlayersToJoinTime++;
			}
		}
		else if(UpdateGameSessionState != nullptr && (UpdateGameSessionState->Reason == EUpdateReason::BACKFILL_INITIATED || UpdateGameSessionState->Reason == EUpdateReason::BACKFILL_COMPLETED)){
			// if we are not waiting on players, either update the current backfill request 
			// or create a new backfill request to account for players leaving
			// then we should always check if anyone left the game
			// because this case will only happen if there are 4 people in the game at one point
			// but maybe just maybe someone left the game after there were at one point 4 people in the game
			// or there was a successful backfill request that only resulted in two people being in the game successfully
			TArray<APlayerState*> PlayerStates = GetWorld()->GetGameState()->PlayerArray;

			int NumPlayersInGame = PlayerStates.Num();
			if (NumPlayersInGame > 0 && NumPlayersInGame < 4 && ConnectedPlayers.Num() != NumPlayersInGame) {
				TMap<FString, FPlayer> ConnectedPlayersUpdated;
				for (APlayerState* PlayerState : PlayerStates) {
					if (PlayerState != nullptr) {
						AGameLiftTutorialPlayerState* GameLiftTutorialPlayerState = Cast<AGameLiftTutorialPlayerState>(PlayerState);
						if (GameLiftTutorialPlayerState != nullptr) {
							FPlayer* PlayerObj = ConnectedPlayers.Find(GameLiftTutorialPlayerState->PlayerId);
							if (PlayerObj != nullptr) {
								ConnectedPlayersUpdated.Add(GameLiftTutorialPlayerState->PlayerId, *PlayerObj);
							}
						}
					}
				}

				ConnectedPlayers = ConnectedPlayersUpdated;
				// start a new backfill request
				auto GameSessionIdOutcome = Aws::GameLift::Server::GetGameSessionId();
				if (GameSessionIdOutcome.IsSuccess()) {
					FString GameSessionArn = FString(GameSessionIdOutcome.GetResult());
					// the only benefit of this if check is in the case where the matchmaking configuration arn ever changes, in our tutorial that won't be the case
					if (UpdateGameSessionState != nullptr && UpdateGameSessionState->MatchmakingConfigurationArn.Len() > 0) {
						CreateBackfillRequest(GameSessionArn, UpdateGameSessionState->MatchmakingConfigurationArn, ConnectedPlayers);
					}
					else if (StartGameSessionState != nullptr && StartGameSessionState->MatchmakingConfigurationArn.Len() > 0) {
						CreateBackfillRequest(GameSessionArn, StartGameSessionState->MatchmakingConfigurationArn, ConnectedPlayers);
					}
				}
			}
			else if (NumPlayersInGame == 0) {
				// terminate the game session if everyone just left the game
				GetWorldTimerManager().ClearTimer(CountDownUntilGameOverHandle);
				GetWorldTimerManager().ClearTimer(HandleBackfillUpdatesHandle);
				GetWorldTimerManager().ClearTimer(PickAWinningTeamHandle);
				GetWorldTimerManager().ClearTimer(EndGameHandle);
				auto TerminateGameSessionOutcome = Aws::GameLift::Server::TerminateGameSession();
				if (TerminateGameSessionOutcome.IsSuccess()) {
					auto ProcessEndingOutcome = Aws::GameLift::Server::ProcessEnding();
					if (ProcessEndingOutcome.IsSuccess())
					{
						FGenericPlatformMisc::RequestExit(false);
					}
				}
			}
		}
	}
#endif
}

void AGameLiftTutorialGameMode::CreateBackfillRequest(FString GameSessionArn, FString MatchmakingConfigurationArn, TMap<FString, FPlayer> Players) {
#if WITH_GAMELIFT
	// use startgamesessionstate player map
	Aws::GameLift::Server::Model::StartMatchBackfillRequest* StartMatchBackfillRequest = new Aws::GameLift::Server::Model::StartMatchBackfillRequest();
	
	StartMatchBackfillRequest->SetGameSessionArn(TCHAR_TO_ANSI(*GameSessionArn));
	StartMatchBackfillRequest->SetMatchmakingConfigurationArn(TCHAR_TO_ANSI(*MatchmakingConfigurationArn));

	for (auto& Elem : Players)
	{
		Aws::GameLift::Server::Model::Player* Player = new Aws::GameLift::Server::Model::Player();
		FPlayer PlayerObj = Elem.Value;
		FString Team = PlayerObj.m_team;
		double SkillValue = (PlayerObj.m_playerAttributes.Find("skill"))->m_N;
		auto SkillAttributeValue = new Aws::GameLift::Server::Model::AttributeValue(SkillValue);

		Player->SetPlayerId(TCHAR_TO_ANSI(*(Elem.Key)));
		Player->SetTeam(TCHAR_TO_ANSI(*(Team)));
		Player->AddPlayerAttribute("skill", *SkillAttributeValue);
		StartMatchBackfillRequest->AddPlayer(*Player);
	}

	auto StartMatchBackfillOutcome = Aws::GameLift::Server::StartMatchBackfill(*StartMatchBackfillRequest);
	if (StartMatchBackfillOutcome.IsSuccess()) {
		UE_LOG(LogTemp, Warning, TEXT("Start match backfill request succeeded"));
		UpdateGameSessionState->Reason = EUpdateReason::BACKFILL_INITIATED;
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("Start match backfill request failed"));
	}
#endif
}

void AGameLiftTutorialGameMode::OnAssignMatchResultsResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful) {
	GetWorldTimerManager().SetTimer(EndGameHandle, this, &AGameLiftTutorialGameMode::EndGame, 1.0f, false, 5.0f);
}