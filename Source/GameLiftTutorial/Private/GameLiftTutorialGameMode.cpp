// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "GameLiftTutorialGameMode.h"
#include "GameLiftTutorial.h"
#include "Engine.h"
#include "EngineGlobals.h"
#include "GameLiftTutorialCharacter.h"
#include "GameLiftServerSDK.h"
#include "UObject/ConstructorHelpers.h"

AGameLiftTutorialGameMode::AGameLiftTutorialGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPersonCPP/Blueprints/ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
	ReadyTimeCount = 0;
	GameStarted = false;
	//Let's run this code only if GAMELIFT is enabled. Only with Server targets!
#if WITH_GAMELIFT

	//Getting the module first.
	gameLiftSdkModule = &FModuleManager::LoadModuleChecked<FGameLiftServerSDKModule>(FName("GameLiftServerSDK"));

	//InitSDK establishes a local connection with GameLift's agent to enable communication.
	gameLiftSdkModule->InitSDK();

	//Respond to new game session activation request. GameLift sends activation request 
	//to the game server along with a game session object containing game properties 
	//and other settings. Once the game server is ready to receive player connections, 
	//invoke GameLiftServerAPI.ActivateGameSession()
	auto onGameSession = [=](Aws::GameLift::Server::Model::GameSession gameSession)
	{
		gameLiftSdkModule->ActivateGameSession();
	};

	FProcessParameters* params = new FProcessParameters();
	params->OnStartGameSession.BindLambda(onGameSession);

	//OnProcessTerminate callback. GameLift invokes this before shutting down the instance 
	//that is hosting this game server to give it time to gracefully shut down on its own. 
	//In this example, we simply tell GameLift we are indeed going to shut down.
	params->OnTerminate.BindLambda([=]() {gameLiftSdkModule->ProcessEnding(); });

	//HealthCheck callback. GameLift invokes this callback about every 60 seconds. By default, 
	//GameLift API automatically responds 'true'. A game can optionally perform checks on 
	//dependencies and such and report status based on this info. If no response is received  
	//within 60 seconds, health status is recorded as 'false'. 
	//In this example, we're always healthy!
	params->OnHealthCheck.BindLambda([]() {return true; });

	//Here, the game server tells GameLift what port it is listening on for incoming player 
	//connections. In this example, the port is hardcoded for simplicity. Since active game
	//that are on the same instance must have unique ports, you may want to assign port values
	//from a range, such as:
	//const int32 port = FURL::UrlConfig.DefaultPort;
	//params->port;
	params->port = 7777;

	//Here, the game server tells GameLift what set of files to upload when the game session 
	//ends. GameLift uploads everything specified here for the developers to fetch later.
	TArray<FString> logfiles;
	logfiles.Add(TEXT("aLogFile.txt"));
	params->logParameters = logfiles;

	//Call ProcessReady to tell GameLift this game server is ready to receive game sessions!
	gameLiftSdkModule->ProcessReady(*params);
#endif
}

void AGameLiftTutorialGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) {
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
#if WITH_GAMELIFT
	if (*Options && Options.Len() > 0) {
		const FString& PlayerSessionId = UGameplayStatics::ParseOption(Options, "PlayerSessionId");
		if (PlayerSessionId.Len() > 0) {
			gameLiftSdkModule->AcceptPlayerSession(PlayerSessionId);
		}
	}
#endif
}

void AGameLiftTutorialGameMode::Logout(AController* Exiting) {
	Super::Logout(Exiting);
}

void AGameLiftTutorialGameMode::BeginPlay() {
	Super::BeginPlay();
	GetWorldTimerManager().SetTimer(ReadyCheckTimerHandle, this, &AGameLiftTutorialGameMode::CheckPlayerReadyCount, 1.0f, true);
}

FString AGameLiftTutorialGameMode::InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal) {
	FString InitializedString = Super::InitNewPlayer(NewPlayerController, UniqueId, Options, Portal);

	return InitializedString;
}

void AGameLiftTutorialGameMode::CheckPlayerReadyCount() {
	int NumPlayers = GetNumPlayers();
	int ReadyPlayers = 0;

	TArray<AActor*> Players;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGameLiftTutorialCharacter::StaticClass(), Players);

	for (AActor* Player : Players) {
		AGameLiftTutorialCharacter* Character = Cast<AGameLiftTutorialCharacter>(Player);
		if (Character->GameReady) {
			ReadyPlayers += 1;
		}
	}

	if ((ReadyPlayers / (NumPlayers * 1.0)) > 0.5) {
		ReadyTimeCount += 1;
		if (ReadyTimeCount >= 10) {
			StartGame();
		}
	}
	else {
		ReadyTimeCount = 0;
	}
}

void AGameLiftTutorialGameMode::StartGame() {
	GetWorldTimerManager().ClearTimer(ReadyCheckTimerHandle);
	GameStarted = true;
#if WITH_GAMELIFT
	gameLiftSdkModule->UpdatePlayerSessionCreationPolicy(EPlayerSessionCreationPolicy::DENY_ALL);
#endif
}