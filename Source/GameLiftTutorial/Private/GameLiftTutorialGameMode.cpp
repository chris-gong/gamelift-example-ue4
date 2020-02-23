// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "GameLiftTutorialGameMode.h"
#include "GameLiftTutorial.h"
#include "Engine/Engine.h"
#include "GameLiftTutorialCharacter.h"
#include "GameLiftTutorialPlayerState.h"
#include "GameLiftServerSDK.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"

AGameLiftTutorialGameMode::AGameLiftTutorialGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPersonCPP/Blueprints/ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
		PlayerStateClass = AGameLiftTutorialPlayerState::StaticClass();
	}

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
	params->port = FURL::UrlConfig.DefaultPort;

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

	if (*Options && Options.Len() > 0) {
		const FString& PlayerSessionId = UGameplayStatics::ParseOption(Options, "PlayerSessionId");
		if (PlayerSessionId.Len() > 0) {
#if WITH_GAMELIFT
			gameLiftSdkModule->AcceptPlayerSession(PlayerSessionId);
#endif
		}
	}

}

void AGameLiftTutorialGameMode::Logout(AController* Exiting) {
	Super::Logout(Exiting);
	APlayerState* State = Exiting->PlayerState;
	if (State != nullptr) {
		AGameLiftTutorialPlayerState* PlayerState = Cast<AGameLiftTutorialPlayerState>(State);
		const FString& PlayerSessionId = PlayerState->PlayerSessionId;
		if (PlayerSessionId.Len() > 0) {
#if WITH_GAMELIFT
			gameLiftSdkModule->RemovePlayerSession(PlayerSessionId);
#endif
		}
	}
	

}

void AGameLiftTutorialGameMode::BeginPlay() {
	Super::BeginPlay();
}

FString AGameLiftTutorialGameMode::InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal) {
	FString InitializedString = Super::InitNewPlayer(NewPlayerController, UniqueId, Options, Portal);
	if (*Options && Options.Len() > 0) {
		const FString& PlayerSessionId = UGameplayStatics::ParseOption(Options, "PlayerSessionId");
		if (PlayerSessionId.Len() > 0) {
			APlayerState* State = NewPlayerController->PlayerState;
			if (State != nullptr) {
				AGameLiftTutorialPlayerState* PlayerState = Cast<AGameLiftTutorialPlayerState>(State);
				PlayerState->PlayerSessionId = *PlayerSessionId;
				//PlayerState->Team = ?
			}
		}
	}
	return InitializedString;
}
